//
// Created by abyss on 2026/5/2.
//

#ifndef JETBRAINS_DEV_HOOK_H
#define JETBRAINS_DEV_HOOK_H
#include <string>


class hook
{
private:
    bool enabled = true;

public:
    virtual ~hook() = default;
    virtual bool enable() = 0;
    virtual bool disable() = 0;
    virtual std::string name() = 0;

    [[nodiscard]] virtual bool is_enabled() const
    {
        return this->enabled;
    }
};


#endif //JETBRAINS_DEV_HOOK_H
