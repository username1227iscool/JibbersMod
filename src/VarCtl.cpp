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

    Il2CppMethod FindObjectsOfTypeMethod(
        Il2CppClass unityObjectClass)
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

        if (!g_monoBehaviourClass ||
            !unityObjectClass)
        {
            return false;
        }

        g_findObjectsOfType =
            FindObjectsOfTypeMethod(
                unityObjectClass
            );

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
        if (!field ||
            !g_api.field_get_type ||
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

        case VarCtl::ValueType::Vector3:
            return std::strcmp(typeName, "Vector3") == 0;
        }

        return false;
    }


    // ========================================================================
    // FIND FIELD AND OBJECT
    // ========================================================================

    bool FindFieldAndObject(
        const char* fieldName,
        VarCtl::ValueType requestedType,
        void*& outObject,
        Il2CppField& outField)
    {
        outObject = nullptr;
        outField = nullptr;

        if (!g_apiReady ||
            !fieldName ||
            !g_monoBehaviourClass ||
            !g_findObjectsOfType)
        {
            return false;
        }

        const Il2CppType* monoBehaviourType =
            g_api.class_get_type(
                g_monoBehaviourClass
            );

        Il2CppReflectionType* typeObject =
            monoBehaviourType
            ? g_api.type_get_object(
                monoBehaviourType
            )
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

        for (uint32_t i = 0;
            i < length;
            ++i)
        {
            void* instance =
                elements[i];

            if (!instance)
                continue;

            Il2CppClass instanceClass =
                g_api.object_get_class(
                    instance
                );

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
                requestedType
            ))
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

        float overrideFloat;
        int overrideInt;
        bool overrideBool;

        VarCtl::Vector3 overrideVector3;

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

        VarCtl::Vector3 originalVector3{ 0.0f, 0.0f, 0.0f };

        float lastTargetFloat = 0.0f;
        int lastTargetInt = 0;
        bool lastTargetBool = false;

        VarCtl::Vector3 lastTargetVector3{ 0.0f, 0.0f, 0.0f };

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


        // --------------------------------------------------------------------
        // CACHE ORIGINAL
        // --------------------------------------------------------------------

        void CacheOriginal()
        {
            if (!Found() ||
                originalCached)
            {
                return;
            }

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

            case VarCtl::ValueType::Vector3:
            {
                VarCtl::Vector3 value{};

                g_api.field_get_value(
                    object,
                    field,
                    &value
                );

                originalVector3 = value;
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

            case VarCtl::ValueType::Vector3:
            {
                VarCtl::Vector3 value{};

                g_api.field_get_value(
                    object,
                    field,
                    &value
                );

                return value.x;
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

            if (type == VarCtl::ValueType::Int)
            {
                g_api.field_get_value(
                    object,
                    field,
                    &value
                );
            }

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

            if (type == VarCtl::ValueType::Bool)
            {
                g_api.field_get_value(
                    object,
                    field,
                    &value
                );
            }

            return value;
        }


        // --------------------------------------------------------------------
        // READ VECTOR3
        // --------------------------------------------------------------------

        VarCtl::Vector3 ReadVector3()
        {
            if (!Found())
                return { 0.0f, 0.0f, 0.0f };

            VarCtl::Vector3 value{};

            if (type == VarCtl::ValueType::Vector3)
            {
                g_api.field_get_value(
                    object,
                    field,
                    &value
                );
            }

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
        // WRITE VECTOR3
        // --------------------------------------------------------------------

        void WriteVector3(
            const VarCtl::Vector3& value)
        {
            if (!Found())
                return;

            VarCtl::Vector3 copy = value;

            g_api.field_set_value(
                object,
                field,
                &copy
            );

            touched = true;
        }


        // --------------------------------------------------------------------
        // VECTOR3 EQUALITY
        // --------------------------------------------------------------------

        static bool SameVector3(
            const VarCtl::Vector3& a,
            const VarCtl::Vector3& b)
        {
            return a.x == b.x &&
                a.y == b.y &&
                a.z == b.z;
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
            // Object disappeared or changed
            // ---------------------------------------------------------------

            if (!InstanceAlive(object) || !Found())
            {
                DWORD now = GetTickCount();

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
                lastTargetVector3 =
                { 0.0f, 0.0f, 0.0f };
            }

            // ---------------------------------------------------------------
            // Cache original value
            // ---------------------------------------------------------------

            CacheOriginal();

            if (!originalCached)
                return;

            bool enabled =
                active.load(
                    std::memory_order_relaxed
                );

            // ---------------------------------------------------------------
            // FLOAT
            // ---------------------------------------------------------------

            if (type == VarCtl::ValueType::Float)
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

                return;
            }

            // ---------------------------------------------------------------
            // INT
            // ---------------------------------------------------------------

            if (type == VarCtl::ValueType::Int)
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

                return;
            }

            // ---------------------------------------------------------------
            // BOOL
            // ---------------------------------------------------------------

            if (type == VarCtl::ValueType::Bool)
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

                return;
            }

            // ---------------------------------------------------------------
            // VECTOR3
            // ---------------------------------------------------------------

            if (type == VarCtl::ValueType::Vector3)
            {
                VarCtl::Vector3 target =
                    enabled
                    ? VarCtl::Vector3
                {
                    overrideVector3.x * multiplier,
                    overrideVector3.y * multiplier,
                    overrideVector3.z * multiplier
                }
                : originalVector3;

                if (!SameVector3(
                    target,
                    lastTargetVector3
                ))
                {
                    WriteVector3(target);
                    lastTargetVector3 = target;
                }

                return;
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

                case VarCtl::ValueType::Vector3:
                    WriteVector3(originalVector3);
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
    // CONTROLLERS
    //
    // Entry layout:
    //
    //     display name
    //     field name
    //     value type
    //     multiplier
    //     float override
    //     int override
    //     bool override
    //     vector3 override
    //     lower limit
    //     upper limit
    //     show slider
    //
    // For the type you're NOT using, leave its storage at:
    //
    //     float  = 0.0f
    //     int    = 0
    //     bool   = false
    //     vector = { 0, 0, 0 }
    //
    // ========================================================================

    Controller g_controllers[] =
    {
        // Float
        {
            "Camera Distance",
            "distance",
            VarCtl::ValueType::Float,
            1.0f,
            10.0f,
            0,
            false,
            { 0.0f, 0.0f, 0.0f },
            -1.0f,
            50.0f,
            true
        },

        // Float
        {
            "Camera Rotation Speed",
            "rotateSpeed",
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

        // Float
        {
            "Camera Rotation",
            "yAngle",
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

        // Float, no slider
        {
            "Stop Camera",
            "speed",
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

        {
            "Pop Skis",
            "skiBreakForce",
            VarCtl::ValueType::Vector3,
            1.0f,
            0.0f,
            0,
            false,
            { 0.0f, 0.0f, 0.0f },  // X, Y, Z override
            0.0f,
            100.0f,
            false   
        },




        // --------------------------------------------------------------------
        // Vector3 example
        //
        // Uncomment / edit this when you have a real Vector3 field.
        // --------------------------------------------------------------------
        //
        // {
        //     "Camera Position",
        //     "position",
        //     VarCtl::ValueType::Vector3,
        //     1.0f,
        //     0.0f,
        //     0,
        //     false,
        //     { 0.0f, 0.0f, 0.0f },
        //     -100.0f,
        //     100.0f,
        //     true
        // }
    };

    constexpr size_t g_controllerCount =
        sizeof(g_controllers) /
        sizeof(g_controllers[0]);
}


// ============================================================================
// PUBLIC API
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


    void SetOverrideValueAt(
        size_t index,
        float value)
    {
        if (index >= g_controllerCount)
            return;

        g_controllers[index].overrideFloat =
            value;
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


    void SetOverrideIntAt(
        size_t index,
        int value)
    {
        if (index >= g_controllerCount)
            return;

        g_controllers[index].overrideInt =
            value;
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


    void SetOverrideBoolAt(
        size_t index,
        bool value)
    {
        if (index >= g_controllerCount)
            return;

        g_controllers[index].overrideBool =
            value;
    }


    // ========================================================================
    // VECTOR3
    // ========================================================================

    Vector3 OverrideVector3At(size_t index)
    {
        if (index >= g_controllerCount)
            return { 0.0f, 0.0f, 0.0f };

        return g_controllers[index].overrideVector3;
    }


    void SetOverrideVector3At(
        size_t index,
        Vector3 value)
    {
        if (index >= g_controllerCount)
            return;

        g_controllers[index].overrideVector3 =
            value;
    }


    float* OverrideVector3PtrAt(size_t index)
    {
        if (index >= g_controllerCount)
            return nullptr;

        return &g_controllers[index].overrideVector3.x;
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


    Vector3 CurrentVector3At(size_t index)
    {
        if (index >= g_controllerCount)
            return { 0.0f, 0.0f, 0.0f };

        return g_controllers[index].ReadVector3();
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
