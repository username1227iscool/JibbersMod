#pragma once
#include <cstddef>

// ============================================================================
// VarCtl -- controls an arbitrary list of game float fields.
// ----------------------------------------------------------------------------
// To add a new controlled variable: add ONE line to the g_controllers[] list
// in VarCtl.cpp. Nothing here in the header needs to change, and no new
// functions/namespaces are needed per variable.
//
// Everything is accessed by index (0, 1, 2, ...) matching the order you
// listed them in VarCtl.cpp. If you want named constants instead of raw
// numbers at call sites, add an enum here, e.g.:
//
//   enum Index { CamDist = 0, CamRot = 1 };
//
// ============================================================================

namespace VarCtl
{
    bool Init();
    void Tick();
    void Shutdown();

    size_t Count();

    const char* NameAt(size_t index);
    bool ReadyAt(size_t index);
    bool FoundAt(size_t index);

    bool IsActiveAt(size_t index);
    void SetActiveAt(size_t index, bool value);

    float OverrideValueAt(size_t index);
    void SetOverrideValueAt(size_t index, float value);

    // Direct pointer to the stored override value, for binding UI widgets
    // (e.g. ImGui::SliderFloat) straight to it instead of a get/set pair.
    float* OverridePtrAt(size_t index);

    float LowerLimitAt(size_t index);
    float UpperLimitAt(size_t index);

    // Live read of whatever the field currently holds in-game.
    float CurrentValueAt(size_t index);

    bool ShowSliderAt(size_t index);
}
