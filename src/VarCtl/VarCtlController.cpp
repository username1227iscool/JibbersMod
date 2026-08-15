#include "VarCtlController.h"
#include "VarCtlUnity.h"
#include "Main.h"

#include <cstring>

namespace
{
    bool SameVector3(
        const VarCtl::Vector3& a,
        const VarCtl::Vector3& b)
    {
        return a.x == b.x &&
            a.y == b.y &&
            a.z == b.z;
    }
}

void Controller::Configure(const ControllerConfig& config)
{
    displayName = config.name ? config.name : "";
    id = config.id ? config.id : "";
    fieldName = config.fieldName ? config.fieldName : "";

    kind = config.kind;
    type = config.valueType;
    transformProperty = config.transformProperty;

    multiplier = config.step;

    overrideFloat = config.defaultVal;
    overrideInt = static_cast<int>(config.defaultVal);
    overrideBool = config.isBool;

    overrideVector3 = {
        config.vectorOverride[0],
        config.vectorOverride[1],
        config.vectorOverride[2]
    };

    lowerLimit = config.minVal;
    upperLimit = config.maxVal;
    showSlider = config.hasSlider;
}

bool Controller::Found() const
{
    if (kind == VarCtl::ControllerKind::Transform)
    {
        return VarCtlUnity::PointerLooksValid(gameObject) &&
            VarCtlUnity::PointerLooksValid(transform);
    }

    return VarCtlUnity::PointerLooksValid(object) &&
        VarCtlUnity::PointerLooksValid(field);
}

void Controller::ClearLocatedObject()
{
    object = nullptr;
    field = nullptr;

    gameObject = nullptr;
    transform = nullptr;

    found = false;
    originalCached = false;
    touched = false;
}

bool Controller::Locate()
{
    ClearLocatedObject();

    if (kind == VarCtl::ControllerKind::Transform)
    {
        if (id.empty())
            return false;

        Il2CppObject go =
            VarCtlUnity::FindGameObject(id.c_str());

        if (!go)
            return false;

        if (!VarCtlUnity::InstanceAlive(go))
            return false;

        Il2CppObject tr =
            VarCtlUnity::GetTransform(go);

        if (!tr)
            return false;

        if (!VarCtlUnity::InstanceAlive(tr))
            return false;

        gameObject = go;
        transform = tr;
        found = true;

        return true;
    }

    if (kind == VarCtl::ControllerKind::Field)
    {
        if (fieldName.empty())
            return false;

        bool located =
            VarCtlUnity::FindFieldAndObject(
                fieldName.c_str(),
                type,
                object,
                field
            );

        found =
            located &&
            VarCtlUnity::PointerLooksValid(object) &&
            VarCtlUnity::PointerLooksValid(field);

        return found;
    }

    return false;
}

void Controller::CacheOriginal()
{
    if (!Found() || originalCached)
        return;

    Il2CppApi& api = VarCtlUnity::Api();

    if (kind == VarCtl::ControllerKind::Transform)
    {
        originalVector3 =
            VarCtlUnity::GetTransformVector(
                transform,
                transformProperty
            );

        lastTargetVector3 = originalVector3;
        originalCached = true;
        return;
    }

    if (kind == VarCtl::ControllerKind::Field)
    {
        if (!api.field_get_value)
            return;

        Il2CppObject instance =
            reinterpret_cast<Il2CppObject>(object);

        switch (type)
        {
        case VarCtl::ValueType::Float:
        {
            float value = 0.0f;

            api.field_get_value(
                instance,
                field,
                &value
            );

            originalFloat = value;
            lastTargetFloat = value;
            break;
        }

        case VarCtl::ValueType::Int:
        {
            int value = 0;

            api.field_get_value(
                instance,
                field,
                &value
            );

            originalInt = value;
            lastTargetInt = value;
            break;
        }

        case VarCtl::ValueType::Bool:
        {
            bool value = false;

            api.field_get_value(
                instance,
                field,
                &value
            );

            originalBool = value;
            lastTargetBool = value;
            break;
        }

        case VarCtl::ValueType::Vector3:
        {
            VarCtl::Vector3 value{
                0.0f,
                0.0f,
                0.0f
            };

            api.field_get_value(
                instance,
                field,
                &value
            );

            originalVector3 = value;
            lastTargetVector3 = value;
            break;
        }
        }

        originalCached = true;
    }
}

VarCtl::Vector3 Controller::ReadVector3()
{
    if (!Found())
        return { 0.0f, 0.0f, 0.0f };

    Il2CppApi& api = VarCtlUnity::Api();

    if (kind == VarCtl::ControllerKind::Transform)
    {
        return VarCtlUnity::GetTransformVector(
            transform,
            transformProperty
        );
    }

    if (kind == VarCtl::ControllerKind::Field)
    {
        if (!api.field_get_value)
            return { 0.0f, 0.0f, 0.0f };

        Il2CppObject instance =
            reinterpret_cast<Il2CppObject>(object);

        if (type == VarCtl::ValueType::Vector3)
        {
            VarCtl::Vector3 value{
                0.0f,
                0.0f,
                0.0f
            };

            api.field_get_value(
                instance,
                field,
                &value
            );

            return value;
        }

        if (type == VarCtl::ValueType::Float)
        {
            float value = 0.0f;

            api.field_get_value(
                instance,
                field,
                &value
            );

            return { value, 0.0f, 0.0f };
        }
    }

    return { 0.0f, 0.0f, 0.0f };
}

bool Controller::WriteVector3(
    const VarCtl::Vector3& value)
{
    if (!Found())
        return false;

    if (kind == VarCtl::ControllerKind::Transform)
    {
        if (VarCtlUnity::SetTransformVector(
            transform,
            transformProperty,
            value))
        {
            touched = true;
            return true;
        }
    }

    return false;
}

