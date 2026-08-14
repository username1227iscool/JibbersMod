#include "VarCtl.h"
#include "il2cpp_api.h"

#include <windows.h>
#include <cstring>
#include <atomic>
#include <cstddef>

namespace
{
    Il2CppApi g_api{};

    bool g_apiReady = false;

    Il2CppClass g_monoBehaviourClass = nullptr;
    Il2CppMethod g_findObjectsOfType = nullptr;
    Il2CppField g_cachedPtrField = nullptr;


    // ========================================================================
    // IL2CPP THREAD ATTACH
    // ========================================================================

    void EnsureThreadAttached()
    {
        thread_local bool attached = false;

        if (attached || !g_api.thread_attach || !g_api.domain_get)
            return;

        if (g_api.thread_current && g_api.thread_current())
        {
            attached = true;
            return;
        }

        Il2CppDomain domain = g_api.domain_get();

        if (!domain)
            return;

        g_api.thread_attach(domain);
        attached = true;
    }


    // ========================================================================
    // FIND OBJECTS OF TYPE
    // ========================================================================

    Il2CppMethod FindObjectsOfTypeMethod(Il2CppClass unityObjectClass)
    {
        return g_api.class_get_method_from_name(
            unityObjectClass,
            "FindObjectsOfType",
            1
        );
    }


    // ========================================================================
    // RESOLVE UNITY CLASSES
    // ========================================================================

    bool ResolveClasses()
    {
        g_monoBehaviourClass =
            FindIl2CppClass(
                g_api,
                "UnityEngine",
                "MonoBehaviour"
            );

        Il2CppClass unityObjectClass =
            FindIl2CppClass(
                g_api,
                "UnityEngine",
                "Object"
            );

        if (!g_monoBehaviourClass || !unityObjectClass)
            return false;

        g_findObjectsOfType =
            FindObjectsOfTypeMethod(unityObjectClass);

        if (!g_findObjectsOfType)
            return false;

        g_cachedPtrField =
            FindFieldInHierarchy(
                g_api,
                unityObjectClass,
                "m_CachedPtr"
            );

        return true;
    }


    // ========================================================================
    // CHECK UNITY OBJECT
    // ========================================================================

    bool InstanceAlive(void* instance)
    {
        if (!instance)
            return false;

        if (!g_cachedPtrField)
            return true;

        void* cached = nullptr;

        g_api.field_get_value(
            instance,
            g_cachedPtrField,
            &cached
        );

        return cached != nullptr;
    }


    // ========================================================================
    // CHECK FIELD TYPE
    // ========================================================================

    bool FieldIsSupported(
        Il2CppField field,
        VarCtl::ValueType requestedType)
    {
        if (!g_api.field_get_type ||
            !g_api.class_from_type ||
            !g_api.class_get_name)
        {
            return false;
        }

        const Il2CppType* il2cppType =
            g_api.field_get_type(field);

        if (!il2cppType)
            return false;

        Il2CppClass fieldClass =
            g_api.class_from_type(il2cppType);

        if (!fieldClass)
            return false;

        const char* typeName =
            g_api.class_get_name(fieldClass);

        if (!typeName)
            return false;

        switch (requestedType)
        {
        case VarCtl::ValueType::Float:
            return std::strcmp(typeName, "Single") == 0;

        case VarCtl::ValueType::Int:
            return std::strcmp(typeName, "Int32") == 0;

        case VarCtl::ValueType::Bool:
            return std::strcmp(typeName, "Boolean") == 0;
        }

        return false;
    }


    // ========================================================================
    // FIND A FIELD ON ANY MONOBEHAVIOUR
    // ========================================================================

    bool FindFieldAndObject(
        const char* fieldName,
        VarCtl::ValueType requestedType,
        void*& outObject,
        Il2CppField& outField)
    {
        outObject = nullptr;
        outField = nullptr;

        if (!g_apiReady)
            return false;

        const Il2CppType* monoBehaviourType =
            g_api.class_get_type(g_monoBehaviourClass);

        Il2CppReflectionType* typeObject =
            monoBehaviourType
            ? g_api.type_get_object(monoBehaviourType)
            : nullptr;

        if (!typeObject)
            return false;

        void* args[1] =
        {
            typeObject
        };

        Il2CppException exc = nullptr;

        Il2CppObject array =
            g_api.runtime_invoke(
                g_findObjectsOfType,
                nullptr,
                args,
                &exc
            );

        if (exc || !array)
            return false;

        uint32_t length =
            g_api.array_length(array);

        void** elements =
            ArrayElements(array);

        if (!elements)
            return false;

        for (uint32_t i = 0; i < length; ++i)
        {
            void* instance =
                elements[i];

            if (!instance)
                continue;

            Il2CppClass instanceClass =
                g_api.object_get_class(instance);

            if (!instanceClass)
                continue;

            Il2CppField field =
                FindFieldInHierarchy(
                    g_api,
                    instanceClass,
                    fieldName
                );

            if (!field)
                continue;

            if (!FieldIsSupported(
                field,
                requestedType))
            {
                continue;
            }

            outObject = instance;
            outField = field;

            return true;
        }

        return false;
    }


