#include "Main.h"
#include "VarCtl.h"

// ============================================================
// Controller configuration
// ============================================================

ControllerConfig g_controllers[] =
{
    // --------------------------------------------------------
    // Camera Distance
    // --------------------------------------------------------
    {
        "Camera Distance",
        "distance",

        ControllerKind::Field,
        VarCtl::ValueType::Float,

        1.0f,

        10.0f,       // defaultFloat
        0,           // defaultInt
        false,       // defaultBool

        { 0.0f, 0.0f, 0.0f },

        -10.0f,
        50.0f,

        true
    },


    // --------------------------------------------------------
    // Camera Rotation Speed
    // --------------------------------------------------------
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


    // --------------------------------------------------------
    // Camera Rotation
    // --------------------------------------------------------
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


    // --------------------------------------------------------
    // Camera Offset
    // --------------------------------------------------------
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


    // --------------------------------------------------------
    // Stop Camera
    // --------------------------------------------------------
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


    // --------------------------------------------------------
    // Pop Skis
    // --------------------------------------------------------
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


    // ========================================================
    // TRANSFORM EXAMPLES
    //
    // These are ready for the transform system once
    // VarCtl.cpp supports ControllerKind::Transform.
    // ========================================================

    {
        "Left Ski Position",
        "Bodies/SkiCharacter(Clone)/LeftSki/Mesh",

        ControllerKind::Transform,
        VarCtl::ValueType::Vector3,

        1.0f,

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
        "Bodies/SkiCharacter(Clone)/LeftSki/Mesh",

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
        "Bodies/SkiCharacter(Clone)/LeftSki/Mesh",

        ControllerKind::Transform,
        VarCtl::ValueType::Vector3,

        1.0f,

        0.0f,
        0,
        false,

        { 1.0f, 1.0f, 1.0f },

        0.0f,
        10.0f,

        false
    }
};


// ============================================================
// Controller count
// ============================================================

const size_t g_controllers_count =
sizeof(g_controllers) / sizeof(g_controllers[0]);