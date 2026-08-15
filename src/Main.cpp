#include "Main.h"
#include "VarCtl.h"

// ============================================================================
// CONTROLLER CONFIGURATION
//
// This file ONLY describes what we want VarCtl to control.
//
// The actual runtime Controller lives privately inside VarCtl.cpp.
// ============================================================================

ControllerConfig g_controllers[] =
{
    // ========================================================================
    // CAMERA
    // ========================================================================

    {
        "Camera Distance",
        "distance",

        ControllerKind::Field,

        VarCtl::ValueType::Float,

        1.0f,
        10.0f,

        0,
        false,

        { 0.0f, 0.0f, 0.0f },

        -10.0f,
        50.0f,

        true
    },

    {
        "Camera Rotation Speed",
        "rotateSpeed",

        ControllerKind::Field,

        VarCtl::ValueType::Float,

        1.0f,
        10.0f,

        0,
        false,

        { 0.0f, 0.0f, 0.0f },

        0.0f,
        360.0f,

        true
    },

    {
        "Camera Rotation",
        "yAngle",

        ControllerKind::Field,

        VarCtl::ValueType::Float,

        1.0f,
        10.0f,

        0,
        false,

        { 0.0f, 0.0f, 0.0f },

        0.0f,
        360.0f,

        true
    },

    {
        "Camera Offset",
        "yOffset",

        ControllerKind::Field,

        VarCtl::ValueType::Float,

        1.0f,
        0.0f,

        0,
        false,

        { 0.0f, 0.0f, 0.0f },

        -100.0f,
        100.0f,

        true
    },

    {
        "Stop Camera",
        "speed",

        ControllerKind::Field,

        VarCtl::ValueType::Float,

        1.0f,
        0.0f,

        0,
        false,

        { 0.0f, 0.0f, 0.0f },

        0.0f,
        360.0f,

        false
    },


    // ========================================================================
    // SKI
    // ========================================================================

    {
        "Pop Skis",
        "skiBreakForce",

        ControllerKind::Field,

        VarCtl::ValueType::Vector3,

        1.0f,
        0.0f,

        0,
        false,

        { 0.0f, 0.0f, 0.0f },

        0.0f,
        100.0f,

        false
    },


    // ========================================================================
    // TRANSFORMS
    //
    // These are NOT ordinary fields.
    //
    // The id is the object name that VarCtl will search for.
    // ========================================================================

    {
        "Left Ski Position",
        "LeftSki",

        ControllerKind::Transform,

        VarCtl::ValueType::Vector3,

        0.1f,
        0.0f,

        0,
        false,

        { 0.0f, 0.0f, 0.0f },

        -10.0f,
        10.0f,

        false
    },

    {
        "Left Ski Rotation",
        "LeftSki",

        ControllerKind::Transform,

        VarCtl::ValueType::Vector3,

        1.0f,
        0.0f,

        0,
        false,

        { 0.0f, 0.0f, 0.0f },

        -360.0f,
        360.0f,

        false
    },

    {
        "Left Ski Scale",
        "LeftSki",

        ControllerKind::Transform,

        VarCtl::ValueType::Vector3,

        0.01f,
        1.0f,

        0,
        false,

        { 1.0f, 1.0f, 1.0f },

        0.0f,
        10.0f,

        false
    }
};


// ============================================================================
// CONTROLLER COUNT
// ============================================================================

const size_t g_controllers_count =
sizeof(g_controllers) /
sizeof(g_controllers[0]);