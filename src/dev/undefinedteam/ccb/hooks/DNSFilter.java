package dev.undefinedteam.ccb.hooks;

import dev.undefinedteam.obfuscator.annotations.natives.NativeDefine;

import java.io.IOException;
import java.net.InetAddress;
import java.net.UnknownHostException;
import java.util.List;

@NativeDefine
public class DNSFilter {
    private static final List<String> ruleList = List.of(
            "jetbrains.com",
            "plugin.obroom.com"
    );

    public static String testQuery(String host) throws IOException {
        if (null == host) {
            return null;
        }
        for (String rule : ruleList) {
            if (!rule.equals(host)) continue;
            throw new UnknownHostException();
        }
        return host;
    }

    public static Object testReachable(InetAddress n) throws IOException {
        if (null == n) {
            return null;
        }
        for (String rule : ruleList) {
            if (!rule.equals(n.getHostName())) continue;
            return false;
        }
        return null;
    }
}
