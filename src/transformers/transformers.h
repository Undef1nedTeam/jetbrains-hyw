#pragma once
#include <concepts>
#include <memory>
#include <optional>
#include <ranges>
#include <string>
#include <unordered_map>

#include "jni.h"
#include "resources.h"
#include "logger/logger.h"
#include "utils/jvm_utility.h"

namespace transformers
{
    class ITransformer
    {
    protected:
        jclass _clazz = nullptr;
        jmethodID _method = nullptr;
        jobject _instance = nullptr;

    public:
        explicit ITransformer(JNIEnv* env, const std::string& class_name)
        {
            // 优先用 FindClass 查找；若找不到（在 ClassFileLoadHook 回调中
            // FindClass 使用的是 bootstrap loader 上下文，无法找到由
            // PluginClassLoader 定义的类），则改用存储的 g_class_loader
            // 通过 ClassLoader.loadClass() 来查找。
            jclass klass = env->FindClass(class_name.c_str());
            if (env->ExceptionCheck()) env->ExceptionClear();

            if (!klass && env::g_class_loader)
            {
                // 将内部名称（斜杠）转换为二进制名称（点号）
                std::string binary_name = class_name;
                for (char& c : binary_name) if (c == '/') c = '.';

                jclass cl_class = env->GetObjectClass(env::g_class_loader);
                jmethodID load_class_mid = env->GetMethodID(
                    cl_class, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;");
                env->DeleteLocalRef(cl_class);

                if (load_class_mid)
                {
                    jstring jname = env->NewStringUTF(binary_name.c_str());
                    klass = reinterpret_cast<jclass>(
                        env->CallObjectMethod(env::g_class_loader, load_class_mid, jname));
                    env->DeleteLocalRef(jname);
                    if (env->ExceptionCheck()) { env->ExceptionClear(); klass = nullptr; }
                }
            }

            if (!klass)
            {
                logger::log("&c[transformer] &e", class_name, " &cclazz is null!");
                return;
            }

            _method = env->GetStaticMethodID(klass, "transform", "(Ljava/lang/String;[B)[B");
            if (!_method)
            {
                logger::log("&c[transformer] &e", class_name, " &ccannot find method 'transform'");
                if (env->ExceptionCheck())
                {
                    env->ExceptionDescribe();
                    env->ExceptionClear();
                }
                return;
            }

            _clazz = reinterpret_cast<jclass>(env->NewGlobalRef(klass));
            env->DeleteLocalRef(klass);

            logger::log("&7[transformer] &a", class_name, " &7initialized, clazz=&e", _clazz);
        }

        virtual ~ITransformer() = default;

        jbyteArray transform(JNIEnv* env, const char* name, const unsigned char* class_data, jint class_data_len) const
        {
            env->PushLocalFrame(16);

            jbyteArray input = env->NewByteArray(class_data_len);
            env->SetByteArrayRegion(input, 0, class_data_len, reinterpret_cast<const jbyte*>(class_data));

            jstring jname = env->NewStringUTF(name);

            const auto result = reinterpret_cast<jbyteArray>(
                env->CallStaticObjectMethod(_clazz, _method, jname, input)
            );

            // PopLocalFrame 会释放 frame 内所有 local ref，但保留并返回 result 的 local ref（在外层 frame）
            return reinterpret_cast<jbyteArray>(env->PopLocalFrame(result));
        }
    };

    class ClassLoaderTransformer : public ITransformer
    {
    public:
        explicit ClassLoaderTransformer(JNIEnv* env)
            : ITransformer(env, "dev/undefinedteam/ccb/transformers/ClassLoaderTransformer")
        {
        }
    };

    class LicensingFacadeTransformer : public ITransformer
    {
    public:
        explicit LicensingFacadeTransformer(JNIEnv* env)
            : ITransformer(env, "dev/undefinedteam/ccb/transformers/LicensingFacadeTransformer")
        {
        }
    };

