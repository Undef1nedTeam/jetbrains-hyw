package dev.undefinedteam.ccb.hooks;

import dev.undefinedteam.obfuscator.annotations.natives.NativeDefine;

import java.util.List;

@NativeDefine
public class ClassFilter {
    private static final List<String> PREVENT_LOAD_PACKAGES = List.of("dev.undefinedteam.ccb");

    public static void check(String name) throws Exception {
        if (PREVENT_LOAD_PACKAGES.stream().anyMatch(name::startsWith)) {
            throw new ClassNotFoundException(name);
        }
    }
}
