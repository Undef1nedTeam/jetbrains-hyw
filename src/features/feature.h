//
// Created by abyss on 2026/5/2.
//

#ifndef JETBRAINS_DEV_FEATURE_H
#define JETBRAINS_DEV_FEATURE_H
#include <string>


class feature
{
private:
    bool enabled = true;

public:
    virtual ~feature() = default;
    virtual bool enable()
    {
        return true;
    }
    virtual bool disable()
    {
        return true;
    }
    virtual std::string name() = 0;

    [[nodiscard]] virtual bool is_enabled() const
    {
        return this->enabled;
    }
};


#endif //JETBRAINS_DEV_FEATURE_H