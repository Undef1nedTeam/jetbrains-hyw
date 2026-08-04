# jetbrains-hyw

面向 JetBrains IDE 的字节码级拦截工具包 —— 借鉴自 **ja-netfilter** 思路的一个副本实现。

本仓库只包含项目的 **Java 侧**：`transformers`（字节码注入器）与 `hooks`（过滤逻辑）。
Java 侧编译产出 `ccb.jar` 后，再交给 **Onyx Shield**（私人项目，仅用于生成 JNI 侧的 define 代码）
通过 `JNI` `DefineClass` 把类注册进目标 JVM —— 也就是说，真正被注入进 IDE 的是 native 层，
而本仓库的 Java 代码是 native 层的"定义来源"。

## 工作原理

整个拦截分两层：

1. **Transformers（注入器）** —— 基于 ASM 遍历目标类字节码，定位特定方法，在方法头/返回处插入对 hooks 的调用。
2. **Hooks（过滤逻辑）** —— 纯 Java 的静态方法，实现具体的过滤 / 篡改行为，全部标注 `@NativeDefine`。

### Transformers

| Transformer | 目标方法 | 调用 Hook | 作用 |
| --- | --- | --- | --- |
| `HttpClientTransformer` | `sun.net.www.http.HttpClient.openServer()` | `URLFilter.testURL` | 对 JetBrains 许可服务地址（`account.jetbrains.com` / `account.jetbrains.com.cn` 下的 `validateKey` / `validateLicense` / `obtainAgreement` / `obtainLicense` / `fetchData`）直接抛 `SocketTimeoutException`，让校验请求"连接超时" |
| `InetAddressTransformer` | `java.net.InetAddress.getAllByName`、`isReachable` | `DNSFilter.testQuery` / `testReachable` | 对 `jetbrains.com`、`plugin.obroom.com` 抛 `UnknownHostException`，阻断基于 DNS 的校验与连通性探测 |
| `BigIntegerTransformer` | `java.math.BigInteger.oddModPow` | `ArgsFilter` / `ResultFilter` | 拦截 RSA 模幂运算：入口处替换参数，出口处直接返回预计算的签名结果，使签名校验恒通过 |
| `LicensingFacadeTransformer` | `getLicenseExpirationDate()` | `StackTraceRule.hook` | 返回"当前时间 + 30 天"的到期日期，滚动续期 |
| `ClassLoaderTransformer` | `PluginClassLoader.loadClass`、`Class.forName` | `ClassFilter.check` | 阻止加载 `dev.undefinedteam.ccb.*`，防止 IDE 发现本工具自身类（自保护） |
| `PluginManagerCoreTransformer` | （暂为 stub） | — | 占位 / 开发中，逻辑已被注释掉 |

### Hooks

| Hook | 说明 |
| --- | --- |
| `URLFilter` | URL 黑名单，命中 JetBrains 许可服务则抛超时异常 |
| `DNSFilter` | 域名黑名单（`jetbrains.com` / `plugin.obroom.com`），命中抛 `UnknownHostException`；`isReachable` 返回 false |
| `ArgsFilter` | `oddModPow` 参数过滤器：按 (指数, 模数) 命中缓存表，替换为已知公钥参数 |
| `ResultFilter` | `oddModPow` 结果过滤器：命中缓存表时返回预计算的正确签名，跳过真实运算 |
| `ClassFilter` | 类加载黑名单，禁止 `dev.undefinedteam.ccb` 包被外部加载 |
| `StackTraceRule` | 滚动返回 30 天后的到期日期；附带调用栈检查，规避部分调用上下文 |

## 目录结构

```
src/dev/undefinedteam/ccb/
  hooks/          # 过滤逻辑（native 注解）
  transformers/   # ASM 字节码注入器
asm/              # 内置的 ASM 分支（dev.undefinedteam.asm，避免外部依赖）
annotations/      # onyx-shield-annotations 注解 jar（@NativeDefine 等）
gen/              # 构建产物 + Onyx Shield 配置（config.yml、日志）
  ccb.jar         # 由 IDEA artifact 产出（out/artifacts/ccb/）
  ccb-out.jar     # Onyx Shield 混淆 + native 化后的最终产物
  config.yml      # Onyx Shield 处理配置（native_obfuscator.enabled = true）
```

## 构建流水线

1. IDEA artifact 把 `src` + `asm` 编译打包为 `out/artifacts/ccb/ccb.jar`。
2. Onyx Shield（私人项目，仅用于生成 JNI 侧的 define 代码）读取 `gen/config.yml`，消费 `ccb.jar`，输出 `gen/ccb-out.jar`。
3. `native_obfuscator.enabled = true` 时，`@NativeDefine` 类被编译成 native 代码，
   运行时由 `DefineClass` 注册进 JVM。

> `gen/` 已在 `.gitignore` 中忽略，属本地构建产物。

## 免责声明

本项目仅用于**学习与研究**（字节码插桩、JNI / native 化、JVM 机制等方向）目的，
请勿用于商业软件的破解与非法授权。请在遵守相关软件许可协议与法律法规的前提下使用。
