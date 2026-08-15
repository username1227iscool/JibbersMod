#include "Main.h"


// ============================================================================
// CONTROLLER CONFIGURATION
// ============================================================================

ControllerConfig g_controllers[] =
{
    // ========================================================================
    // FIELD CONTROLLERS
    // ========================================================================

    {
        "Camera Distance",

        // GameObject containing "distance"
        "YOUR_CAMERA_GAMEOBJECT_PATH",

        // Field name
        "distance",

        VarCtl::ControllerKind::Field,

        VarCtl::ValueType::Float,

        VarCtl::TransformProperty::Position,

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

        "YOUR_CAMERA_GAMEOBJECT_PATH",

        "rotateSpeed",

        VarCtl::ControllerKind::Field,

        VarCtl::ValueType::Float,

        VarCtl::TransformProperty::Position,

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

        "YOUR_CAMERA_GAMEOBJECT_PATH",

        "yAngle",

        VarCtl::ControllerKind::Field,

        VarCtl::ValueType::Float,

        VarCtl::TransformProperty::Position,

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

        "YOUR_CAMERA_GAMEOBJECT_PATH",

        "yOffset",

        VarCtl::ControllerKind::Field,

        VarCtl::ValueType::Float,

        VarCtl::TransformProperty::Position,

        1.0f,

        0.0f,

        0,

        false,

        { 0.0f, 0.0f, 0.0f },

        -10.0f,

        10.0f,

        true
    },


    {
        "Stop Camera",

        "YOUR_CAMERA_GAMEOBJECT_PATH",

        "speed",

        VarCtl::ControllerKind::Field,

        VarCtl::ValueType::Float,

        VarCtl::TransformProperty::Position,

        1.0f,

        0.0f,

        0,

        false,

        { 0.0f, 0.0f, 0.0f },

        0.0f,

        360.0f,

        false
    },


    {
        "Pop Skis",

        "YOUR_SKI_GAMEOBJECT_PATH",

        "skiBreakForce",

        VarCtl::ControllerKind::Field,

        VarCtl::ValueType::Vector3,

        VarCtl::TransformProperty::Position,

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
    // TRANSFORM CONTROLLERS
    // ========================================================================

    {
        "Left Ski Scale",

        "Bodies/SkiCharacter(Clone)/LeftSki/Mesh",

        "",

        VarCtl::ControllerKind::Transform,

        VarCtl::ValueType::Vector3,

        VarCtl::TransformProperty::Scale,

        1.0f,

        1.0f,

        0,

        false,

        { 1.0f, 1.0f, 1.0f },

        0.0f,

        5.0f,

        true
    },


    {
        "Right Ski Scale",

        "Bodies/SkiCharacter(Clone)/RightSki/Mesh",

        "",

        VarCtl::ControllerKind::Transform,

        VarCtl::ValueType::Vector3,

        VarCtl::TransformProperty::Scale,

        1.0f,

        1.0f,

        0,

        false,

        { 1.0f, 1.0f, 1.0f },

        0.0f,

        5.0f,

        true
    }
};


// ============================================================================
// COUNT
// ============================================================================

const size_t g_controllers_count =
sizeof(g_controllers) /
sizeof(g_controllers[0]);