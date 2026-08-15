#include "Main.h"

// ============================================================================
// CONTROLLER MACROS
// ============================================================================

#define FIELD_CTRL(name, path, field, type, min, max, slider) \
    { name, path, field, VarCtl::ControllerKind::Field, VarCtl::ValueType::type, \
      VarCtl::TransformProperty::Position, 1.0f, (slider ? 10.0f : 0.0f), 0, false, \
      { 0.0f, 0.0f, 0.0f }, min, max, slider }

#define TRANSFORM_CTRL(name, path, property, type, min, max, slider) \
    { name, path, "", VarCtl::ControllerKind::Transform, VarCtl::ValueType::type, \
      VarCtl::TransformProperty::property, 1.0f, (slider ? 1.0f : 0.0f), 0, false, \
      { 1.0f, 1.0f, 1.0f }, min, max, slider }


// ============================================================================
// CONTROLLERS
// ============================================================================

ControllerConfig g_controllers[] =
{
    FIELD_CTRL("Camera Distance",       "YOUR_CAMERA_GAMEOBJECT_PATH", "distance",      Float,   -10.0f, 50.0f,  true),
    FIELD_CTRL("Camera Rotation Speed", "YOUR_CAMERA_GAMEOBJECT_PATH", "rotateSpeed",   Float,    0.0f, 360.0f,  true),
    FIELD_CTRL("Camera Rotation",       "YOUR_CAMERA_GAMEOBJECT_PATH", "yAngle",        Float,    0.0f, 360.0f,  true),
    FIELD_CTRL("Camera Offset",         "YOUR_CAMERA_GAMEOBJECT_PATH", "yOffset",       Float,  -10.0f, 10.0f,   true),
    FIELD_CTRL("Stop Camera",           "YOUR_CAMERA_GAMEOBJECT_PATH", "speed",         Float,    0.0f, 360.0f,  false),
    FIELD_CTRL("Pop Skis",              "YOUR_SKI_GAMEOBJECT_PATH",    "skiBreakForce", Vector3,  0.0f, 100.0f,  false),

    TRANSFORM_CTRL("Left Ski Scale",  "Bodies/SkiCharacter(Clone)/LeftSki/Mesh",  Scale, Vector3, 0.0f, 5.0f, true),
    TRANSFORM_CTRL("Right Ski Scale", "Bodies/SkiCharacter(Clone)/RightSki/Mesh", Scale, Vector3, 0.0f, 5.0f, true),
};


// ============================================================================
// COUNT
// ============================================================================

const size_t g_controllers_count =
sizeof(g_controllers) / sizeof(g_controllers[0]);