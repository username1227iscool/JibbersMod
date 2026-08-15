#ifndef MAIN_H
#define MAIN_H

#include "VarCtl.h"
#include <cstddef>

struct ControllerConfig
{
    const char* name;
    const char* id;

    VarCtl::ValueType valueType;

    float step;
    float defaultVal;

    int flags;
    bool isBool;

    float vectorOverride[3];

    float minVal;
    float maxVal;

    bool hasSlider;
};

extern ControllerConfig g_controllers[];
extern const size_t g_controllers_count;

#endif