//
// Created by abyss on 2026/5/2.
//

#ifndef JETBRAINS_DEV_JVMTI_H
#define JETBRAINS_DEV_JVMTI_H
#include "features/feature.h"


class jvmti : public feature
{
public:
    bool enable() override;
    std::string name() override { return "jvmti"; }
};


#endif //JETBRAINS_DEV_JVMTI_H