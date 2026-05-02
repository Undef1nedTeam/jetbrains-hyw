//
// Created by abyss on 2026/5/2.
//

#ifndef JETBRAINS_DEV_FEATURE_MANAGER_H
#define JETBRAINS_DEV_FEATURE_MANAGER_H
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

#include "feature.h"
#include "impl/crash_logger.h"
#include "logger/logger.h"


class feature_manager
{
private:
    std::unordered_map<std::string, std::unique_ptr<feature>> features;

    template <class T>
    void register_feature() requires (std::derived_from<T, feature>)
    {
        auto ptr = std::make_unique<T>();
        const std::string hook_name = ptr->name();
        features.emplace(hook_name, std::move(ptr));
        logger::log("&7[&afeatures&7] registering feature: &3", hook_name);
    }

public:
    void init()
    {
        register_feature<crash_logger>();

        for (auto it = features.begin(); it != features.end(); ++it)
        {
            if (it->second->is_enabled())
            {
                logger::log("&7[&afeatures&7] initializing feature: &3", it->second->name());
                it->second->enable();
            }
        }
    }

    std::optional<feature*> find_feature(const std::string& name)
    {
        const auto it = features.find(name);
        if (it == features.end())
            return std::nullopt;

        return std::make_optional(it->second.get());
    }
};


#endif //JETBRAINS_DEV_FEATURE_MANAGER_H