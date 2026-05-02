#include "jvm_utility.h"

#include <unordered_map>
#include <mutex>
#include <windows.h>

#include <environment.h>

void utils::jvm_utility::pause_all_java_threads(JNIEnv* env, jvmtiEnv* vmti)
{
    local_frame frame{env, 16};
    jthread curthread;
    jthread* threads;
    jint thread_count;

    if (vmti->GetCurrentThread(&curthread) != JVMTI_ERROR_NONE)
        return;

    if (vmti->GetAllThreads(&thread_count, &threads) != JVMTI_ERROR_NONE)
        return;

    // TODO: Only suspend/resume threads that are actually active
    for (jint i = 0; i < thread_count; ++i)
    {
        if (env->IsSameObject(threads[i], curthread))
            continue;

        vmti->SuspendThread(threads[i]);
    }
}

void utils::jvm_utility::resume_all_java_threads(JNIEnv* env, jvmtiEnv* vmti)
{
    local_frame frame{env, 16};
    jthread curthread;
    jthread* threads;
    jint thread_count;


    if (vmti->GetCurrentThread(&curthread) != JVMTI_ERROR_NONE)
        return;

    if (vmti->GetAllThreads(&thread_count, &threads) != JVMTI_ERROR_NONE)
        return;

    // TODO: Only suspend/resume threads that are actually active
    for (jint i = 0; i < thread_count; ++i)
    {
        if (env->IsSameObject(threads[i], curthread))
            continue;

        vmti->ResumeThread(threads[i]);
    }
}

auto utils::jvm_utility::get_jni_tools(JNIEnv* env) -> std::tuple<jvmtiEnv*, JavaVM*>
{
    jvmtiEnv* jti = nullptr;
    JavaVM* jvm = nullptr;
    env->GetJavaVM(&jvm);
    jvm->GetEnv(reinterpret_cast<void**>(&jti), JVMTI_VERSION_1_2);
    return std::make_tuple(jti, jvm);
}

auto utils::jvm_utility::get_current_thread_env() -> JNIEnv*
{
    std::cout << "enter mutex" << std::endl;
    static std::unordered_map<std::thread::id, JNIEnv*> env_cache{};
    static std::mutex env_cache_mutex{};

    const std::lock_guard lock_guard{env_cache_mutex};
    if (auto it = env_cache.find(std::this_thread::get_id()); it != env_cache.end())
    {
        std::cout << "exit" << std::endl;
        return it->second;
    }
    else
    {
        JNIEnv* env = nullptr;
        JavaVM* jvm = env::g_jvm;
        if (!jvm)
            return nullptr;
        if (jvm->GetEnv((void**)&env, JNI_VERSION_1_8) == JNI_EDETACHED)
            jvm->AttachCurrentThreadAsDaemon((void**)&env, nullptr);
        if (env)
            env_cache.insert({std::this_thread::get_id(), env});
        std::cout << "exit" << std::endl;
        return env;
    }
    return nullptr;
}

auto utils::jvm_utility::get_thread_classloader(JNIEnv* jni_env, const std::string_view& thread_name) -> jobject
{
    //const auto jni_env = JNI::get_env();
    /* TODO: save to any class? */
    const auto [jti, jvm] = get_jni_tools(jni_env);
    jthread* thread = nullptr;
    jint thread_count;
    jti->GetAllThreads(&thread_count, &thread);
    jvmtiThreadInfo info;
    for (int i = 0; i < thread_count; i++)
    {
        jti->GetThreadInfo(thread[i], &info);
        if (thread_name == info.name)
        {
            return info.context_class_loader;
        }
    }
    return nullptr;
}

auto utils::jvm_utility::get_java_vm() -> JavaVM*
{
    static jsize count{0};
    static JavaVM* jvm{nullptr};
    if (count == 0)
    {
        env::functions::pfnGetCreatedJavaVMs(&jvm, 1, &count);
    }
    return jvm;
}
