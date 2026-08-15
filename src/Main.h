#pragma once

#include <cstddef>
#include "VarCtl.h"

enum class ControllerKind
{
    Field,
    Transform
};

struct ControllerConfig
{
    const char* name;
    const char* id;

    VarCtl::ValueType type;

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