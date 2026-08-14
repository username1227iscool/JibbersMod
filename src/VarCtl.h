#pragma once

#include <cstddef>

// ============================================================================
// VarCtl -- controls an arbitrary list of game fields.
//
// To add a variable, add ONE line to g_controllers[] in VarCtl.cpp.
//
// Change ValueType to change how the field is read/written and how the GUI
// displays it.
//
// Supported types:
//     Float
//     Int
//     Bool
// ============================================================================

namespace VarCtl
{
    enum class ValueType
    {
        Float,
        Int,
        Bool
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

    // ------------------------------------------------------------------------
    // Float access
    // ------------------------------------------------------------------------

    float OverrideValueAt(size_t index);
    void SetOverrideValueAt(size_t index, float value);
    float* OverridePtrAt(size_t index);

    // ------------------------------------------------------------------------
    // Int access
    // ------------------------------------------------------------------------

    int OverrideIntAt(size_t index);
    void SetOverrideIntAt(size_t index, int value);
    int* OverrideIntPtrAt(size_t index);

    // ------------------------------------------------------------------------
    // Bool access
    // ------------------------------------------------------------------------

    bool OverrideBoolAt(size_t index);
    void SetOverrideBoolAt(size_t index, bool value);

    // ------------------------------------------------------------------------
    // Limits
    // ------------------------------------------------------------------------

    float LowerLimitAt(size_t index);
    float UpperLimitAt(size_t index);

    // ------------------------------------------------------------------------
    // Current value
    // ------------------------------------------------------------------------

    float CurrentValueAt(size_t index);

    bool CurrentBoolAt(size_t index);
    int CurrentIntAt(size_t index);

    // ------------------------------------------------------------------------
    // GUI
    // ------------------------------------------------------------------------

    bool ShowSliderAt(size_t index);
}

