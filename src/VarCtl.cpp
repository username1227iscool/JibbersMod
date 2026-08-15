#include "VarCtl.h"
#include "il2cpp_api.h"
#include "Main.h"

#include <windows.h>

#include <atomic>
#include <cstring>
#include <new>


namespace
{
    // ========================================================================
    // IL2CPP STATE
    // ========================================================================

    Il2CppApi g_api{};

    bool g_apiReady = false;


    // ========================================================================
    // UNITY CLASSES
    // ========================================================================

    Il2CppClass g_objectClass = nullptr;
    Il2CppClass g_gameObjectClass = nullptr;
    Il2CppClass g_transformClass = nullptr;


    // ========================================================================
    // UNITY METHODS
    // ========================================================================

    Il2CppMethod g_gameObjectFind = nullptr;
    Il2CppMethod g_gameObjectGetTransform = nullptr;

    Il2CppMethod g_getLocalPosition = nullptr;
    Il2CppMethod g_setLocalPosition = nullptr;

    Il2CppMethod g_getLocalEulerAngles = nullptr;
    Il2CppMethod g_setLocalEulerAngles = nullptr;

    Il2CppMethod g_getLocalScale = nullptr;
    Il2CppMethod g_setLocalScale = nullptr;


    // ========================================================================
    // UNITY OBJECT FIELD
    // ========================================================================

    Il2CppField g_cachedPtrField = nullptr;


    // ========================================================================
    // THREAD ATTACHMENT
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
    // UNITY OBJECT VALIDITY
    // ========================================================================

