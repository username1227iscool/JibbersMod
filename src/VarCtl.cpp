#include "VarCtl.h"
#include "il2cpp_api.h"
#include "Main.h"

#include <windows.h>
#include <cstring>
#include <atomic>
#include <cstddef>

namespace
{
    // ========================================================================
    // IL2CPP STATE
    // ========================================================================

    Il2CppApi g_api{};

    bool g_apiReady = false;

    Il2CppClass g_monoBehaviourClass = nullptr;
    Il2CppMethod g_findObjectsOfType = nullptr;
    Il2CppField g_cachedPtrField = nullptr;


    // ========================================================================
    // THREAD ATTACH
    // ========================================================================

    void EnsureThreadAttached()
    {
        thread_local bool attached = false;

        if (attached)
            return;

        if (!g_api.thread_attach ||
            !g_api.domain_get)
        {
            return;
        }

        if (g_api.thread_current &&
            g_api.thread_current())
        {
            attached = true;
            return;
        }

        Il2CppDomain domain =
            g_api.domain_get();

        if (!domain)
            return;

        g_api.thread_attach(domain);

        attached = true;
    }


    // ========================================================================
    // UNITY OBJECT CHECK
    // ========================================================================

    bool InstanceAlive(void* instance)
    {
        if (!instance)
            return false;

        // If we don't have m_CachedPtr, don't try to dereference
        // anything else. The pointer itself is all we can safely check.
        if (!g_cachedPtrField ||
            !g_api.field_get_value)
        {
            return true;
        }

        void* cachedPtr = nullptr;

        g_api.field_get_value(
            reinterpret_cast<Il2CppObject>(instance),
            g_cachedPtrField,
            &cachedPtr
        );

        return cachedPtr != nullptr;
    }


    // ========================================================================
    // FIND OBJECTS OF TYPE
    // ========================================================================

