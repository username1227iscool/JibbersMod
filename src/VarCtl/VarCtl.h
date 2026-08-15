#pragma once

#include <cstddef>

namespace VarCtl
{
    enum class ValueType
    {
        Float,
        Int,
        Bool,
        Vector3
    };

    enum class ControllerKind
    {
        Field,
        Transform
    };

    enum class TransformProperty
    {
        Position,
        Rotation,
        Scale
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
    ControllerKind KindAt(size_t index);
    TransformProperty TransformPropertyAt(size_t index);

    bool IsActiveAt(size_t index);
    void SetActiveAt(size_t index, bool value);

    float OverrideValueAt(size_t index);
    void SetOverrideValueAt(size_t index, float value);
    float* OverridePtrAt(size_t index);

    int OverrideIntAt(size_t index);
    void SetOverrideIntAt(size_t index, int value);
    int* OverrideIntPtrAt(size_t index);

    bool OverrideBoolAt(size_t index);
    void SetOverrideBoolAt(size_t index, bool value);

    Vector3 OverrideVector3At(size_t index);
    void SetOverrideVector3At(size_t index, Vector3 value);
    float* OverrideVector3PtrAt(size_t index);

    float LowerLimitAt(size_t index);
    float UpperLimitAt(size_t index);

    float CurrentValueAt(size_t index);
    int CurrentIntAt(size_t index);
    bool CurrentBoolAt(size_t index);
    Vector3 CurrentVector3At(size_t index);

    bool ShowSliderAt(size_t index);
}