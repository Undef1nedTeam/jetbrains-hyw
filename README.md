# jetbrains-hyw

**English** | [**中文**](README_zh.md)

A bytecode-level interception toolkit for JetBrains IDEs, built along the lines of **ja-netfilter**.

This repository holds only the **Java side** of the project: `transformers` (bytecode injectors) and
`hooks` (filter logic). The Java side is compiled into `ccb.jar`, which is then handed to **Onyx Shield**
(a private project, used only to generate the JNI-side define code) to register those classes into the
target JVM through `JNI` + `DefineClass` — in other words, what actually gets injected into the IDE is
the native layer, and this Java code is the *source definition* of that layer.

## How it works

Interception happens in two layers:

1. **Transformers** — walk target classes with ASM, locate a specific method, and insert a call to a hook
   at the method entry / return point.
2. **Hooks** — plain-Java static methods implementing the actual filtering / tampering behaviour,
   all annotated `@NativeDefine`.

### Transformers

| Transformer | Target method | Hook invoked | Effect |
| --- | --- | --- | --- |
| `HttpClientTransformer` | `sun.net.www.http.HttpClient.openServer()` | `URLFilter.testURL` | Throws `SocketTimeoutException` for JetBrains license-service endpoints (`account.jetbrains.com` / `account.jetbrains.com.cn`: `validateKey` / `validateLicense` / `obtainAgreement` / `obtainLicense` / `fetchData`), so validation requests appear to "time out" |
| `InetAddressTransformer` | `java.net.InetAddress.getAllByName`, `isReachable` | `DNSFilter.testQuery` / `testReachable` | Throws `UnknownHostException` for `jetbrains.com` and `plugin.obroom.com`; blocks DNS-based checks and reachability probes |
| `BigIntegerTransformer` | `java.math.BigInteger.oddModPow` | `ArgsFilter` / `ResultFilter` | Intercepts RSA modular exponentiation: substitutes parameters on entry and returns a precomputed signature on exit, so signature verification always succeeds |
| `LicensingFacadeTransformer` | `getLicenseExpirationDate()` | `StackTraceRule.hook` | Returns a license expiration date 30 days from now (rolling renewal) |
| `ClassLoaderTransformer` | `PluginClassLoader.loadClass`, `Class.forName` | `ClassFilter.check` | Blocks loading of `dev.undefinedteam.ccb.*` so the IDE cannot discover the toolkit's own classes (self-protection) |
| `PluginManagerCoreTransformer` | *(stub)* | — | Placeholder / work in progress; logic is commented out |

### Hooks

| Hook | Description |
| --- | --- |
| `URLFilter` | URL blocklist — throws a timeout for JetBrains license services |
| `DNSFilter` | Hostname blocklist (`jetbrains.com` / `plugin.obroom.com`) — throws `UnknownHostException`; `isReachable` returns `false` |
| `ArgsFilter` | `oddModPow` argument filter — replaces arguments with cached known public-key parameters by (exponent, modulus) |
| `ResultFilter` | `oddModPow` result filter — returns the precomputed correct signature from the cache, skipping the real computation |
| `ClassFilter` | Class-loading blocklist — prevents the `dev.undefinedteam.ccb` package from being loaded externally |
| `StackTraceRule` | Returns a rolling 30-day expiration date; includes a stack-trace check to evade certain call contexts |

## Layout

```
src/dev/undefinedteam/ccb/
  hooks/          # filter logic (native-annotated)
  transformers/   # ASM bytecode injectors
asm/              # vendored ASM fork (dev.undefinedteam.asm, no external dependency)
annotations/      # onyx-shield-annotations jars (@NativeDefine etc.)
gen/              # build output + Onyx Shield config (config.yml, logs)
  ccb.jar         # built by the IDEA artifact (out/artifacts/ccb/)
  ccb-out.jar     # final artifact after Onyx Shield obfuscation + nativization
  config.yml      # Onyx Shield processing config (native_obfuscator.enabled = true)
```

## Build pipeline

1. The IDEA artifact compiles `src` + `asm` into `out/artifacts/ccb/ccb.jar`.
2. Onyx Shield (a private project, used only to generate the JNI-side define code) reads `gen/config.yml`, consumes `ccb.jar`, and emits `gen/ccb-out.jar`.
3. With `native_obfuscator.enabled = true`, the `@NativeDefine` classes are compiled into native code
   and registered into the JVM at runtime via `DefineClass`.

> `gen/` is ignored by `.gitignore` — it is local build output.

## Disclaimer

This project is intended **for learning and research purposes only** (bytecode instrumentation, JNI /
nativization, JVM internals). Do not use it to crack commercial software or bypass licensing.
Use it in compliance with the relevant software license agreements and applicable laws.