    // ========================================================================
    // CONTROLLER
    // ========================================================================

    struct Controller
    {
        // --------------------------------------------------------------------
        // CONFIG
        // --------------------------------------------------------------------

        const char* displayName;
        const char* fieldName;

        VarCtl::ValueType type;

        float multiplier;

        // Type-specific override storage
        float overrideFloat;
        int overrideInt;
        bool overrideBool;

        float lowerLimit;
        float upperLimit;

        bool showSlider;

        // --------------------------------------------------------------------
        // RUNTIME
        // --------------------------------------------------------------------

        std::atomic<bool> active{ false };

        void* object = nullptr;
        Il2CppField field = nullptr;

        bool ready = false;
        bool originalCached = false;
        bool touched = false;

        float originalFloat = 0.0f;
        int originalInt = 0;
        bool originalBool = false;

        float lastTargetFloat = 0.0f;
        int lastTargetInt = 0;
        bool lastTargetBool = false;

        DWORD nextScanTick = 0;


        bool Found() const
        {
            return object != nullptr &&
                field != nullptr;
        }


        bool Locate()
        {
            object = nullptr;
            field = nullptr;

            return FindFieldAndObject(
                fieldName,
                type,
                object,
                field
            );
        }


        void CacheOriginal()
        {
            if (!Found() || originalCached)
                return;

            switch (type)
            {
            case VarCtl::ValueType::Float:
            {
                float value = 0.0f;

                g_api.field_get_value(
                    object,
                    field,
                    &value
                );

                originalFloat = value;
                break;
            }

            case VarCtl::ValueType::Int:
            {
                int value = 0;

                g_api.field_get_value(
                    object,
                    field,
                    &value
                );

                originalInt = value;
                break;
            }

            case VarCtl::ValueType::Bool:
            {
                bool value = false;

                g_api.field_get_value(
                    object,
                    field,
                    &value
                );

                originalBool = value;
                break;
            }
            }

            originalCached = true;
        }


        // --------------------------------------------------------------------
        // READ FLOAT
        // --------------------------------------------------------------------

        float Read()
        {
            if (!Found())
                return 0.0f;

            switch (type)
            {
            case VarCtl::ValueType::Float:
            {
                float value = 0.0f;

                g_api.field_get_value(
                    object,
                    field,
                    &value
                );

                return value;
            }

            case VarCtl::ValueType::Int:
            {
                int value = 0;

                g_api.field_get_value(
                    object,
                    field,
                    &value
                );

                return static_cast<float>(value);
            }

            case VarCtl::ValueType::Bool:
            {
                bool value = false;

                g_api.field_get_value(
                    object,
                    field,
                    &value
                );

                return value ? 1.0f : 0.0f;
            }
            }

            return 0.0f;
        }


        // --------------------------------------------------------------------
        // READ INT
        // --------------------------------------------------------------------

        int ReadInt()
        {
            if (!Found())
                return 0;

            int value = 0;

            g_api.field_get_value(
                object,
                field,
                &value
            );

            return value;
        }


        // --------------------------------------------------------------------
        // READ BOOL
        // --------------------------------------------------------------------

        bool ReadBool()
        {
            if (!Found())
                return false;

            bool value = false;

            g_api.field_get_value(
                object,
                field,
                &value
            );

            return value;
        }


        // --------------------------------------------------------------------
        // WRITE FLOAT
        // --------------------------------------------------------------------

        void WriteFloat(float value)
        {
            if (!Found())
                return;

            g_api.field_set_value(
                object,
                field,
                &value
            );

            touched = true;
        }


        // --------------------------------------------------------------------
        // WRITE INT
        // --------------------------------------------------------------------

