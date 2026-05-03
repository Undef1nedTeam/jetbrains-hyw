package dev.undefinedteam.ccb.hooks;

import dev.undefinedteam.obfuscator.annotations.natives.NativeDefine;

import java.io.IOException;
import java.net.SocketTimeoutException;
import java.net.URL;
import java.util.List;

@NativeDefine
public class URLFilter {
    private static final List<String> ruleList = List.of(
            "https://account.jetbrains.com/lservice/rpc/validateKey.action",
            "https://account.jetbrains.com/lservice/rpc/validateLicense.action",
            "https://account.jetbrains.com/lservice/rpc/obtainAgreement.action",
            "https://account.jetbrains.com/lservice/rpc/obtainLicense.action",
            "https://account.jetbrains.com/lservice/rpc/fetchData.action",
            "https://account.jetbrains.com.cn/lservice/rpc/validateKey.action",
            "https://account.jetbrains.com.cn/lservice/rpc/validateLicense.action",
            "https://account.jetbrains.com.cn/lservice/rpc/obtainAgreement.action",
            "https://account.jetbrains.com.cn/lservice/rpc/obtainLicense.action",
            "https://account.jetbrains.com.cn/lservice/rpc/fetchData.action"
    );

    public static URL testURL(URL url) throws IOException {
        if (null == url) {
            return null;
        }
        for (String rule : ruleList) {
            if (!url.toString().startsWith(rule)) continue;
            throw new SocketTimeoutException("connect timed out");
        }
        return url;
    }
}