    Il2CppMethod FindObjectsOfTypeMethod(
        Il2CppClass unityObjectClass)
    {
        if (!unityObjectClass ||
            !g_api.class_get_method_from_name)
        {
            return nullptr;
        }

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

        const Il2CppType* type =
            g_api.field_get_type(field);

        if (!type)
            return false;

        Il2CppClass fieldClass =
            g_api.class_from_type(type);

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
    // FIND FIELD + OBJECT
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

        if (!g_api.class_get_type ||
            !g_api.type_get_object ||
            !g_api.runtime_invoke ||
            !g_api.array_length)
        {
            return false;
        }

        const Il2CppType* monoType =
            g_api.class_get_type(
                g_monoBehaviourClass
            );

        if (!monoType)
            return false;

        Il2CppReflectionType* reflectionType =
            g_api.type_get_object(
                monoType
            );

        if (!reflectionType)
            return false;

        void* args[1] =
        {
            reflectionType
        };

        Il2CppException exception = nullptr;

        Il2CppObject result =
            g_api.runtime_invoke(
                g_findObjectsOfType,
                nullptr,
                args,
                &exception
            );

        if (exception || !result)
            return false;

        uint32_t count =
            g_api.array_length(result);

        void** elements =
            ArrayElements(result);

        if (!elements)
            return false;

        for (uint32_t i = 0; i < count; ++i)
        {
            void* instance =
                elements[i];

            if (!instance)
                continue;

            if (!InstanceAlive(instance))
                continue;

            Il2CppClass instanceClass =
                g_api.object_get_class(
                    reinterpret_cast<Il2CppObject>(
                        instance
                        )
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
    // INTERNAL CONTROLLER
    //
    // IMPORTANT:
    //
    // This is NOT ControllerConfig.
    //
    // ControllerConfig lives in Main.cpp and only contains configuration.
    // Controller contains runtime state and IL2CPP pointers.
    // ========================================================================

    struct Controller
    {
        const char* displayName = "";
        const char* fieldName = "";

        VarCtl::ValueType type =
            VarCtl::ValueType::Float;

        float multiplier = 1.0f;

        float overrideFloat = 0.0f;
        int overrideInt = 0;
        bool overrideBool = false;

        VarCtl::Vector3 overrideVector3{
            0.0f,
            0.0f,
            0.0f
        };

        float lowerLimit = 0.0f;
        float upperLimit = 1.0f;

        bool showSlider = false;

        // Runtime state

        std::atomic<bool> active{
            false
        };

        void* object = nullptr;
        Il2CppField field = nullptr;

        bool ready = false;
        bool originalCached = false;
        bool touched = false;

        float originalFloat = 0.0f;
        int originalInt = 0;
        bool originalBool = false;

        VarCtl::Vector3 originalVector3{
            0.0f,
            0.0f,
            0.0f
        };

        float lastTargetFloat = 0.0f;
        int lastTargetInt = 0;
        bool lastTargetBool = false;

        VarCtl::Vector3 lastTargetVector3{
            0.0f,
            0.0f,
            0.0f
        };

        DWORD nextScanTick = 0;


        bool Found() const
        {
            return object != nullptr &&
                field != nullptr;
        }


        void Configure(
            const ControllerConfig& config)
        {
            displayName = config.name;
            fieldName = config.id;
            type = config.valueType;

            multiplier = config.step;

            overrideFloat = config.defaultVal;
            overrideInt =
                static_cast<int>(
                    config.defaultVal
                    );

            overrideBool =
                config.isBool;

            overrideVector3 = {
                config.vectorOverride[0],
                config.vectorOverride[1],
                config.vectorOverride[2]
            };

            lowerLimit = config.minVal;
            upperLimit = config.maxVal;

            showSlider = config.hasSlider;
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
                    reinterpret_cast<Il2CppObject>(
                        object
                        ),
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
                    reinterpret_cast<Il2CppObject>(
                        object
                        ),
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
                    reinterpret_cast<Il2CppObject>(
                        object
                        ),
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
                    reinterpret_cast<Il2CppObject>(
                        object
                        ),
                    field,
                    &value
                );

                originalVector3 = value;
                break;
            }
            }

            originalCached = true;
        }


        float Read()
        {
            if (!Found())
                return 0.0f;

            if (type ==
                VarCtl::ValueType::Float)
            {
                float value = 0.0f;

                g_api.field_get_value(
                    reinterpret_cast<Il2CppObject>(
                        object
                        ),
                    field,
                    &value
                );

                return value;
            }

            if (type ==
                VarCtl::ValueType::Int)
            {
                int value = 0;

                g_api.field_get_value(
                    reinterpret_cast<Il2CppObject>(
                        object
                        ),
                    field,
                    &value
                );

                return static_cast<float>(
                    value
                    );
            }

            if (type ==
                VarCtl::ValueType::Bool)
            {
                bool value = false;

                g_api.field_get_value(
                    reinterpret_cast<Il2CppObject>(
                        object
                        ),
                    field,
                    &value
                );

                return value ? 1.0f : 0.0f;
            }

            if (type ==
                VarCtl::ValueType::Vector3)
            {
                VarCtl::Vector3 value{};

                g_api.field_get_value(
                    reinterpret_cast<Il2CppObject>(
                        object
                        ),
                    field,
                    &value
                );

                return value.x;
            }

            return 0.0f;
        }


        int ReadInt()
        {
            if (!Found() ||
                type != VarCtl::ValueType::Int)
            {
                return 0;
            }

            int value = 0;

            g_api.field_get_value(
                reinterpret_cast<Il2CppObject>(
                    object
                    ),
                field,
                &value
            );

            return value;
        }


        bool ReadBool()
        {
            if (!Found() ||
                type != VarCtl::ValueType::Bool)
            {
                return false;
            }

            bool value = false;

            g_api.field_get_value(
                reinterpret_cast<Il2CppObject>(
                    object
                    ),
                field,
                &value
            );

            return value;
        }


        VarCtl::Vector3 ReadVector3()
        {
            if (!Found() ||
                type != VarCtl::ValueType::Vector3)
            {
                return {
                    0.0f,
                    0.0f,
                    0.0f
                };
            }

            VarCtl::Vector3 value{};

            g_api.field_get_value(
                reinterpret_cast<Il2CppObject>(
                    object
                    ),
                field,
                &value
            );

            return value;
        }


        void WriteFloat(float value)
        {
            if (!Found())
                return;

            g_api.field_set_value(
                reinterpret_cast<Il2CppObject>(
                    object
                    ),
                field,
                &value
            );

            touched = true;
        }


        void WriteInt(int value)
        {
            if (!Found())
                return;

            g_api.field_set_value(
                reinterpret_cast<Il2CppObject>(
                    object
                    ),
                field,
                &value
            );

            touched = true;
        }


        void WriteBool(bool value)
        {
            if (!Found())
                return;

            g_api.field_set_value(
                reinterpret_cast<Il2CppObject>(
                    object
                    ),
                field,
                &value
            );

            touched = true;
        }


        void WriteVector3(
            const VarCtl::Vector3& value)
        {
            if (!Found())
                return;

            VarCtl::Vector3 copy = value;

            g_api.field_set_value(
                reinterpret_cast<Il2CppObject>(
                    object
                    ),
                field,
                &copy
            );

            touched = true;
        }


        static bool SameVector3(
            const VarCtl::Vector3& a,
            const VarCtl::Vector3& b)
        {
            return
                a.x == b.x &&
                a.y == b.y &&
                a.z == b.z;
        }


        void Tick()
        {
            if (!ready)
                return;

            EnsureThreadAttached();

            if (!Found() ||
                !InstanceAlive(object))
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

                lastTargetVector3 = {
                    0.0f,
                    0.0f,
                    0.0f
                };
            }

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

            if (type ==
                VarCtl::ValueType::Float)
            {
                float target =
                    enabled
                    ? overrideFloat * multiplier
                    : originalFloat;

                if (target != lastTargetFloat)
                {
                    WriteFloat(target);

                    lastTargetFloat =
                        target;
                }

                return;
            }


            // ---------------------------------------------------------------
            // INT
            // ---------------------------------------------------------------

            if (type ==
                VarCtl::ValueType::Int)
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

                    lastTargetInt =
                        target;
                }

