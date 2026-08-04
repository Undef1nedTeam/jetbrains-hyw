# jetbrains-hyw

**English** | [**中文**](README_zh.md)

A native (C++ / Win32) implementation of a **ja-netfilter-style** bytecode instrumentation agent for JetBrains IDEs, delivered as a single **`winmm.dll`** and loaded via **DLL hijacking**.

Instead of being launched through the `-javaagent` JVM option like the original [ja-netfilter](https://github.com/ja-netfilter/ja-netfilter), this project is packaged as a *proxy DLL* that shadows the system `winmm.dll`. It injects itself into the IDE process during normal startup, waits for the bundled JVM to come up, and then uses **JVMTI** to rewrite JetBrains classes at load time.

This repository is the **native half** of the project. The bytecode-transform logic itself (the `dev/undefinedteam/ccb` transformers & hooks) lives in the separate **Java-side repository** and reaches this repo baked into `src/native_definitions.hpp` as embedded class bytes — see [Java side](#java-side).

> ⚠️ **Disclaimer:** this repository is provided for educational and reverse-engineering research only. It must not be used to infringe the licensing terms of JetBrains products.

---

## How the injection works (DLL hijacking)

JetBrains launchers (`idea64.exe`, etc.) import `winmm.dll`. Because of the Windows DLL search order, a `winmm.dll` placed in the same directory as the launcher is loaded **before** the one in `System32`.

`src/winmm.cpp` is generated from [AheadLib-x86-x64](https://github.com/strivexjun/AheadLib-x86-x64) and turns the module into a *transparent proxy*:

1. **`Load()`** — loads the real `C:\Windows\System32\winmm.dll` via `LoadLibrary`.
2. **`Init()`** — resolves all 180+ original `winmm` exports with `GetProcAddress` and stores them as thunk pointers, so every multimedia API call made by the IDE is forwarded to the genuine library.
3. **`DllMain` (`DLL_PROCESS_ATTACH`)** — after `Load()` + `Init()` succeed, spawns a worker thread that runs `hello()` and immediately returns, so the host process keeps starting normally.

The result is that the IDE *thinks* it loaded the real `winmm.dll` — every export behaves identically — while our initialization code runs inside the process at launch time.

---

## Startup sequence

```
DllMain (DLL_PROCESS_ATTACH)
  └─ Load()                         → map real System32\winmm.dll
  └─ Init()                         → resolve all winmm exports
  └─ CreateThread(hello)
       ├─ wait until "jvm.dll" is mapped into the process
       ├─ resolve JNI_GetCreatedJavaVMs / JVM_DefineClass
       ├─ poll until a JavaVM* exists
       └─ init(jvm)
            ├─ obtain JNIEnv + jvmtiEnv
            ├─ env::hook_manager.init()      → native API hooks (Detours)
            └─ env::feature_manager.init()   → JVM features (JVMTI / JNI)
```

Key entry point: `hello()` in `src/library.cpp`.

- It **waits for `jvm.dll`** (the bundled JetBrains Runtime) to be loaded, which is the signal that the JVM has started inside this process.
- It repeatedly calls `JNI_GetCreatedJavaVMs` until a `JavaVM*` appears (the JVM may not have created VMs yet).
- Once found, `init(jvm)` captures the `JavaVM*`, the `JNIEnv*` and the `jvmtiEnv*` into the global `env` namespace and boots the two managers.

---

## Architecture

### Native hooks — `src/hooks/` (Microsoft Detours)

`hook_manager` registers concrete `hook` implementations and enables the enabled ones.

- `TitanHook` (`src/titan_hook.h`) is a thin RAII wrapper around **Microsoft Detours** (`DetourTransactionBegin/Attach/Commit`).
- The only registered hook today is `load_library_hook` (`src/hooks/impl/`), which hooks `LoadLibraryW` from `kernel32.dll` and logs every library load — mostly a scaffolding example of the native-hook layer.

### Features — `src/features/`

`feature_manager` registers concrete `feature` implementations:

| Feature | Purpose |
|---|---|
| `jvmti` | **The core.** Adds JVMTI capabilities, installs a `ClassFileLoadHook` callback, exposes the embedded ASM library to the bootstrap class loader, and drives all class rewriting. |
| `crash_logger` | Installs a vectored exception handler (`AddVectoredExceptionHandler`) that logs exception code / address / RIP. Currently **disabled** in `is_enabled()`. |

### JVMTI class rewriting — `src/features/impl/jvmti.cpp`

This is where the ja-netfilter-style work actually happens:

1. `AddCapabilities` — `can_retransform_classes`, `can_redefine_any_class`, `can_suspend`, etc.
2. `SetEventCallbacks` + `SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_CLASS_FILE_LOAD_HOOK)`.
3. The embedded **ASM jar** (`src/resources/asm.h`, bytes `__asm_jar`/`__asm_jar_len`) is written to a temporary file (`TempFile`, `src/utils/temps.h`) and published to the bootstrap class loader via `AddToBootstrapClassLoaderSearch`, so the injected Java classes can parse/rewrite bytecode at runtime.

The `ClassTransform` callback does:

- The **first** time `com/intellij/ide/plugins/cl/PluginClassLoader` is seen, it:
  - keeps a global ref to that class loader (`env::g_class_loader`),
  - calls `native_def::init()` to inject the embedded Java plugin classes,
  - builds the transformer registry and retransforms already-loaded JDK classes.
- For every subsequently loaded class whose name matches a registered transformer, it calls the corresponding Java `transform(String, byte[])` static method and, if new bytes are returned, replaces the class data in the VM via `vmti->Allocate` + `GetByteArrayRegion`.

### Native → Java bridge — `src/transformers/transformers.h`

`ITransformer` locates a Java class named `dev/undefinedteam/ccb/transformers/…`, resolves its static `transform` method, and invokes it from the `ClassFileLoadHook` callback. It tries `JNIEnv::FindClass` first and falls back to `ClassLoader.loadClass()` through the captured `PluginClassLoader` (because inside the hook, `FindClass` only sees bootstrap-loader classes).

Registered targets:

| Target class | Transformer |
|---|---|
| `com/intellij/ide/plugins/cl/PluginClassLoader` | `ClassLoaderTransformer` |
| `com/intellij/ui/LicensingFacade` | `LicensingFacadeTransformer` |
| `com/intellij/ide/plugins/PluginManagerCore` | `PluginManagerCoreTransformer` |
| `com/intellij/diagnostic/VMOptions` | `VMOptionsTransformer` *(commented out)* |
| `java/net/InetAddress` | `InetAddressTransformer` |
| `sun/net/www/http/HttpClient` | `HttpClientTransformer` |
| `java/math/BigInteger` | `BigIntegerTransformer` |

`retransformAll()` force-loads `java/net/InetAddress`, `sun/net/www/http/HttpClient` and `java/math/BigInteger` through the plugin class loader and triggers `RetransformClasses`, so already-initialized JDK classes also get patched.

### Embedded Java plugin classes — `src/native_definitions.hpp`

The byte arrays (`native_def::def_01 … def_111`) are **not hand-written** — they are the compiled output of the project's **Java side**. The Java repository compiles its transformers & hooks into `ccb.jar`; the Onyx Shield pipeline turns that into these native define tables, and `native_def::init` injects them into the JVM with `JNIEnv::DefineClass`.

- **Hooks** (bootstrap loader) — the actual filtering/tampering logic:
  - `URLFilter` — blocklist of JetBrains license-service URLs; matched requests get a `SocketTimeoutException` (validation "times out")
  - `DNSFilter` — hostname blocklist (`jetbrains.com` / `plugin.obroom.com`); matched lookups get `UnknownHostException`
  - `ArgsFilter` / `ResultFilter` — intercept `BigInteger.oddModPow`: substitute known public-key parameters on entry and return a precomputed signature on exit, so RSA verification always succeeds
  - `ClassFilter` — blocks external loading of `dev.undefinedteam.ccb.*` (self-protection)
  - `StackTraceRule` — rolling license expiry "now + 30 days", with a stack-trace check
- **Transformers** (plugin class loader) — the ASM injectors `BigIntegerTransformer`, `PluginManagerCoreTransformer`, `LicensingFacadeTransformer`, `ClassLoaderTransformer`, `InetAddressTransformer`, `HttpClientTransformer`, one-to-one with the registry in `transformers/transformers.h`.

> Behavioral details are documented in the Java-side repository's README (`README.md` / `README_zh.md`).

---

## Project layout

```
jetbrains_dev/
├── CMakeLists.txt                      # Build → winmm.dll (x64, MSVC)
├── src/
│   ├── winmm.cpp                       # AheadLib proxy DLL — DllMain, export thunks
│   ├── winmm_jump.asm                  # MASM jump stub
│   ├── library.cpp / library.h         # hello() — wait for JVM, then init
│   ├── titan_hook.h                    # Detours RAII wrapper template
│   ├── environment.h                   # Global env: JavaVM / JNIEnv / jvmtiEnv / fn ptrs
│   ├── native_definitions.hpp          # Embedded Java plugin classes (byte arrays)
│   ├── hooks/                          # Native API hooks (Detours)
│   │   ├── hook_manager.* / hook.*     # Hook registry + base class
│   │   └── impl/load_library_hook.*    # Example: hooks LoadLibraryW
│   ├── features/                       # JVM-side features
│   │   ├── feature_manager.* / feature.*
│   │   └── impl/
│   │       ├── jvmti.*                 # Core: ClassFileLoadHook + class rewriting
│   │       └── crash_logger.*          # Vectored exception handler (disabled)
│   ├── transformers/transformers.h     # Native ↔ Java transformer bridge
│   ├── logger/                         # Colored console log (&c Minecraft-style codes)
│   ├── utils/                          # pattern, hex, converts, temps, jvm_utility, texts…
│   └── resources/asm.h                 # Embedded ASM bytecode-library jar
├── third_party/
│   ├── detour/                         # Microsoft Detours (detours.lib + headers)
│   └── java/                           # JNI / JVMTI headers
└── test/jvm.dll                        # Standalone jvm.dll copy for testing
```

---

## Building

Requirements:

- Windows x64
- Visual Studio / MSVC toolchain (the JNI/JVMTI headers are the standard Windows JDK ones; Detours `detours.lib` is prebuilt in `third_party/`)
- CMake ≥ 4.1 (developed with CLion)
- MASM support (for `winmm_jump.asm`)

```bash
cmake -S . -B build
cmake --build build --config Release
```

Output: `build/bin/winmm.dll` (x64, `winmm` is forced as the output name).

Notes:

- The project uses the dynamic CRT (`/MD`, `/MDd`).
- `TITANMODLOADER_EXPORTS`, `_WINDOWS`, `_USRDLL`, `NOMINMAX` etc. are defined in `CMakeLists.txt`.
- `detours.lib` must remain in `third_party/detour/` (it is git-ignored? — if you cloned fresh, rebuild/restore it).

> Deployment consists of placing the built `winmm.dll` next to the target launcher so the search-order hijack applies. This is the exact step that circumvents JetBrains' licensing — the project must only be used where you are licensed to do so.

---

## Dependencies

- [Microsoft Detours](https://github.com/microsoft/Detours) — native API hooking.
- JDK `jni.h` / `jni_md.h` / `jvmti.h` (vendored under `third_party/java/`).
- ASM (bytecode library) — embedded as a jar resource in `src/resources/asm.h`.
- [AheadLib-x86-x64](https://github.com/strivexjun/AheadLib-x86-x64) — generated the proxy-DLL scaffolding in `src/winmm.cpp`.

## Java side

This repo is the native loader/injector; the bytecode-transform logic is written in Java and lives in the **Java-side repository** (package `dev/undefinedteam/ccb`: `hooks/` for filter logic, `transformers/` for ASM injectors). Its READMEs describe the two-layer interception model — `README.md` (English) / `README_zh.md` (中文).

```
Java side (transformers + hooks)
   → ccb.jar
   → Onyx Shield (private; generates the JNI-side define code)
   → native_definitions.hpp   (this repo)
   → injected into the JVM via DefineClass (native_def::init)
```

## Related

- **Java-side repository** — the sibling repo this native layer consumes.
