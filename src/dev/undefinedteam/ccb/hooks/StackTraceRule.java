package dev.undefinedteam.ccb.hooks;

import dev.undefinedteam.obfuscator.annotations.natives.NativeDefine;

import java.util.Calendar;
import java.util.Date;
import java.util.regex.Pattern;

@NativeDefine
public class StackTraceRule {
    private static final Pattern PACKAGE_NAME_PATTERN = Pattern.compile("\\A\\p{ASCII}*\\z");

    public static boolean check() {
        for (StackTraceElement element : Thread.currentThread().getStackTrace()) {
            if (PACKAGE_NAME_PATTERN.matcher(element.getMethodName()).matches()) continue;
            return true;
        }
        return false;
    }

    public static Date hook() {
        for (StackTraceElement element : Thread.currentThread().getStackTrace()) {
            if (PACKAGE_NAME_PATTERN.matcher(element.getMethodName()).matches()) continue;
            Calendar calendar = Calendar.getInstance();
            calendar.add(Calendar.DATE, 30);
            return calendar.getTime();
        }
        return null;
    }
}
