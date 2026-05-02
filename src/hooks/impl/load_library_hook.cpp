#include "load_library_hook.h"

#include "environment.h"
#include "titan_hook.h"
#include "logger/logger.h"
#include "utils/converts.h"

using Fn_LoadLibraryW = HMODULE(WINAPI*)(LPCWSTR);
TitanHook<Fn_LoadLibraryW> g_load_library_w_hook{};

static HMODULE WINAPI HookLoadLibraryW(LPCWSTR lpLibFileName)
{
    auto real = g_load_library_w_hook.GetOrignalFunc();
    const std::string name = utils::converts::to_string(lpLibFileName);

    logger::log("[load-library-hook] loading &3", name);
    return real(lpLibFileName);
}

bool load_library_hook::enable()
{
    const std::string OK = "&aok";
    const std::string FAILED = "&cfailed";

    if (auto p = GetProcAddress(env::g_kernel32, "LoadLibraryW"))
    {
        g_load_library_w_hook.InitHook(reinterpret_cast<void*>(p), reinterpret_cast<void*>(&HookLoadLibraryW));
        logger::log("[load-library-hook] ", "hook &3LoadLibraryW&7 ",
                    g_load_library_w_hook.SetHook() ? OK : FAILED);
    }

    return true;
}