    class PluginManagerCoreTransformer : public ITransformer
    {
    public:
        explicit PluginManagerCoreTransformer(JNIEnv* env)
            : ITransformer(env, "dev/undefinedteam/ccb/transformers/PluginManagerCoreTransformer")
        {
        }
    };

    class VMOptionsTransformer : public ITransformer
    {
    public:
        explicit VMOptionsTransformer(JNIEnv* env)
            : ITransformer(env, "dev/undefinedteam/ccb/transformers/VMOptionsTransformer")
        {
        }
    };

    class InetAddressTransformer : public ITransformer
    {
    public:
        explicit InetAddressTransformer(JNIEnv* env)
            : ITransformer(env, "dev/undefinedteam/ccb/transformers/InetAddressTransformer")
        {
        }
    };

    class HttpClientTransformer : public ITransformer
    {
    public:
        explicit HttpClientTransformer(JNIEnv* env)
            : ITransformer(env, "dev/undefinedteam/ccb/transformers/HttpClientTransformer")
        {
        }
    };

    class BigIntegerTransformer : public ITransformer
    {
    public:
        explicit BigIntegerTransformer(JNIEnv* env)
            : ITransformer(env, "dev/undefinedteam/ccb/transformers/BigIntegerTransformer")
        {
        }
    };

    class manager
    {
        static inline std::unordered_map<std::string, std::unique_ptr<ITransformer>> registry;

        template <class T>
        static void register_transformer(const std::string& target, JNIEnv* env) requires std::derived_from<
            T, ITransformer>
        {
            auto ptr = std::make_unique<T>(env);
            registry.emplace(target, std::move(ptr));
            logger::log("&7[transformers] &7registering transformer: &3", target);
        }

    public:
        static std::optional<ITransformer*> find(const std::string& name)
        {
            const auto it = registry.find(name);
            if (it == registry.end())
                return std::nullopt;
            return it->second.get();
        }

        static void init(JNIEnv* env)
        {
            register_transformer<ClassLoaderTransformer>("com/intellij/ide/plugins/cl/PluginClassLoader", env);
            register_transformer<LicensingFacadeTransformer>("com/intellij/ui/LicensingFacade", env);
            register_transformer<PluginManagerCoreTransformer>("com/intellij/ide/plugins/PluginManagerCore", env);
            // register_transformer<VMOptionsTransformer>("com/intellij/diagnostic/VMOptions", env);
            register_transformer<InetAddressTransformer>("java/net/InetAddress", env);
            register_transformer<HttpClientTransformer>("sun/net/www/http/HttpClient", env);
            register_transformer<BigIntegerTransformer>("java/math/BigInteger", env);
        }

        static void retransformAll(JNIEnv* env, jvmtiEnv* vmti)
        {
            if (!env::g_class_loader)
            {
               logger::log("&cnull classloader");
                return;
            }

            std::vector<std::string> targets = {
                "java/net/InetAddress","sun/net/www/http/HttpClient","java/math/BigInteger"
            };


            jclass cl_class = env->GetObjectClass(env::g_class_loader);
            jmethodID load_class_mid = env->GetMethodID(
                cl_class, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;");
            env->DeleteLocalRef(cl_class);

            jclass classes[64];

            jint count = 0;
            for (const auto& name : targets)
            {
                std::string binary_name = name;
                for (char& c : binary_name) if (c == '/') c = '.';
                jstring jname = env->NewStringUTF(binary_name.c_str());
                auto klass = reinterpret_cast<jclass>(
                    env->CallObjectMethod(env::g_class_loader, load_class_mid, jname));
                env->DeleteLocalRef(jname);
                if (env->ExceptionCheck()) { env->ExceptionClear(); klass = nullptr; }

                if (klass)
                    classes[count++] = klass;
            }
            if (count > 0)
                vmti->RetransformClasses(count, classes);
        }
    };
}
