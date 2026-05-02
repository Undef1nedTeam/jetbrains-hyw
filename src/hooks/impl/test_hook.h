//
// Created by abyss on 2026/5/2.
//

#ifndef JETBRAINS_DEV_TEST_HOOK_H
#define JETBRAINS_DEV_TEST_HOOK_H
#include "hooks/hook.h"


class test_hook : public hook
{
    void install();

public:
    bool enable() override;
    bool disable() override
    {
        return true;
    }
    std::string name() override
    {
        return "test";
    }
};


#endif //JETBRAINS_DEV_TEST_HOOK_H