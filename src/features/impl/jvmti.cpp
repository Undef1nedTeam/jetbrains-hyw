//
// Created by abyss on 2026/5/2.
//

#include "jvmti.h"

#include <jvmti.h>

#include "environment.h"
#include "logger/logger.h"
#include "transformers/transformers.h"
#include "utils/texts.h"
#include "native_definitions.hpp"
#include "resources/asm.h"
#include "utils/temps.h"

static void ClassTransform(jvmtiEnv* vmti,
                           JNIEnv* env,
                           jclass class_being_redefined,
                           jobject loader,
                           const char* name,
                           jobject protection_domain,
                           jint class_data_len,
                           const unsigned char* class_data,
                           jint* new_class_data_len,
                           unsigned char** new_class_data)
{
    if (!env::g_class_loader && utils::texts::equals(name, "com/intellij/ide/plugins/cl/PluginClassLoader"))
    {
        env::g_class_loader = env->NewGlobalRef(loader);

        native_def::init(env, loader);
        transformers::manager::init(env);
        transformers::manager::retransformAll(env, vmti);
    }

    if (env::g_class_loader)
    {
        if (const auto transformer = transformers::manager::find(name); transformer.has_value())
        {
            if (const auto bytes = transformer.value()->transform(env, name, class_data, class_data_len))
            {
                *new_class_data_len = env->GetArrayLength(bytes);
                vmti->Allocate(*new_class_data_len, new_class_data);
                env->GetByteArrayRegion(bytes, 0, *new_class_data_len, reinterpret_cast<jbyte*>(*new_class_data));
                env->DeleteLocalRef(bytes);

                logger::log("&7[&etransformer&7] Transformed '&3", name, "&7'");
            }
        }
    }
}

static TempFile g_asm_file(TempFile::CreateTempFile());

bool jvmti::enable()
{
    auto vmti = env::g_jvmti_env;
    jvmtiCapabilities capabilities = {
        .can_redefine_classes = 1,
        .can_suspend = 1,
        .can_redefine_any_class = 1,
        .can_generate_native_method_bind_events = 1,
        .can_retransform_classes = 1,
        .can_retransform_any_class = 1,
    };
    auto err = vmti->AddCapabilities(&capabilities);
    logger::log("jvmti &3AddCapabilities &e", err);

    jvmtiEventCallbacks callbacks = {};
    callbacks.ClassFileLoadHook = &ClassTransform;
    auto err1 = vmti->SetEventCallbacks(&callbacks, sizeof(callbacks));
    vmti->SetEventNotificationMode(JVMTI_ENABLE,
                                   JVMTI_EVENT_CLASS_FILE_LOAD_HOOK, nullptr);


    logger::log("[asm] &e", g_asm_file.path());
    if (auto res = g_asm_file.WriteBytesToFile(__asm_jar, __asm_jar_len))
    {
        vmti->AddToBootstrapClassLoaderSearch(g_asm_file.path().c_str());
    }

    logger::log("jvmti &3SetEventCallbacks &e", err1);

    return true;
}
