#include "library.h"

#include <iostream>
#include <windows.h>
#include <logger/logger.h>

#include "features/feature_manager.h"
#include "hooks/hook_manager.h"
#include "environment.h"
#include "titan_hook.h"
#include "utils/converts.h"
#include "utils/jvm_utility.h"



void init(JavaVM* vm)
{
    env::g_jvm = vm;
    logger::log("[env] &a JavaVM*: &e", env::g_jvm);
    if (const auto vm = env::g_jvm; vm->GetEnv(reinterpret_cast<void**>(&env::g_env), JNI_VERSION_1_8) != JNI_OK)
    {
        if (vm->AttachCurrentThread(reinterpret_cast<void**>(&env::g_env), nullptr) == JNI_OK)
        {
            const auto jni_tools = utils::jvm_utility::get_jni_tools(env::g_env);
            env::g_jvmti_env = std::get<0>(jni_tools);
            logger::log("[env] &a JNIEnv*: &e", env::g_env);
            logger::log("[env] &a jvmtiEnv*: &e", env::g_jvmti_env);
            logger::log("[666] &a initializing features.");

            env::hook_manager.init();
            env::feature_manager.init();
        }
    }
}

void hello()
{
    logger::WriteRainbowLine("bbbbb!??");

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
    }
}
