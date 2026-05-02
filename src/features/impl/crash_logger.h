//
// Created by abyss on 2026/5/2.
//

#ifndef JETBRAINS_DEV_CRASH_LOGGER_H
#define JETBRAINS_DEV_CRASH_LOGGER_H
#include "features/feature.h"
#include "utils/console.h"


class crash_logger : public feature
{
    PVOID crash_logger_handle = nullptr;
    void InstallCrashLogger();

public:
    bool enable() override
    {
        InstallCrashLogger();
        return true;
    }

    std::string name() override
    {
        return "crash_logger";
    }

    [[nodiscard]] bool is_enabled() const override
    {
        // 66会刷屏。。
        return false;
    }
};


#endif //JETBRAINS_DEV_CRASH_LOGGER_H
