# jetbrains-hyw

**中文** | [**English**](README.md)

一个用 **C++ / Win32 原生实现**的、类似 [ja-netfilter](https://github.com/ja-netfilter/ja-netfilter) 的 JetBrains 系 IDE 字节码插桩工具。产物是一个独立的 **`winmm.dll`**，通过 **DLL 劫持（hijack）** 方式加载。

与通过 `-javaagent` JVM 参数加载的原版 ja-netfilter 不同，本项目被打包成**代理 DLL**，顶替系统 `winmm.dll`。它在 IDE 正常启动过程中被注入到进程内，等待内嵌 JVM 启动完成后，通过 **JVMTI** 在类加载时改写 JetBrains 相关类的字节码。

本仓库是项目的 **native 半边**。真正的字节码转换逻辑（`dev/undefinedteam/ccb` 的 transformers 与 hooks）位于独立的 **Java 侧仓库**，最终以内嵌类字节码的形式沉淀到 `src/native_definitions.hpp` —— 详见 [Java 侧](#java-侧)。

> ⚠️ **免责声明：** 本仓库仅用于学习与逆向研究，请勿用于侵犯 JetBrains 产品授权条款。

---

## 注入原理（DLL 劫持）

JetBrains 启动器（`idea64.exe` 等）会导入 `winmm.dll`。由于 Windows 的 DLL 搜索顺序，放在启动器同目录下的 `winmm.dll` 会**先于** `System32` 中的同名 DLL 被加载。

`src/winmm.cpp` 由 [AheadLib-x86-x64](https://github.com/strivexjun/AheadLib-x86-x64) 生成，把模块变成一个**透明代理**：

1. **`Load()`** — 用 `LoadLibrary` 加载真实的 `C:\Windows\System32\winmm.dll`。
2. **`Init()`** — 用 `GetProcAddress` 解析原 `winmm` 的 180+ 个导出并保存为跳板指针，保证 IDE 调用的所有多媒体 API 都被转发给真库。
3. **`DllMain`（`DLL_PROCESS_ATTACH`）** — 在 `Load()` + `Init()` 成功后，创建一条工作线程执行 `hello()` 并立即返回，宿主进程正常继续启动。

效果是：IDE 以为自己加载的是真正的 `winmm.dll`（所有导出行为一致），而我们的初始化代码已经在进程启动阶段悄悄运行。

---

## 启动流程

```
DllMain (DLL_PROCESS_ATTACH)
  └─ Load()                         → 映射真实的 System32\winmm.dll
  └─ Init()                         → 解析 winmm 全部导出
  └─ CreateThread(hello)
       ├─ 等待 "jvm.dll" 被映射进进程
       ├─ 解析 JNI_GetCreatedJavaVMs / JVM_DefineClass
       ├─ 轮询直到出现 JavaVM*
       └─ init(jvm)
            ├─ 取得 JNIEnv + jvmtiEnv
            ├─ env::hook_manager.init()      → 原生 API Hook（Detours）
            └─ env::feature_manager.init()   → JVM 功能（JVMTI / JNI）
```

核心入口是 `src/library.cpp` 中的 `hello()`：

- 先**等待 `jvm.dll`**（JetBrains Runtime）被加载——这是 JVM 已在本进程启动的信号。
- 反复调用 `JNI_GetCreatedJavaVMs` 直到出现 `JavaVM*`（JVM 可能尚未创建 VM）。
- 拿到 VM 后，`init(jvm)` 把 `JavaVM*`、`JNIEnv*`、`jvmtiEnv*` 存入全局 `env` 命名空间，并启动两个管理器。

---

## 架构说明

### 原生 Hook —— `src/hooks/`（Microsoft Detours）

`hook_manager` 负责注册具体 `hook` 实现并启用可用的 Hook。

- `TitanHook`（`src/titan_hook.h`）是对 **Microsoft Detours**（`DetourTransactionBegin/Attach/Commit`）的 RAII 封装模板。
- 目前注册的 Hook 是 `load_library_hook`（`src/hooks/impl/`），它对 `kernel32.dll` 的 `LoadLibraryW` 下钩子并记录每次库加载——更多是原生 Hook 层的脚手架示例。

### 功能 —— `src/features/`

`feature_manager` 注册具体 `feature` 实现：

| 功能 | 作用 |
|---|---|
| `jvmti` | **核心。** 添加 JVMTI 能力、安装 `ClassFileLoadHook` 回调、把内嵌 ASM 库暴露给启动类加载器，并驱动所有类改写。 |
| `crash_logger` | 安装向量化异常处理器（`AddVectoredExceptionHandler`），记录异常码 / 地址 / RIP。当前在 `is_enabled()` 中被**关闭**。 |

### JVMTI 类改写 —— `src/features/impl/jvmti.cpp`

这是真正干活的地方：

1. `AddCapabilities` — `can_retransform_classes`、`can_redefine_any_class`、`can_suspend` 等。
2. `SetEventCallbacks` + `SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_CLASS_FILE_LOAD_HOOK)`。
3. 把内嵌的 **ASM jar**（`src/resources/asm.h`，字节 `__asm_jar`/`__asm_jar_len`）通过 `TempFile`（`src/utils/temps.h`）写到临时文件，再用 `AddToBootstrapClassLoaderSearch` 发布给启动类加载器，让注入的 Java 类能在运行时解析/改写字节码。

`ClassTransform` 回调做的事：

- **第一次**见到 `com/intellij/ide/plugins/cl/PluginClassLoader` 时：
  - 保存该类加载器的全局引用（`env::g_class_loader`）；
  - 调用 `native_def::init()` 注入内嵌 Java 插件类；
  - 构建转换器注册表，并 retransform 已加载的 JDK 类。
- 之后每加载一个与注册表匹配的类，就调用对应 Java 类的静态 `transform(String, byte[])`；若返回新的字节，就通过 `vmti->Allocate` + `GetByteArrayRegion` 替换 VM 中的类数据。

### 原生 → Java 桥 —— `src/transformers/transformers.h`

`ITransformer` 定位名为 `dev/undefinedteam/ccb/transformers/…` 的 Java 类，解析其静态 `transform` 方法，并在 `ClassFileLoadHook` 回调里调用它。优先用 `JNIEnv::FindClass`，失败则通过保存的 `PluginClassLoader` 走 `ClassLoader.loadClass()`（因为在 Hook 回调中 `FindClass` 只能看到启动类加载器的类）。

已注册的目标：

| 目标类 | 转换器 |
|---|---|
| `com/intellij/ide/plugins/cl/PluginClassLoader` | `ClassLoaderTransformer` |
| `com/intellij/ui/LicensingFacade` | `LicensingFacadeTransformer` |
| `com/intellij/ide/plugins/PluginManagerCore` | `PluginManagerCoreTransformer` |
| `com/intellij/diagnostic/VMOptions` | `VMOptionsTransformer` *（已注释）* |
| `java/net/InetAddress` | `InetAddressTransformer` |
| `sun/net/www/http/HttpClient` | `HttpClientTransformer` |
| `java/math/BigInteger` | `BigIntegerTransformer` |

`retransformAll()` 会通过插件类加载器强制加载 `java/net/InetAddress`、`sun/net/www/http/HttpClient`、`java/math/BigInteger` 并触发 `RetransformClasses`，让已初始化的 JDK 类也能被打上补丁。

### 内嵌 Java 插件类 —— `src/native_definitions.hpp`

这些字节数组（`native_def::def_01 … def_111`）**不是手写的**，而是来自本项目 **Java 侧**的编译产物：Java 侧把 transformers 与 hooks 编译成 `ccb.jar`，经 Onyx Shield 流水线转成这份 native define 表，最后由 `native_def::init` 用 `JNIEnv::DefineClass` 注入 JVM。

- **Hook 类**（启动类加载器）—— 真正的过滤/篡改逻辑：
  - `URLFilter`：JetBrains 许可服务地址黑名单，命中抛 `SocketTimeoutException`（校验请求"连接超时"）
  - `DNSFilter`：域名黑名单（`jetbrains.com` / `plugin.obroom.com`），命中抛 `UnknownHostException`
  - `ArgsFilter` / `ResultFilter`：拦截 `BigInteger.oddModPow`——入口替换为已知公钥参数、出口直接返回预计算的签名，使 RSA 校验恒通过
  - `ClassFilter`：阻止外部加载 `dev.undefinedteam.ccb.*`（自保护）
  - `StackTraceRule`：滚动返回"当前时间 + 30 天"的许可到期时间，附带调用栈检查
- **转换器类**（插件类加载器）—— ASM 注入器：`BigIntegerTransformer`、`PluginManagerCoreTransformer`、`LicensingFacadeTransformer`、`ClassLoaderTransformer`、`InetAddressTransformer`、`HttpClientTransformer`，与 `transformers/transformers.h` 的注册表一一对应。

> 各类的行为细节见 Java 侧仓库的 README（`README.md` / `README_zh.md`）。

---

## 目录结构

```
jetbrains_dev/
├── CMakeLists.txt                      # 构建 → winmm.dll（x64，MSVC）
├── src/
│   ├── winmm.cpp                       # AheadLib 代理 DLL —— DllMain、导出跳板
│   ├── winmm_jump.asm                  # MASM 跳转 stub
│   ├── library.cpp / library.h         # hello() —— 等待 JVM 后初始化
│   ├── titan_hook.h                    # Detours RAII 封装模板
│   ├── environment.h                   # 全局环境：JavaVM / JNIEnv / jvmtiEnv / 函数指针
│   ├── native_definitions.hpp          # 内嵌 Java 插件类（字节数组）
│   ├── hooks/                          # 原生 API Hook（Detours）
│   │   ├── hook_manager.* / hook.*     # Hook 注册表 + 基类
│   │   └── impl/load_library_hook.*    # 示例：Hook LoadLibraryW
│   ├── features/                       # JVM 侧功能
│   │   ├── feature_manager.* / feature.*
│   │   └── impl/
│   │       ├── jvmti.*                 # 核心：ClassFileLoadHook + 类改写
│   │       └── crash_logger.*          # 向量化异常处理器（已禁用）
│   ├── transformers/transformers.h     # 原生 ↔ Java 转换器桥
│   ├── logger/                         # 彩色控制台日志（&c Minecraft 风格颜色码）
│   ├── utils/                          # pattern、hex、converts、temps、jvm_utility、texts…
│   └── resources/asm.h                 # 内嵌 ASM 字节码库 jar
├── third_party/
│   ├── detour/                         # Microsoft Detours（detours.lib + 头文件）
│   └── java/                           # JNI / JVMTI 头文件
└── test/jvm.dll                        # 供测试使用的独立 jvm.dll 副本
```

---

## 构建

环境要求：

- Windows x64
- Visual Studio / MSVC 工具链（JNI/JVMTI 头文件为 Windows 版 JDK 标准头；`detours.lib` 预编译在 `third_party/`）
- CMake ≥ 4.1（在 CLion 中开发）
- MASM 支持（用于 `winmm_jump.asm`）

```bash
cmake -S . -B build
cmake --build build --config Release
```

输出：`build/bin/winmm.dll`（x64，输出名强制为 `winmm`）。

说明：

- 使用动态 CRT（`/MD`、`/MDd`）。
- `TITANMODLOADER_EXPORTS`、`_WINDOWS`、`_USRDLL`、`NOMINMAX` 等在 `CMakeLists.txt` 中定义。
- `third_party/detour/detours.lib` 需要保留（若是全新克隆，请先恢复/重新构建它）。

> 部署方式是把编译好的 `winmm.dll` 放到目标启动器旁，使其命中搜索顺序劫持。这一步正是绕过 JetBrains 授权校验的手段——请仅在你有权使用该产品时使用本项目。

---

## 依赖

- [Microsoft Detours](https://github.com/microsoft/Detours) — 原生 API Hook。
- JDK `jni.h` / `jni_md.h` / `jvmti.h`（vendor 在 `third_party/java/`）。
- ASM（字节码库）—— 作为 jar 资源内嵌在 `src/resources/asm.h`。
- [AheadLib-x86-x64](https://github.com/strivexjun/AheadLib-x86-x64) — 生成 `src/winmm.cpp` 的代理 DLL 骨架。

## Java 侧

本仓库是 native 加载器/注入器；字节码转换逻辑用 Java 编写，位于独立的 **Java 侧仓库**（包 `dev/undefinedteam/ccb`：`hooks/` 是过滤逻辑，`transformers/` 是 ASM 注入器）。其 README 描述了两层拦截模型 —— `README.md`（英文）/ `README_zh.md`（中文）。

```
Java 侧（transformers + hooks）
   → ccb.jar
   → Onyx Shield（私人项目，仅用于生成 JNI 侧的 define 代码）
   → native_definitions.hpp   （本仓库）
   → 经 DefineClass 注入 JVM（native_def::init）
```

## 相关项目

- **Java 侧仓库** — 本 native 层所消费的兄弟仓库。
