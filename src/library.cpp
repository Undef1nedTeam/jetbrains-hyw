#include "library.h"

#include <iostream>
#include <windows.h>
#include <logger/logger.h>

#include "features/feature_manager.h"
#include "hooks/hook_manager.h"
#include "environment.h"
#include "resources.h"
#include "titan_hook.h"
#include "utils/converts.h"
#include "utils/jvm_utility.h"


void init(JavaVM* vm)
{
    env::g_jvm = vm;
    logger::log("[env] &a JavaVM*: &e", env::g_jvm);

    JNIEnv* jniEnv = nullptr;
    jvmtiEnv* jti = nullptr;

    // obtain JNIENV pointer
    jint result = vm->GetEnv(reinterpret_cast<void**>(&jniEnv), JNI_VERSION_1_6);
    if (result == JNI_EDETACHED)
        result = vm->AttachCurrentThread(reinterpret_cast<void**>(&jniEnv), nullptr);
    if (result != JNI_OK)
    {
        logger::log("[env] &c failed to obtain jni env.");
        return;
    }

    if (vm->GetEnv(reinterpret_cast<void**>(&jti), JVMTI_VERSION_1_2) != JNI_OK)
    {
        logger::log("[env] &c failed to obtain jvmti env.");
        return;
    }

    env::g_jvmti_env = jti;
    env::g_env = jniEnv;
    logger::log("[env] &a jvmtiEnv*: &e", env::g_jvmti_env);
    logger::log("[env] &a initializing features.");

    env::hook_manager.init();
    env::feature_manager.init();
}

void hello()
{
    logger::WriteRainbowLine("bbbbb!??");

    int counts = 0;
    while (counts < 20)
    {
        if (GetModuleHandle("jvm.dll") != nullptr)
        {
            break;
        }
        counts++;
        Sleep(50);
    }

    if (const auto jvm_handle = GetModuleHandle("jvm.dll"))
    {
        env::g_jvm_handle = jvm_handle;

        env::load_modules();
        env::functions::load_functions();

        while (true)
        {
            JavaVM* jvm = nullptr;
            jsize count = 0;
            if (env::functions::pfnGetCreatedJavaVMs &&
                env::functions::pfnGetCreatedJavaVMs(&jvm, 1, &count) == JNI_OK && count > 0 && jvm != nullptr)
            {
                init(jvm);
                break;
            }
            logger::log("&etrying get created java vms;");
            Sleep(50);
        }
    }
    else
    {
        logger::log("&c jvm.dll HMODULE not found, disabled.");
        MessageBoxA(nullptr, "jvm.dll HMODULE not found, disabled.", "err",MB_OK);
    }
}