void Controller::InvalidateField()
{
    object = nullptr;
    field = nullptr;

    found = false;
    originalCached = false;
    touched = false;
    nextScanTick = 0;
}

void Controller::InvalidateTransform()
{
    gameObject = nullptr;
    transform = nullptr;

    found = false;
    originalCached = false;
    touched = false;
    nextScanTick = 0;
}

void Controller::Tick()
{
    if (!ready)
        return;

    VarCtlUnity::EnsureThreadAttached();

    if (!Found())
    {
        DWORD now = GetTickCount();

        if (now < nextScanTick)
            return;

        nextScanTick = now + 1000;

        if (!Locate())
            return;

        originalCached = false;
        touched = false;

        lastTargetVector3 = {
            0.0f,
            0.0f,
            0.0f
        };

        lastTargetFloat = 0.0f;
        lastTargetInt = 0;
        lastTargetBool = false;
    }

    if (kind == VarCtl::ControllerKind::Transform)
    {
        if (!VarCtlUnity::InstanceAlive(gameObject) ||
            !VarCtlUnity::InstanceAlive(transform))
        {
            InvalidateTransform();
            return;
        }
    }

    if (kind == VarCtl::ControllerKind::Field)
    {
        if (!VarCtlUnity::InstanceAlive(
            reinterpret_cast<Il2CppObject>(object)))
        {
            InvalidateField();
            return;
        }
    }

    CacheOriginal();

    if (!originalCached)
        return;

    bool enabled =
        active.load(std::memory_order_relaxed);

    if (kind == VarCtl::ControllerKind::Transform)
    {
        VarCtl::Vector3 target;

        if (enabled)
        {
            target = {
                overrideVector3.x * multiplier,
                overrideVector3.y * multiplier,
                overrideVector3.z * multiplier
            };
        }
        else
        {
            target = originalVector3;
        }

        if (!SameVector3(
            target,
            lastTargetVector3))
        {
            if (WriteVector3(target))
                lastTargetVector3 = target;
        }

        return;
    }

    if (kind == VarCtl::ControllerKind::Field)
    {
        Il2CppApi& api = VarCtlUnity::Api();

        if (!api.field_set_value)
            return;

        Il2CppObject instance =
            reinterpret_cast<Il2CppObject>(object);

        switch (type)
        {
        case VarCtl::ValueType::Float:
        {
            float target =
                enabled
                ? overrideFloat * multiplier
                : originalFloat;

            if (target != lastTargetFloat)
            {
                float value = target;

                api.field_set_value(
                    instance,
                    field,
                    &value
                );

                touched = true;
                lastTargetFloat = target;
            }

            break;
        }

        case VarCtl::ValueType::Int:
        {
            int target =
                enabled
                ? overrideInt
                : originalInt;

            if (target != lastTargetInt)
            {
                int value = target;

                api.field_set_value(
                    instance,
                    field,
                    &value
                );

                touched = true;
                lastTargetInt = target;
            }

            break;
        }

        case VarCtl::ValueType::Bool:
        {
            bool target =
                enabled
                ? overrideBool
                : originalBool;

            if (target != lastTargetBool)
            {
                bool value = target;

                api.field_set_value(
                    instance,
                    field,
                    &value
                );

                touched = true;
                lastTargetBool = target;
            }

            break;
        }

        case VarCtl::ValueType::Vector3:
        {
            VarCtl::Vector3 target;

            if (enabled)
            {
                target = {
                    overrideVector3.x * multiplier,
                    overrideVector3.y * multiplier,
                    overrideVector3.z * multiplier
                };
            }
            else
            {
                target = originalVector3;
            }

            if (!SameVector3(
                target,
                lastTargetVector3))
            {
                VarCtl::Vector3 value = target;

                api.field_set_value(
                    instance,
                    field,
                    &value
                );

                touched = true;
                lastTargetVector3 = target;
            }

            break;
        }
        }
    }
}

void Controller::Shutdown()
{
    if (!ready)
        return;

    Il2CppApi& api = VarCtlUnity::Api();

    if (touched && originalCached && Found())
    {
        if (kind == VarCtl::ControllerKind::Transform)
        {
            if (VarCtlUnity::InstanceAlive(gameObject) &&
                VarCtlUnity::InstanceAlive(transform))
            {
                WriteVector3(originalVector3);
            }
        }
        else if (kind == VarCtl::ControllerKind::Field)
        {
            if (VarCtlUnity::InstanceAlive(
                reinterpret_cast<Il2CppObject>(object)))
            {
                Il2CppObject instance =
                    reinterpret_cast<Il2CppObject>(object);

                switch (type)
                {
                case VarCtl::ValueType::Float:
                {
                    float value = originalFloat;

                    api.field_set_value(
                        instance,
                        field,
                        &value
                    );

                    break;
                }

                case VarCtl::ValueType::Int:
                {
                    int value = originalInt;

                    api.field_set_value(
                        instance,
                        field,
                        &value
                    );

                    break;
                }

                case VarCtl::ValueType::Bool:
                {
                    bool value = originalBool;

                    api.field_set_value(
                        instance,
                        field,
                        &value
                    );

                    break;
                }

                case VarCtl::ValueType::Vector3:
                {
                    VarCtl::Vector3 value =
                        originalVector3;

                    api.field_set_value(
                        instance,
                        field,
                        &value
                    );

                    break;
                }
                }
            }
        }
    }

    object = nullptr;
    field = nullptr;
    gameObject = nullptr;
    transform = nullptr;

    found = false;
    originalCached = false;
    touched = false;
    ready = false;

    active.store(
        false,
        std::memory_order_relaxed
    );
}