        void WriteInt(int value)
        {
            if (!Found())
                return;

            g_api.field_set_value(
                object,
                field,
                &value
            );

            touched = true;
        }


        // --------------------------------------------------------------------
        // WRITE BOOL
        // --------------------------------------------------------------------

        void WriteBool(bool value)
        {
            if (!Found())
                return;

            g_api.field_set_value(
                object,
                field,
                &value
            );

            touched = true;
        }


        // --------------------------------------------------------------------
        // TICK
        // --------------------------------------------------------------------

        void Tick()
        {
            if (!ready)
                return;

            EnsureThreadAttached();

            // ---------------------------------------------------------------
            // Object disappeared / changed
            // ---------------------------------------------------------------

            if (!InstanceAlive(object) || !Found())
            {
                DWORD now =
                    GetTickCount();

                if (now < nextScanTick)
                    return;

                nextScanTick =
                    now + 1000;

                if (!Locate())
                    return;

                originalCached = false;
                touched = false;

                lastTargetFloat = 0.0f;
                lastTargetInt = 0;
                lastTargetBool = false;
            }

            // ---------------------------------------------------------------
            // Cache original value
            // ---------------------------------------------------------------

            CacheOriginal();

            if (!originalCached)
                return;

            // ---------------------------------------------------------------
            // Determine active state
            // ---------------------------------------------------------------

            bool enabled =
                active.load(
                    std::memory_order_relaxed
                );

            // ---------------------------------------------------------------
            // Apply correct type
            // ---------------------------------------------------------------

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
                    WriteFloat(target);
                    lastTargetFloat = target;
                }

                break;
            }

            case VarCtl::ValueType::Int:
            {
                int target =
                    enabled
                    ? static_cast<int>(
                        static_cast<float>(
                            overrideInt
                            ) * multiplier
                        )
                    : originalInt;

                if (target != lastTargetInt)
                {
                    WriteInt(target);
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
                    WriteBool(target);
                    lastTargetBool = target;
                }

                break;
            }
            }
        }


        // --------------------------------------------------------------------
        // SHUTDOWN
        // --------------------------------------------------------------------

        void Shutdown()
        {
            if (!ready)
                return;

            if (touched &&
                originalCached &&
                InstanceAlive(object))
            {
                switch (type)
                {
                case VarCtl::ValueType::Float:
                    WriteFloat(originalFloat);
                    break;

                case VarCtl::ValueType::Int:
                    WriteInt(originalInt);
                    break;

                case VarCtl::ValueType::Bool:
                    WriteBool(originalBool);
                    break;
                }
            }

            object = nullptr;
            field = nullptr;

            originalCached = false;
            touched = false;
            ready = false;

            active.store(
                false,
                std::memory_order_relaxed
            );
        }
    };


    // ========================================================================
    // CONTROLLER LIST
    // ========================================================================
    //
    // Layout:
    //
    // {
    //     display name,
    //     IL2CPP field name,
    //     type,
    //     multiplier,
    //     float override,
    //     int override,
    //     bool override,
    //     lower limit,
    //     upper limit,
    //     show slider
    // }
    //
    // ========================================================================

    Controller g_controllers[] =
    {//    displayName                 fieldName         Var Type                   mult  override lower  upper    slider
        { "Camera Distance",           "distance",       VarCtl::ValueType::Float,  1.0f,  10.0f,  -1.0f, 50.0f,   true }, 
        { "Camera Rotation Speed",     "rotateSpeed",    VarCtl::ValueType::Float,  1.0f,  10.0f,  0.0f,  360.0f,  true }, 
        { "Camera Rotation ",          "yAngle",         VarCtl::ValueType::Float,  1.0f,  10.0f,  0.0f,  360.0f,  true }, 
        { "Stop Camera",               "speed",          VarCtl::ValueType::Float,  1.0f,  0.0f,   0.0f,  360.0f,  false}, 
        { "Move Speed",                "moveSpeed",      VarCtl::ValueType::Float,  1.0f,  10.0f,  0.0f,  20.0f,   true }, 
        { "Ski Drag",                  "skiDrag",        VarCtl::ValueType::Float,  1.0f,  10.0f,  0.0f,  5000.0f, true }, 
        { "Enable Ski Drag",           "enableDrag",     VarCtl::ValueType::Float,  1.0f,  10.0f,  0.0f,  5000.0f, false}
    };

    constexpr size_t g_controllerCount =
        sizeof(g_controllers) /
        sizeof(g_controllers[0]);
}


