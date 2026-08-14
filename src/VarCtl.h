#pragma once

#include <cstddef>

// ============================================================================
// VarCtl
//
// One controller entry in VarCtl.cpp can control a:
//     Float
//     Int
//     Bool
//     Vector3
//
// The GUI automatically selects the appropriate control from ValueType.
//
// Adding a variable should normally only require one line in g_controllers[].
// ============================================================================

namespace VarCtl
{
    enum class ValueType
    {
        Float,
        Int,
        Bool,
        Vector3
    };

    struct Vector3
    {
        float x;
        float y;
        float z;
    };

    bool Init();
    void Tick();
    void Shutdown();

    size_t Count();

    const char* NameAt(size_t index);

    bool ReadyAt(size_t index);
    bool FoundAt(size_t index);

    ValueType TypeAt(size_t index);

    bool IsActiveAt(size_t index);
    void SetActiveAt(size_t index, bool value);

    // ========================================================================
    // FLOAT
    // ========================================================================

    float OverrideValueAt(size_t index);
    void SetOverrideValueAt(size_t index, float value);
    float* OverridePtrAt(size_t index);

    // ========================================================================
    // INT
    // ========================================================================

    int OverrideIntAt(size_t index);
    void SetOverrideIntAt(size_t index, int value);
    int* OverrideIntPtrAt(size_t index);

    // ========================================================================
    // BOOL
    // ========================================================================

    bool OverrideBoolAt(size_t index);
    void SetOverrideBoolAt(size_t index, bool value);

    // ========================================================================
    // VECTOR3
    // ========================================================================

    Vector3 OverrideVector3At(size_t index);
    void SetOverrideVector3At(size_t index, Vector3 value);
    float* OverrideVector3PtrAt(size_t index);

    // ========================================================================
    // LIMITS
    //
    // Float:
    //     lowerLimit / upperLimit
    //
    // Int:
    //     values are converted to int by the GUI
    //
    // Vector3:
    //     the same lower/upper limit is used for X/Y/Z
    // ========================================================================

    float LowerLimitAt(size_t index);
    float UpperLimitAt(size_t index);

    // ========================================================================
    // CURRENT VALUES
    // ========================================================================

    float CurrentValueAt(size_t index);
    int CurrentIntAt(size_t index);
    bool CurrentBoolAt(size_t index);
    Vector3 CurrentVector3At(size_t index);

    // ========================================================================
    // GUI
    // ========================================================================

    bool ShowSliderAt(size_t index);
}