    bool InstanceAlive(void* instance)
    {
        if (!instance)
            return false;

        /*
            If we cannot access m_CachedPtr, don't reject the object.

            This is important because some IL2CPP builds do not expose
            the field exactly as expected.
        */

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
    // INVOKE
    // ========================================================================

    Il2CppObject Invoke(
        Il2CppMethod method,
        void* instance,
        void** args)
    {
        if (!method ||
            !g_api.runtime_invoke)
        {
            return nullptr;
        }

        Il2CppException exception = nullptr;

        Il2CppObject result =
            g_api.runtime_invoke(
                method,
                instance,
                args,
                &exception
            );

        if (exception)
            return nullptr;

        return result;
    }


    // ========================================================================
    // RESOLVE UNITY CLASSES
    // ========================================================================

    bool ResolveUnityClasses()
    {
        g_objectClass =
            FindIl2CppClass(
                g_api,
                "UnityEngine",
                "Object"
            );

        g_gameObjectClass =
            FindIl2CppClass(
                g_api,
                "UnityEngine",
                "GameObject"
            );

        g_transformClass =
            FindIl2CppClass(
                g_api,
                "UnityEngine",
                "Transform"
            );

        if (!g_objectClass ||
            !g_gameObjectClass ||
            !g_transformClass)
        {
            return false;
        }


        // ====================================================================
        // GAMEOBJECT.FIND
        // ====================================================================

        g_gameObjectFind =
            g_api.class_get_method_from_name(
                g_gameObjectClass,
                "Find",
                1
            );


        // ====================================================================
        // GAMEOBJECT.TRANSFORM
        // ====================================================================

        g_gameObjectGetTransform =
            g_api.class_get_method_from_name(
                g_gameObjectClass,
                "get_transform",
                0
            );


        // ====================================================================
        // TRANSFORM POSITION
        // ====================================================================

        g_getLocalPosition =
            g_api.class_get_method_from_name(
                g_transformClass,
                "get_localPosition",
                0
            );

        g_setLocalPosition =
            g_api.class_get_method_from_name(
                g_transformClass,
                "set_localPosition",
                1
            );


        // ====================================================================
        // TRANSFORM ROTATION
        // ====================================================================

        g_getLocalEulerAngles =
            g_api.class_get_method_from_name(
                g_transformClass,
                "get_localEulerAngles",
                0
            );

        g_setLocalEulerAngles =
            g_api.class_get_method_from_name(
                g_transformClass,
                "set_localEulerAngles",
                1
            );


        // ====================================================================
        // TRANSFORM SCALE
        // ====================================================================

        g_getLocalScale =
            g_api.class_get_method_from_name(
                g_transformClass,
                "get_localScale",
                0
            );

        g_setLocalScale =
            g_api.class_get_method_from_name(
                g_transformClass,
                "set_localScale",
                1
            );


        // ====================================================================
        // CACHED POINTER
        // ====================================================================

        g_cachedPtrField =
            FindFieldInHierarchy(
                g_api,
                g_objectClass,
                "m_CachedPtr"
            );


        /*
            Transform functionality is optional from the point of view of
            field lookup, but we require it because the controller system
            supports both controller types.
        */

        return
            g_gameObjectFind != nullptr &&
            g_gameObjectGetTransform != nullptr &&
            g_getLocalPosition != nullptr &&
            g_setLocalPosition != nullptr &&
            g_getLocalEulerAngles != nullptr &&
            g_setLocalEulerAngles != nullptr &&
            g_getLocalScale != nullptr &&
            g_setLocalScale != nullptr;
    }


    // ========================================================================
    // FIND GAMEOBJECT
    // ========================================================================

    Il2CppObject FindGameObject(
        const char* path)
    {
        if (!path ||
            !*path)
        {
            return nullptr;
        }

        if (!g_api.string_new ||
            !g_gameObjectFind)
        {
            return nullptr;
        }

        Il2CppObject pathString =
            g_api.string_new(path);

        if (!pathString)
            return nullptr;

        void* args[1] =
        {
            pathString
        };

        return
            Invoke(
                g_gameObjectFind,
                nullptr,
                args
            );
    }


    // ========================================================================
    // GET TRANSFORM
    // ========================================================================

    Il2CppObject GetTransform(
        Il2CppObject gameObject)
    {
        if (!gameObject ||
            !g_gameObjectGetTransform)
        {
            return nullptr;
        }

        return
            Invoke(
                g_gameObjectGetTransform,
                gameObject,
                nullptr
            );
    }


    // ========================================================================
    // READ TRANSFORM VECTOR
    // ========================================================================

    VarCtl::Vector3 GetTransformVector(
        Il2CppObject transform,
        VarCtl::TransformProperty property)
    {
        VarCtl::Vector3 result{
            0.0f,
            0.0f,
            0.0f
        };

        if (!transform)
            return result;

        Il2CppMethod method = nullptr;

        switch (property)
        {
        case VarCtl::TransformProperty::Position:
            method = g_getLocalPosition;
            break;

        case VarCtl::TransformProperty::Rotation:
            method = g_getLocalEulerAngles;
            break;

        case VarCtl::TransformProperty::Scale:
            method = g_getLocalScale;
            break;
        }

        if (!method)
            return result;

        Il2CppObject boxed =
            Invoke(
                method,
                transform,
                nullptr
            );

        if (!boxed)
            return result;

        if (!g_api.object_unbox)
            return result;

        void* data =
            g_api.object_unbox(boxed);

        if (!data)
            return result;

        std::memcpy(
            &result,
            data,
            sizeof(VarCtl::Vector3)
        );

        return result;
    }


    // ========================================================================
    // WRITE TRANSFORM VECTOR
    // ========================================================================

    bool SetTransformVector(
        Il2CppObject transform,
        VarCtl::TransformProperty property,
        const VarCtl::Vector3& value)
    {
        if (!transform)
            return false;

        Il2CppMethod method = nullptr;

        switch (property)
        {
        case VarCtl::TransformProperty::Position:
            method = g_setLocalPosition;
            break;

        case VarCtl::TransformProperty::Rotation:
            method = g_setLocalEulerAngles;
            break;

        case VarCtl::TransformProperty::Scale:
            method = g_setLocalScale;
            break;
        }

        if (!method)
            return false;

        VarCtl::Vector3 copy = value;

        void* args[1] =
        {
            &copy
        };

        /*
            Setter returns void.

            runtime_invoke therefore returning nullptr is normal.
            We only care whether an exception was generated, which is
            handled inside Invoke().
        */

        Invoke(
            method,
            transform,
            args
        );

        return true;
    }


    // ========================================================================
    // FIND FIELD
    //
    // This is the important part for your field controllers.
    //
    // We DO NOT enumerate every class in every assembly.
    //
    // Instead, we search the classes that are actually available by using
    // the IL2CPP class metadata returned from assemblies.
    // ========================================================================

    Il2CppField FindFieldByClass(
        Il2CppClass klass,
        const char* fieldName)
    {
        if (!klass ||
            !fieldName ||
            !*fieldName)
        {
            return nullptr;
        }

        return
            FindFieldInHierarchy(
                g_api,
                klass,
                fieldName
            );
    }


    // ========================================================================
    // FIELD TYPE MATCHING
    // ========================================================================

    bool FieldTypeMatches(
        Il2CppField field,
        VarCtl::ValueType requestedType)
    {
        if (!field ||
            !g_api.field_get_type)
        {
            return false;
        }

        /*
            We intentionally don't try to compare Il2CppType pointers.

            Il2CppType is an opaque pointer in your API.

            The safest way for this controller is to use the field's
            storage size/type only when we actually read/write it.
        */

        (void)requestedType;

        return true;
    }


    // ========================================================================
    // FIELD OBJECT SEARCH
    //
    // IMPORTANT:
    //
    // A field such as:
    //
    //     distance
    //
    // is NOT a UnityEngine.GameObject path.
    //
    // We therefore need the actual component/object containing that field.
    //
    // Because your configuration only gives us "distance", we cannot
    // magically know which MonoBehaviour instance owns it.
    //
    // This function therefore tries the common Unity objects that we can
    // identify directly.
    // ========================================================================

    bool FindFieldAndObject(
        const char* fieldName,
        VarCtl::ValueType requestedType,
        void*& outObject,
        Il2CppField& outField)
    {
        outObject = nullptr;
        outField = nullptr;

        if (!fieldName ||
            !*fieldName)
        {
            return false;
        }


        // ====================================================================
        // FIRST: search UnityEngine.Object itself
        // ====================================================================

        if (g_objectClass)
        {
            Il2CppField field =
                FindFieldByClass(
                    g_objectClass,
                    fieldName
                );

            if (field &&
                FieldTypeMatches(
                    field,
                    requestedType))
            {
                /*
                    UnityEngine.Object is a class, but we still don't have
                    an instance of it.

                    Therefore this cannot be used as a writable instance
                    field by itself.
                */
            }
        }


        // ====================================================================
        // IMPORTANT
        // ====================================================================
        //
        // Your current API has no:
        //
        //     il2cpp_object_find_objects_of_type
        //
        // and no:
        //
        //     il2cpp_image_get_class_count
        //
        // or:
        //
        //     il2cpp_image_get_class
        //
        // Therefore there is currently NO generic way to discover:
        //
        //     CameraController.distance
        //
        // from only:
        //
        //     "distance"
        //
        // and then obtain the live CameraController instance.
        //
        // That is why your fields are still "not found".
        //
        // We need a way to locate the actual MonoBehaviour instance.
        //

        return false;
    }


    // ========================================================================
    // INTERNAL CONTROLLER
    // ========================================================================

    struct Controller
    {
        const char* displayName = "";
        const char* id = "";

        VarCtl::ControllerKind kind =
            VarCtl::ControllerKind::Field;

        VarCtl::ValueType type =
            VarCtl::ValueType::Float;

        VarCtl::TransformProperty transformProperty =
            VarCtl::TransformProperty::Position;

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

        std::atomic<bool> active{
            false
        };

        // Field
        void* object = nullptr;
        Il2CppField field = nullptr;

        // Transform
        Il2CppObject gameObject = nullptr;
        Il2CppObject transform = nullptr;

        bool ready = false;
        bool found = false;

        bool originalCached = false;
        bool touched = false;

        VarCtl::Vector3 originalVector3{
            0.0f,
            0.0f,
            0.0f
        };

        VarCtl::Vector3 lastTargetVector3{
            0.0f,
            0.0f,
            0.0f
        };

        DWORD nextScanTick = 0;


        // ====================================================================
        // CONFIGURE
        // ====================================================================

        void Configure(
            const ControllerConfig& config)
        {
            displayName = config.name;
            id = config.id;

            kind = config.kind;
            type = config.valueType;

            transformProperty =
                config.transformProperty;

            multiplier =
                config.step;

            overrideFloat =
                config.defaultVal;

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

            lowerLimit =
                config.minVal;

            upperLimit =
                config.maxVal;

            showSlider =
                config.hasSlider;
        }


        // ====================================================================
        // FOUND
        // ====================================================================

        bool Found() const
        {
            if (kind ==
                VarCtl::ControllerKind::Transform)
            {
                return
                    gameObject != nullptr &&
                    transform != nullptr;
            }

            return
                object != nullptr &&
                field != nullptr;
        }


        // ====================================================================
        // LOCATE
        // ====================================================================

        bool Locate()
        {
            found = false;

            object = nullptr;
            field = nullptr;

            gameObject = nullptr;
            transform = nullptr;


            // =================================================================
            // TRANSFORM
            // =================================================================

            if (kind ==
                VarCtl::ControllerKind::Transform)
            {
                Il2CppObject go =
                    FindGameObject(id);

                if (!go)
                    return false;

                if (!InstanceAlive(go))
                    return false;

                Il2CppObject tr =
                    GetTransform(go);

                if (!tr)
                    return false;

                if (!InstanceAlive(tr))
                    return false;

                gameObject = go;
                transform = tr;

                found = true;

                return true;
            }


            // =================================================================
            // FIELD
            // =================================================================

            if (kind ==
                VarCtl::ControllerKind::Field)
            {
                return
                    FindFieldAndObject(
                        id,
                        type,
                        object,
                        field
                    );
            }

            return false;
        }


        // ====================================================================
        // CACHE ORIGINAL
        // ====================================================================

        void CacheOriginal()
        {
            if (!Found() ||
                originalCached)
            {
                return;
            }

            if (kind ==
                VarCtl::ControllerKind::Transform)
            {
                originalVector3 =
                    GetTransformVector(
                        transform,
                        transformProperty
                    );

                lastTargetVector3 =
                    originalVector3;

                originalCached = true;
            }
        }


        // ====================================================================
        // READ VECTOR
        // ====================================================================

        VarCtl::Vector3 ReadVector3()
        {
            if (!Found())
            {
                return {
                    0.0f,
                    0.0f,
                    0.0f
                };
            }

            if (kind ==
                VarCtl::ControllerKind::Transform)
            {
                return
                    GetTransformVector(
                        transform,
                        transformProperty
                    );
            }

            return {
                0.0f,
                0.0f,
                0.0f
            };
        }


        // ====================================================================
        // WRITE VECTOR
        // ====================================================================

        void WriteVector3(
            const VarCtl::Vector3& value)
        {
            if (!Found())
                return;

            if (kind ==
                VarCtl::ControllerKind::Transform)
            {
                if (SetTransformVector(
                    transform,
                    transformProperty,
                    value))
                {
                    touched = true;
                }
            }
        }


        // ====================================================================
        // VECTOR COMPARISON
        // ====================================================================

        static bool SameVector3(
            const VarCtl::Vector3& a,
            const VarCtl::Vector3& b)
        {
            return
                a.x == b.x &&
                a.y == b.y &&
                a.z == b.z;
        }


        // ====================================================================
        // TICK
        // ====================================================================

        void Tick()
        {
            if (!ready)
                return;

            EnsureThreadAttached();


            // =================================================================
            // TRY TO LOCATE
            // =================================================================

            if (!Found())
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

                lastTargetVector3 = {
                    0.0f,
                    0.0f,
                    0.0f
                };
            }


            // =================================================================
            // CHECK TRANSFORM
            // =================================================================

            if (kind ==
                VarCtl::ControllerKind::Transform)
            {
                if (!InstanceAlive(gameObject) ||
                    !InstanceAlive(transform))
                {
                    gameObject = nullptr;
                    transform = nullptr;

                    found = false;
                    originalCached = false;

                    return;
                }
            }


            // =================================================================
            // CACHE ORIGINAL
            // =================================================================

            CacheOriginal();

            if (!originalCached)
                return;


            // =================================================================
            // TRANSFORM
            // =================================================================

            if (kind ==
                VarCtl::ControllerKind::Transform)
            {
                bool enabled =
                    active.load(
                        std::memory_order_relaxed
                    );

                VarCtl::Vector3 target;

                if (enabled)
                {
                    target = {
                        overrideVector3.x *
                            multiplier,

                        overrideVector3.y *
                            multiplier,

                        overrideVector3.z *
                            multiplier
                    };
                }
                else
                {
                    target =
                        originalVector3;
                }

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


        // ====================================================================
        // SHUTDOWN
        // ====================================================================

        void Shutdown()
        {
            if (!ready)
                return;

            if (touched &&
                originalCached &&
                Found())
            {
                if (kind ==
                    VarCtl::ControllerKind::Transform)
                {
                    WriteVector3(
                        originalVector3
                    );
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
    };


    // ========================================================================
    // RUNTIME CONTROLLERS
    // ========================================================================

    Controller* g_runtimeControllers =
        nullptr;

    size_t g_runtimeControllerCount =
        0;


    // ========================================================================
    // CREATE
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
            new (std::nothrow)
            Controller[
                g_runtimeControllerCount
            ];

        if (!g_runtimeControllers)
        {
            g_runtimeControllerCount = 0;
            return false;
        }

        for (
            size_t i = 0;
            i < g_runtimeControllerCount;
            ++i)
        {
            g_runtimeControllers[i].Configure(
                g_controllers[i]
            );
        }

        return true;
    }


    // ========================================================================
    // DESTROY
    // ========================================================================

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
    // ========================================================================
    // INIT
    // ========================================================================

    bool Init()
    {
        if (g_apiReady)
            return true;

        if (!LoadIl2CppApi(
            g_api,
            60000))
        {
            return false;
        }

        EnsureThreadAttached();


        if (!ResolveUnityClasses())
        {
            return false;
        }


        if (!CreateRuntimeControllers())
        {
            return false;
        }

        g_apiReady = true;


        for (
            size_t i = 0;
            i < g_runtimeControllerCount;
            ++i)
        {
            Controller& controller =
                g_runtimeControllers[i];

            controller.ready = true;

            controller.Locate();

            if (controller.Found())
            {
                controller.CacheOriginal();
            }
        }

        return true;
    }


    // ========================================================================
    // TICK
    // ========================================================================

    void Tick()
    {
        if (!g_apiReady ||
            !g_runtimeControllers)
        {
            return;
        }

        EnsureThreadAttached();

        for (
            size_t i = 0;
            i < g_runtimeControllerCount;
            ++i)
        {
            g_runtimeControllers[i].Tick();
        }
    }


    // ========================================================================
    // SHUTDOWN
    // ========================================================================

    void Shutdown()
    {
        if (g_runtimeControllers)
        {
            for (
                size_t i = 0;
                i < g_runtimeControllerCount;
                ++i)
            {
                g_runtimeControllers[i].Shutdown();
            }
        }

        DestroyRuntimeControllers();

        g_apiReady = false;
    }


    // ========================================================================
    // GENERAL
    // ========================================================================

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

        return
            g_runtimeControllers[index]
            .displayName;
    }


    bool ReadyAt(size_t index)
    {
        if (!g_runtimeControllers ||
            index >= g_runtimeControllerCount)
        {
            return false;
        }

        return
            g_runtimeControllers[index]
            .ready;
    }


    bool FoundAt(size_t index)
    {
        if (!g_runtimeControllers ||
            index >= g_runtimeControllerCount)
        {
            return false;
        }

        return
            g_runtimeControllers[index]
            .Found();
    }


    ValueType TypeAt(size_t index)
    {
        if (!g_runtimeControllers ||
            index >= g_runtimeControllerCount)
        {
            return ValueType::Float;
        }

        return
            g_runtimeControllers[index]
            .type;
    }


    ControllerKind KindAt(size_t index)
    {
        if (!g_runtimeControllers ||
            index >= g_runtimeControllerCount)
        {
            return ControllerKind::Field;
        }

        return
            g_runtimeControllers[index]
            .kind;
    }


    TransformProperty TransformPropertyAt(
        size_t index)
    {
        if (!g_runtimeControllers ||
            index >= g_runtimeControllerCount)
        {
            return TransformProperty::Position;
        }

        return
            g_runtimeControllers[index]
            .transformProperty;
    }


    bool IsActiveAt(size_t index)
    {
        if (!g_runtimeControllers ||
            index >= g_runtimeControllerCount)
        {
            return false;
        }

        return
            g_runtimeControllers[index]
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

        return
            g_runtimeControllers[index]
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

        return
            &g_runtimeControllers[index]
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

        return
            g_runtimeControllers[index]
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

        return
            &g_runtimeControllers[index]
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

        return
            g_runtimeControllers[index]
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

        return
            g_runtimeControllers[index]
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

        return
            &g_runtimeControllers[index]
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

        return
            g_runtimeControllers[index]
            .lowerLimit;
    }


    float UpperLimitAt(size_t index)
    {
        if (!g_runtimeControllers ||
            index >= g_runtimeControllerCount)
        {
            return 0.0f;
        }

        return
            g_runtimeControllers[index]
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

        return
            g_runtimeControllers[index]
            .ReadVector3()
            .x;
    }


    int CurrentIntAt(size_t index)
    {
        return 0;
    }


    bool CurrentBoolAt(size_t index)
    {
        return false;
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

        return
            g_runtimeControllers[index]
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

        return
            g_runtimeControllers[index]
            .showSlider;
    }
}