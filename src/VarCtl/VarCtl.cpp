#include "VarCtl.h"
#include "VarCtlUnity.h"
#include "VarCtlRuntime.h"

#include <atomic>

namespace VarCtl
{
    bool Init()
    {
        if (VarCtlUnity::IsReady())
            return true;

        if (!VarCtlUnity::Initialize())
            return false;

        if (!VarCtlRuntime::Create())
        {
            VarCtlUnity::Reset();
            return false;
        }

        Controller* controllers =
            VarCtlRuntime::Data();

        size_t count =
            VarCtlRuntime::Count();

        for (size_t i = 0; i < count; ++i)
        {
            Controller& controller =
                controllers[i];

            controller.ready = true;
            controller.Locate();

            if (controller.Found())
                controller.CacheOriginal();
        }

        return true;
    }

    void Tick()
    {
        Controller* controllers =
            VarCtlRuntime::Data();

        if (!VarCtlUnity::IsReady() ||
            !controllers)
        {
            return;
        }

        VarCtlUnity::EnsureThreadAttached();

        size_t count =
            VarCtlRuntime::Count();

        for (size_t i = 0; i < count; ++i)
            controllers[i].Tick();
    }

    void Shutdown()
    {
        Controller* controllers =
            VarCtlRuntime::Data();

        size_t count =
            VarCtlRuntime::Count();

        if (controllers)
        {
            for (size_t i = 0; i < count; ++i)
                controllers[i].Shutdown();
        }

        VarCtlRuntime::Destroy();
        VarCtlUnity::Reset();
    }

    size_t Count()
    {
        return VarCtlRuntime::Count();
    }

    const char* NameAt(size_t index)
    {
        Controller* controller =
            VarCtlRuntime::At(index);

        return controller
            ? controller->displayName.c_str()
            : "";
    }

    bool ReadyAt(size_t index)
    {
        Controller* controller =
            VarCtlRuntime::At(index);

        return controller
            ? controller->ready
            : false;
    }

    bool FoundAt(size_t index)
    {
        Controller* controller =
            VarCtlRuntime::At(index);

        return controller
            ? controller->Found()
            : false;
    }

    ValueType TypeAt(size_t index)
    {
        Controller* controller =
            VarCtlRuntime::At(index);

        return controller
            ? controller->type
            : ValueType::Float;
    }

    ControllerKind KindAt(size_t index)
    {
        Controller* controller =
            VarCtlRuntime::At(index);

        return controller
            ? controller->kind
            : ControllerKind::Field;
    }

    TransformProperty TransformPropertyAt(
        size_t index)
    {
        Controller* controller =
            VarCtlRuntime::At(index);

        return controller
            ? controller->transformProperty
            : TransformProperty::Position;
    }

    bool IsActiveAt(size_t index)
    {
        Controller* controller =
            VarCtlRuntime::At(index);

        return controller
            ? controller->active.load(
                std::memory_order_relaxed)
            : false;
    }

    void SetActiveAt(
        size_t index,
        bool value)
    {
        Controller* controller =
            VarCtlRuntime::At(index);

        if (!controller)
            return;

        controller->active.store(
            value,
            std::memory_order_relaxed
        );
    }

    float OverrideValueAt(size_t index)
    {
        Controller* controller =
            VarCtlRuntime::At(index);

        return controller
            ? controller->overrideFloat
            : 0.0f;
    }

    void SetOverrideValueAt(
        size_t index,
        float value)
    {
        Controller* controller =
            VarCtlRuntime::At(index);

        if (controller)
            controller->overrideFloat = value;
    }

    float* OverridePtrAt(size_t index)
    {
        Controller* controller =
            VarCtlRuntime::At(index);

        return controller
            ? &controller->overrideFloat
            : nullptr;
    }

    int OverrideIntAt(size_t index)
    {
        Controller* controller =
            VarCtlRuntime::At(index);

        return controller
            ? controller->overrideInt
            : 0;
    }

    void SetOverrideIntAt(
        size_t index,
        int value)
    {
        Controller* controller =
            VarCtlRuntime::At(index);

        if (controller)
            controller->overrideInt = value;
    }

    int* OverrideIntPtrAt(size_t index)
    {
        Controller* controller =
            VarCtlRuntime::At(index);

        return controller
            ? &controller->overrideInt
            : nullptr;
    }

    bool OverrideBoolAt(size_t index)
    {
        Controller* controller =
            VarCtlRuntime::At(index);

        return controller
            ? controller->overrideBool
            : false;
    }

    void SetOverrideBoolAt(
        size_t index,
        bool value)
    {
        Controller* controller =
            VarCtlRuntime::At(index);

        if (controller)
            controller->overrideBool = value;
    }

    Vector3 OverrideVector3At(size_t index)
    {
        Controller* controller =
            VarCtlRuntime::At(index);

        return controller
            ? controller->overrideVector3
            : Vector3{ 0.0f, 0.0f, 0.0f };
    }

    void SetOverrideVector3At(
        size_t index,
        Vector3 value)
    {
        Controller* controller =
            VarCtlRuntime::At(index);

        if (controller)
            controller->overrideVector3 = value;
    }

    float* OverrideVector3PtrAt(size_t index)
    {
        Controller* controller =
            VarCtlRuntime::At(index);

        return controller
            ? &controller->overrideVector3.x
            : nullptr;
    }

    float LowerLimitAt(size_t index)
    {
        Controller* controller =
            VarCtlRuntime::At(index);

        return controller
            ? controller->lowerLimit
            : 0.0f;
    }

    float UpperLimitAt(size_t index)
    {
        Controller* controller =
            VarCtlRuntime::At(index);

        return controller
            ? controller->upperLimit
            : 0.0f;
    }

    float CurrentValueAt(size_t index)
    {
        Controller* controller =
            VarCtlRuntime::At(index);

        return controller
            ? controller->ReadVector3().x
            : 0.0f;
    }

    int CurrentIntAt(size_t index)
    {
        Controller* controller =
            VarCtlRuntime::At(index);

        if (!controller ||
            !controller->Found() ||
            controller->kind != ControllerKind::Field ||
            controller->type != ValueType::Int)
        {
            return 0;
        }

        Il2CppApi& api =
            VarCtlUnity::Api();

        if (!api.field_get_value)
            return 0;

        int value = 0;

        api.field_get_value(
            reinterpret_cast<Il2CppObject>(
                controller->object),
            controller->field,
            &value
        );

        return value;
    }

    bool CurrentBoolAt(size_t index)
    {
        Controller* controller =
            VarCtlRuntime::At(index);

        if (!controller ||
            !controller->Found() ||
            controller->kind != ControllerKind::Field ||
            controller->type != ValueType::Bool)
        {
            return false;
        }

        Il2CppApi& api =
            VarCtlUnity::Api();

        if (!api.field_get_value)
            return false;

        bool value = false;

        api.field_get_value(
            reinterpret_cast<Il2CppObject>(
                controller->object),
            controller->field,
            &value
        );

        return value;
    }

    Vector3 CurrentVector3At(size_t index)
    {
        Controller* controller =
            VarCtlRuntime::At(index);

        return controller
            ? controller->ReadVector3()
            : Vector3{ 0.0f, 0.0f, 0.0f };
    }

    bool ShowSliderAt(size_t index)
    {
        Controller* controller =
            VarCtlRuntime::At(index);

        return controller
            ? controller->showSlider
            : false;
    }
}