// ============================================================================
// PUBLIC VarCtl API
// ============================================================================

namespace VarCtl
{
    bool Init()
    {
        if (!g_apiReady)
        {
            if (!LoadIl2CppApi(g_api, 60000))
                return false;

            EnsureThreadAttached();

            if (!ResolveClasses())
                return false;

            g_apiReady = true;
        }

        for (auto& controller : g_controllers)
        {
            controller.ready = true;

            controller.Locate();

            if (controller.Found())
                controller.CacheOriginal();
        }

        return true;
    }


    void Tick()
    {
        if (!g_apiReady)
            return;

        for (auto& controller : g_controllers)
            controller.Tick();
    }


    void Shutdown()
    {
        for (auto& controller : g_controllers)
            controller.Shutdown();

        g_apiReady = false;
    }


    size_t Count()
    {
        return g_controllerCount;
    }


    const char* NameAt(size_t index)
    {
        if (index >= g_controllerCount)
            return "";

        return g_controllers[index].displayName;
    }


    bool ReadyAt(size_t index)
    {
        if (index >= g_controllerCount)
            return false;

        return g_controllers[index].ready;
    }


    bool FoundAt(size_t index)
    {
        if (index >= g_controllerCount)
            return false;

        return g_controllers[index].Found();
    }


    ValueType TypeAt(size_t index)
    {
        if (index >= g_controllerCount)
            return ValueType::Float;

        return g_controllers[index].type;
    }


    bool IsActiveAt(size_t index)
    {
        if (index >= g_controllerCount)
            return false;

        return g_controllers[index].active.load(
            std::memory_order_relaxed
        );
    }


    void SetActiveAt(size_t index, bool value)
    {
        if (index >= g_controllerCount)
            return;

        g_controllers[index].active.store(
            value,
            std::memory_order_relaxed
        );
    }


    // ========================================================================
    // FLOAT
    // ========================================================================

    float OverrideValueAt(size_t index)
    {
        if (index >= g_controllerCount)
            return 0.0f;

        return g_controllers[index].overrideFloat;
    }


    void SetOverrideValueAt(size_t index, float value)
    {
        if (index >= g_controllerCount)
            return;

        g_controllers[index].overrideFloat = value;
    }


    float* OverridePtrAt(size_t index)
    {
        if (index >= g_controllerCount)
            return nullptr;

        return &g_controllers[index].overrideFloat;
    }


    // ========================================================================
    // INT
    // ========================================================================

    int OverrideIntAt(size_t index)
    {
        if (index >= g_controllerCount)
            return 0;

        return g_controllers[index].overrideInt;
    }


    void SetOverrideIntAt(size_t index, int value)
    {
        if (index >= g_controllerCount)
            return;

        g_controllers[index].overrideInt = value;
    }


    int* OverrideIntPtrAt(size_t index)
    {
        if (index >= g_controllerCount)
            return nullptr;

        return &g_controllers[index].overrideInt;
    }


    // ========================================================================
    // BOOL
    // ========================================================================

    bool OverrideBoolAt(size_t index)
    {
        if (index >= g_controllerCount)
            return false;

        return g_controllers[index].overrideBool;
    }


    void SetOverrideBoolAt(size_t index, bool value)
    {
        if (index >= g_controllerCount)
            return;

        g_controllers[index].overrideBool = value;
    }


    // ========================================================================
    // LIMITS
    // ========================================================================

    float LowerLimitAt(size_t index)
    {
        if (index >= g_controllerCount)
            return 0.0f;

        return g_controllers[index].lowerLimit;
    }


    float UpperLimitAt(size_t index)
    {
        if (index >= g_controllerCount)
            return 0.0f;

        return g_controllers[index].upperLimit;
    }


    // ========================================================================
    // CURRENT VALUES
    // ========================================================================

    float CurrentValueAt(size_t index)
    {
        if (index >= g_controllerCount)
            return 0.0f;

        return g_controllers[index].Read();
    }


    int CurrentIntAt(size_t index)
    {
        if (index >= g_controllerCount)
            return 0;

        return g_controllers[index].ReadInt();
    }


    bool CurrentBoolAt(size_t index)
    {
        if (index >= g_controllerCount)
            return false;

        return g_controllers[index].ReadBool();
    }


    // ========================================================================
    // GUI
    // ========================================================================

    bool ShowSliderAt(size_t index)
    {
        if (index >= g_controllerCount)
            return false;

        return g_controllers[index].showSlider;
    }
}