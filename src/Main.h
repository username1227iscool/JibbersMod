#pragma once

#include <cstddef>
#include "VarCtl.h"

// ============================================================
// ControllerKind
//
// Describes what kind of thing the controller operates on.
// ============================================================

enum class ControllerKind
{
    Field,
    Transform
};


// ============================================================
// ControllerConfig
//
// This is ONLY configuration.
//
// The actual runtime Controller lives privately inside
// VarCtl.cpp.
//
// Main.cpp can therefore define controllers without needing
// access to VarCtl.cpp's internal Controller implementation.
// ============================================================

struct ControllerConfig
{
    const char* name;
    const char* id;

    ControllerKind kind;

    VarCtl::ValueType valueType;

    float multiplier;

    float defaultFloat;
    int defaultInt;
    bool defaultBool;

    VarCtl::Vector3 defaultVector3;

    float minVal;
    float maxVal;

    bool hasSlider;
};


// ============================================================
// Global controller configuration
// ============================================================

extern ControllerConfig g_controllers[];
extern const size_t g_controllers_count;