                return;
            }


            // ---------------------------------------------------------------
            // BOOL
            // ---------------------------------------------------------------

            if (type ==
                VarCtl::ValueType::Bool)
            {
                bool target =
                    enabled
                    ? overrideBool
                    : originalBool;

                if (target != lastTargetBool)
                {
                    WriteBool(target);

                    lastTargetBool =
                        target;
                }

                return;
            }


            // ---------------------------------------------------------------
            // VECTOR3
            // ---------------------------------------------------------------

            if (type ==
                VarCtl::ValueType::Vector3)
            {
                VarCtl::Vector3 target =
                    enabled
                    ? VarCtl::Vector3{
                        overrideVector3.x *
                            multiplier,

                        overrideVector3.y *
                            multiplier,

                        overrideVector3.z *
                            multiplier
                }
                : originalVector3;

                if (!SameVector3(
                    target,
                    lastTargetVector3))
                {
                    WriteVector3(target);

                    lastTargetVector3 =
                        target;
                }

                return;
            }
        }


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
    // RUNTIME CONTROLLERS
    //
    // This is the important part.
    //
    // These are Controller objects, NOT ControllerConfig objects.
    // ========================================================================

    Controller* g_runtimeControllers =
        nullptr;

    size_t g_runtimeControllerCount =
        0;


    // ========================================================================
    // CREATE RUNTIME CONTROLLERS
    // ========================================================================

    bool CreateRuntimeControllers()
    {
        if (g_runtimeControllers)
            return true;

        if (g_controllers_count == 0)
            return false;

        g_runtimeControllerCount =
            g_controllers_count;

        g_runtimeControllers =
            new Controller[
                g_runtimeControllerCount
            ];

        if (!g_runtimeControllers)
        {
            g_runtimeControllerCount = 0;
            return false;
        }

        for (size_t i = 0;
            i < g_runtimeControllerCount;
            ++i)
        {
            g_runtimeControllers[i].Configure(
                g_controllers[i]
            );
        }

        return true;
    }


    void DestroyRuntimeControllers()
    {
        delete[] g_runtimeControllers;

        g_runtimeControllers = nullptr;
        g_runtimeControllerCount = 0;
    }
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
            if (!LoadIl2CppApi(
                g_api,
                60000))
            {
                return false;
            }

            EnsureThreadAttached();

            if (!ResolveClasses())
                return false;

            g_apiReady = true;
        }


        // ------------------------------------------------------------
        // IMPORTANT:
        //
        // Copy ControllerConfig -> internal Controller.
        //
        // NEVER reinterpret_cast the configuration array.
        // ------------------------------------------------------------

        if (!CreateRuntimeControllers())
        {
            g_apiReady = false;
            return false;
        }


        for (size_t i = 0;
            i < g_runtimeControllerCount;
            ++i)
        {
            Controller& controller =
                g_runtimeControllers[i];

            controller.ready = true;

            controller.Locate();

            if (controller.Found())
                controller.CacheOriginal();
        }

        return true;
    }


    void Tick()
    {
        if (!g_apiReady ||
            !g_runtimeControllers)
        {
            return;
        }

        for (size_t i = 0;
            i < g_runtimeControllerCount;
            ++i)
        {
            Controller& controller =
                g_runtimeControllers[i];

            controller.Tick();
        }
    }


    void Shutdown()
    {
        if (g_runtimeControllers)
        {
            for (size_t i = 0;
                i < g_runtimeControllerCount;
                ++i)
            {
                g_runtimeControllers[i].Shutdown();
            }
        }

        DestroyRuntimeControllers();

        g_apiReady = false;
    }


    size_t Count()
    {
        return g_runtimeControllerCount;
    }


    const char* NameAt(size_t index)
    {
        if (!g_runtimeControllers ||
            index >= g_runtimeControllerCount)
        {
            return "";
        }

        return g_runtimeControllers[index].displayName;
    }


    bool ReadyAt(size_t index)
    {
        if (!g_runtimeControllers ||
            index >= g_runtimeControllerCount)
        {
            return false;
        }

        return g_runtimeControllers[index].ready;
    }


    bool FoundAt(size_t index)
    {
        if (!g_runtimeControllers ||
            index >= g_runtimeControllerCount)
        {
            return false;
        }

        return g_runtimeControllers[index].Found();
    }


    ValueType TypeAt(size_t index)
    {
        if (!g_runtimeControllers ||
            index >= g_runtimeControllerCount)
        {
            return ValueType::Float;
        }

        return g_runtimeControllers[index].type;
    }


    bool IsActiveAt(size_t index)
    {
        if (!g_runtimeControllers ||
            index >= g_runtimeControllerCount)
        {
            return false;
        }

        return g_runtimeControllers[index]
            .active.load(
                std::memory_order_relaxed
            );
    }


    void SetActiveAt(
        size_t index,
        bool value)
    {
        if (!g_runtimeControllers ||
            index >= g_runtimeControllerCount)
        {
            return;
        }

        g_runtimeControllers[index]
            .active.store(
                value,
                std::memory_order_relaxed
            );
    }


    // ========================================================================
    // FLOAT
    // ========================================================================

    float OverrideValueAt(size_t index)
    {
        if (!g_runtimeControllers ||
            index >= g_runtimeControllerCount)
        {
            return 0.0f;
        }

        return g_runtimeControllers[index]
            .overrideFloat;
    }


    void SetOverrideValueAt(
        size_t index,
        float value)
    {
        if (!g_runtimeControllers ||
            index >= g_runtimeControllerCount)
        {
            return;
        }

        g_runtimeControllers[index]
            .overrideFloat = value;
    }


    float* OverridePtrAt(size_t index)
    {
        if (!g_runtimeControllers ||
            index >= g_runtimeControllerCount)
        {
            return nullptr;
        }

        return &g_runtimeControllers[index]
            .overrideFloat;
    }


    // ========================================================================
    // INT
    // ========================================================================

    int OverrideIntAt(size_t index)
    {
        if (!g_runtimeControllers ||
            index >= g_runtimeControllerCount)
        {
            return 0;
        }

        return g_runtimeControllers[index]
            .overrideInt;
    }


    void SetOverrideIntAt(
        size_t index,
        int value)
    {
        if (!g_runtimeControllers ||
            index >= g_runtimeControllerCount)
        {
            return;
        }

        g_runtimeControllers[index]
            .overrideInt = value;
    }


    int* OverrideIntPtrAt(size_t index)
    {
        if (!g_runtimeControllers ||
            index >= g_runtimeControllerCount)
        {
            return nullptr;
        }

        return &g_runtimeControllers[index]
            .overrideInt;
    }


    // ========================================================================
    // BOOL
    // ========================================================================

    bool OverrideBoolAt(size_t index)
    {
        if (!g_runtimeControllers ||
            index >= g_runtimeControllerCount)
        {
            return false;
        }

        return g_runtimeControllers[index]
            .overrideBool;
    }


    void SetOverrideBoolAt(
        size_t index,
        bool value)
    {
        if (!g_runtimeControllers ||
            index >= g_runtimeControllerCount)
        {
            return;
        }

        g_runtimeControllers[index]
            .overrideBool = value;
    }


    // ========================================================================
    // VECTOR3
    // ========================================================================

    Vector3 OverrideVector3At(
        size_t index)
    {
        if (!g_runtimeControllers ||
            index >= g_runtimeControllerCount)
        {
            return {
                0.0f,
                0.0f,
                0.0f
            };
        }

        return g_runtimeControllers[index]
            .overrideVector3;
    }


    void SetOverrideVector3At(
        size_t index,
        Vector3 value)
    {
        if (!g_runtimeControllers ||
            index >= g_runtimeControllerCount)
        {
            return;
        }

        g_runtimeControllers[index]
            .overrideVector3 = value;
    }


    float* OverrideVector3PtrAt(
        size_t index)
    {
        if (!g_runtimeControllers ||
            index >= g_runtimeControllerCount)
        {
            return nullptr;
        }

        return &g_runtimeControllers[index]
            .overrideVector3.x;
    }


    // ========================================================================
    // LIMITS
    // ========================================================================

    float LowerLimitAt(size_t index)
    {
        if (!g_runtimeControllers ||
            index >= g_runtimeControllerCount)
        {
            return 0.0f;
        }

        return g_runtimeControllers[index]
            .lowerLimit;
    }


    float UpperLimitAt(size_t index)
    {
        if (!g_runtimeControllers ||
            index >= g_runtimeControllerCount)
        {
            return 0.0f;
        }

        return g_runtimeControllers[index]
            .upperLimit;
    }


    // ========================================================================
    // CURRENT VALUES
    // ========================================================================

    float CurrentValueAt(size_t index)
    {
        if (!g_runtimeControllers ||
            index >= g_runtimeControllerCount)
        {
            return 0.0f;
        }

        return g_runtimeControllers[index]
            .Read();
    }


    int CurrentIntAt(size_t index)
    {
        if (!g_runtimeControllers ||
            index >= g_runtimeControllerCount)
        {
            return 0;
        }

        return g_runtimeControllers[index]
            .ReadInt();
    }


    bool CurrentBoolAt(size_t index)
    {
        if (!g_runtimeControllers ||
            index >= g_runtimeControllerCount)
        {
            return false;
        }

        return g_runtimeControllers[index]
            .ReadBool();
    }


    Vector3 CurrentVector3At(size_t index)
    {
        if (!g_runtimeControllers ||
            index >= g_runtimeControllerCount)
        {
            return {
                0.0f,
                0.0f,
                0.0f
            };
        }

        return g_runtimeControllers[index]
            .ReadVector3();
    }


    // ========================================================================
    // GUI
    // ========================================================================

    bool ShowSliderAt(size_t index)
    {
        if (!g_runtimeControllers ||
            index >= g_runtimeControllerCount)
        {
            return false;
        }

        return g_runtimeControllers[index]
            .showSlider;
    }
}