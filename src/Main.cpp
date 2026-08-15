#include "Main.h"
#include "VarCtl.h"

ControllerConfig g_controllers[] =
{
    // Camera Distance
    {
        "Camera Distance",
        "distance",
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

    // Camera Rotation Speed
    {
        "Camera Rotation Speed",
        "rotateSpeed",
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

    // Camera Rotation
    {
        "Camera Rotation",
        "yAngle",
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

    // Camera Offset
    {
        "Camera Offset",
        "yOffset",
        VarCtl::ValueType::Float,
        1.0f,
        0.0f,
        0,
        false,
        { 0.0f, 0.0f, 0.0f },
        -10.0f,
        10.0f,
        true
    },

    // Stop Camera
    {
        "Stop Camera",
        "speed",
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

    // Pop Skis
    {
        "Pop Skis",
        "skiBreakForce",
        VarCtl::ValueType::Vector3,
        1.0f,
        0.0f,
        0,
        false,
        { 0.0f, 0.0f, 0.0f },
        0.0f,
        100.0f,
        false
    }
};

const size_t g_controllers_count =
sizeof(g_controllers) / sizeof(g_controllers[0]);