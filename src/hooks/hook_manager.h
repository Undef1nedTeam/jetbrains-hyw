//
// Created by abyss on 2026/5/2.
//

#ifndef JETBRAINS_DEV_HOOK_MANAGER_H
#define JETBRAINS_DEV_HOOK_MANAGER_H
#include <memory>
#include <optional>
#include <unordered_map>

#include "hook.h"
#include "impl/load_library_hook.h"
#include "logger/logger.h"


class hook_manager
{
private:
    std::unordered_map<std::string, std::unique_ptr<hook>> hooks;

    template <class T>
    void register_hook() requires (std::derived_from<T, hook>)
    {
        auto ptr = std::make_unique<T>();
        const std::string hook_name = ptr->name();
        hooks.emplace(hook_name, std::move(ptr));
        logger::log("&7[&ahooks&7] registering hook: &3", hook_name);
    }

public:
    void init()
    {
        register_hook<load_library_hook>();

        for (auto it = hooks.begin(); it != hooks.end(); ++it)
        {
            if (it->second->is_enabled())
            {
                logger::log("&7[&ahooks&7] initializing hook: &3", it->second->name());
                it->second->enable();
            }
        }
    }

    std::optional<hook*> find_hook(const std::string& name)
    {
        const auto it = hooks.find(name);
        if (it == hooks.end())
            return std::nullopt;

        return std::make_optional(it->second.get());
    }
};


#endif //JETBRAINS_DEV_HOOK_MANAGER_H
