#pragma once
#include "jni.h"

struct EnvGuard
{
    JavaVM* vm;
    JNIEnv* env = nullptr;
    bool attached = false;

    explicit EnvGuard(JavaVM* vm_) : vm(vm_)
    {
        if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK)
        {
            if (vm->AttachCurrentThread(reinterpret_cast<void**>(&env), nullptr) == JNI_OK)
            {
                attached = true;
            }
        }
    }

    ~EnvGuard()
    {
        if (attached) vm->DetachCurrentThread();
    }
};
