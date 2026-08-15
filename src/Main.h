#pragma once

#include "VarCtl.h"

#include <cstddef>

// ============================================================================
// CONTROLLER CONFIGURATION
// ============================================================================

struct ControllerConfig
{
    const char* name;

    // GameObject hierarchy path.
    //
    // Field:
    //     path to GameObject containing the field
    //
    // Transform:
    //     path to GameObject whose Transform is controlled
    const char* id;

    // Field name.
    //
    // Only used for ControllerKind::Field.
    //
    // Transform controllers should use "".
    const char* fieldName;

    VarCtl::ControllerKind kind;

    VarCtl::ValueType valueType;

    // Only used for Transform controllers.
    VarCtl::TransformProperty transformProperty;

    float step;

    float defaultVal;

    int flags;

    bool isBool;

    float vectorOverride[3];

    float minVal;

    float maxVal;

    bool hasSlider;
};


// ============================================================================
// CONTROLLER CONFIGURATION ARRAY
// ============================================================================

extern ControllerConfig g_controllers[];

extern const size_t g_controllers_count;