#pragma once
#include "jni.h"
#include "jvmti.h"
#include "features/feature_manager.h"
#include "hooks/hook_manager.h"

namespace env
{
    inline HMODULE g_jvm_handle;
    inline HMODULE g_kernel32;

    inline hook_manager hook_manager;
    inline feature_manager feature_manager;

    inline JavaVM* g_jvm;
    inline jvmtiEnv* g_jvmti_env;
    inline JNIEnv* g_env;

    inline void load_modules()
    {
        g_kernel32 = GetModuleHandle("kernel32.dll");

        logger::log("[env] &a jvm.dll: &e", g_jvm_handle);
        logger::log("[env] &a kernel32.dll: &e ", g_kernel32);
    }

    namespace functions
    {
        using Fn_JNI_GetCreatedJavaVMs = jint(*)(JavaVM**, jsize, jsize*);

        static Fn_JNI_GetCreatedJavaVMs pfnGetCreatedJavaVMs;

        inline void load_functions()
        {
            pfnGetCreatedJavaVMs = reinterpret_cast<Fn_JNI_GetCreatedJavaVMs>(
                GetProcAddress(g_jvm_handle, "JNI_GetCreatedJavaVMs"));

            logger::log("[functions] &3 JNI_GetCreatedJavaVMs: &e", pfnGetCreatedJavaVMs);
        }
    }
}
