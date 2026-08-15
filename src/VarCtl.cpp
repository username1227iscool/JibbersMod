#include "VarCtl.h"
#include "il2cpp_api.h"
#include "Main.h"

#include <windows.h>

#include <atomic>
#include <cstring>
#include <new>
#include <string>
#include <cstdint>


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
    Il2CppClass g_monoBehaviourClass = nullptr;


    // ========================================================================
    // UNITY METHODS
    // ========================================================================

    Il2CppMethod g_gameObjectFind = nullptr;
    Il2CppMethod g_gameObjectGetTransform = nullptr;
    Il2CppMethod g_findObjectsOfType = nullptr;

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
    // POINTER SANITY
    // ========================================================================
    //
    // This does NOT prove that an arbitrary address belongs to a live IL2CPP
    // object. It only rejects obviously invalid addresses.
    //
    // The important protection against stale Unity objects is to stop
    // retaining objects indefinitely and to reacquire them when needed.
    //

    bool PointerLooksValid(
        const void* pointer)
    {
        if (!pointer)
            return false;

        uintptr_t address =
            reinterpret_cast<uintptr_t>(pointer);

        if (address < 0x10000)
            return false;

        if ((address & (sizeof(void*) - 1)) != 0)
            return false;

        return true;
    }


    // ========================================================================
    // UNITY OBJECT VALIDITY
    // ========================================================================
    //
    // IMPORTANT:
    //
    // There is no completely safe C++ way to test an arbitrary pointer
    // without touching it.
    //
    // Therefore this function is only used on pointers that came directly
    // from IL2CPP. The primary protection against stale pointers is that
    // field controllers are periodically reacquired.
    //

    bool InstanceAlive(
        Il2CppObject instance)
    {
        if (!PointerLooksValid(instance))
            return false;

        if (!g_cachedPtrField ||
            !g_api.field_get_value)
        {
            return true;
        }

        void* cachedPtr = nullptr;

        g_api.field_get_value(
            instance,
            g_cachedPtrField,
            &cachedPtr
        );

        return cachedPtr != nullptr;
    }


    // ========================================================================
    // INVOKE
    // ========================================================================
    //
    // Returns true if the method invocation itself completed without an
    // IL2CPP exception.
    //
    // A void-returning Unity method still succeeds even though its return
    // value is nullptr.
    //

    bool Invoke(
        Il2CppMethod method,
        void* instance,
        void** args,
        Il2CppObject* outResult = nullptr)
    {
        if (!method ||
            !g_api.runtime_invoke)
        {
            return false;
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
            return false;

        if (outResult)
            *outResult = result;

        return true;
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

        g_monoBehaviourClass =
            FindIl2CppClass(
                g_api,
                "UnityEngine",
                "MonoBehaviour"
            );

        if (!g_objectClass ||
            !g_gameObjectClass ||
            !g_transformClass ||
            !g_monoBehaviourClass)
        {
            return false;
        }


        // ====================================================================
        // FIND OBJECTS OF TYPE
        // ====================================================================

        g_findObjectsOfType =
            g_api.class_get_method_from_name(
                g_objectClass,
                "FindObjectsOfType",
                1
            );


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


        // ====================================================================
        // REQUIRED METHODS
        // ====================================================================

        return
            g_gameObjectFind != nullptr &&
            g_gameObjectGetTransform != nullptr &&
            g_findObjectsOfType != nullptr &&
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

        Il2CppObject result = nullptr;

        if (!Invoke(
            g_gameObjectFind,
            nullptr,
            args,
            &result))
        {
            return nullptr;
        }

        return result;
    }


    // ========================================================================
    // GET TRANSFORM
    // ========================================================================

    Il2CppObject GetTransform(
        Il2CppObject gameObject)
    {
        if (!PointerLooksValid(gameObject) ||
            !g_gameObjectGetTransform)
        {
            return nullptr;
        }

        Il2CppObject result = nullptr;

        if (!Invoke(
            g_gameObjectGetTransform,
            gameObject,
            nullptr,
            &result))
        {
            return nullptr;
        }

        return result;
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

        if (!PointerLooksValid(transform))
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

        if (!method ||
            !g_api.object_unbox)
        {
            return result;
        }

        Il2CppObject boxed = nullptr;

        if (!Invoke(
            method,
            transform,
            nullptr,
            &boxed))
        {
            return result;
        }

        if (!boxed)
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
        if (!PointerLooksValid(transform))
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

        return Invoke(
            method,
            transform,
            args,
            nullptr
        );
    }


    // ========================================================================
    // FIND FIELD BY CLASS
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

        if (!g_api.class_from_type ||
            !g_api.class_get_name)
        {
            return true;
        }

        const Il2CppType* fieldType =
            g_api.field_get_type(field);

        if (!fieldType)
            return false;

        Il2CppClass fieldClass =
            g_api.class_from_type(fieldType);

        if (!fieldClass)
            return false;

        const char* className =
            g_api.class_get_name(fieldClass);

        if (!className)
            return false;

        switch (requestedType)
        {
        case VarCtl::ValueType::Float:
            return std::strcmp(
                className,
                "Single"
            ) == 0;

        case VarCtl::ValueType::Int:
            return std::strcmp(
                className,
                "Int32"
            ) == 0;

        case VarCtl::ValueType::Bool:
            return std::strcmp(
                className,
                "Boolean"
            ) == 0;

        case VarCtl::ValueType::Vector3:
            return std::strcmp(
                className,
                "Vector3"
            ) == 0;
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

        if (!fieldName ||
            !*fieldName)
        {
            return false;
        }

        if (!g_monoBehaviourClass ||
            !g_findObjectsOfType ||
            !g_api.array_length ||
            !g_api.class_get_type ||
            !g_api.type_get_object ||
            !g_api.object_get_class)
        {
            return false;
        }


        // ====================================================================
        // GET SYSTEM.TYPE FOR MONOBEHAVIOUR
        // ====================================================================

        const Il2CppType* type =
            g_api.class_get_type(
                g_monoBehaviourClass
            );

        if (!type)
            return false;

        Il2CppReflectionType* typeObject =
            g_api.type_get_object(type);

        if (!typeObject)
            return false;


        // ====================================================================
        // FIND OBJECTS
        // ====================================================================

        void* args[1] =
        {
            typeObject
        };

        Il2CppObject array = nullptr;

        if (!Invoke(
            g_findObjectsOfType,
            nullptr,
            args,
            &array))
        {
            return false;
        }

        if (!PointerLooksValid(array))
            return false;


        // ====================================================================
        // GET ARRAY LENGTH
        // ====================================================================

        uint32_t length =
            g_api.array_length(array);

        if (length == 0)
            return false;


        // ====================================================================
        // GET ARRAY ELEMENTS
        // ====================================================================
        //
        // IMPORTANT:
        //
        // ArrayElements() has been corrected in il2cpp_api.cpp.
        //
        // The previous implementation skipped only two pointer-sized
        // values. On 64-bit IL2CPP that caused the bounds/max_length data
        // to be interpreted as object pointers.
        //

        void** elements =
            ArrayElements(array);

        if (!elements)
            return false;


        // ====================================================================
        // SCAN OBJECTS
        // ====================================================================

        for (uint32_t i = 0; i < length; ++i)
        {
            void* instance =
                elements[i];

            if (!PointerLooksValid(instance))
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
                FindFieldByClass(
                    instanceClass,
                    fieldName
                );

            if (!field)
                continue;

            if (!FieldTypeMatches(
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
    // ========================================================================

    struct Controller
    {
        // --------------------------------------------------------------------
        // IMPORTANT:
        //
        // These are std::string now instead of const char*.
        //
        // The controller owns the strings, so we don't keep pointers into
        // temporary ControllerConfig/string storage.
        // --------------------------------------------------------------------

        std::string displayName;
        std::string id;
        std::string fieldName;

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


        // --------------------------------------------------------------------
        // FIELD
        // --------------------------------------------------------------------

        void* object = nullptr;
        Il2CppField field = nullptr;


        // --------------------------------------------------------------------
        // TRANSFORM
        // --------------------------------------------------------------------

        Il2CppObject gameObject = nullptr;
        Il2CppObject transform = nullptr;


        // --------------------------------------------------------------------
        // STATE
        // --------------------------------------------------------------------

        bool ready = false;
        bool found = false;

        bool originalCached = false;
        bool touched = false;


        // --------------------------------------------------------------------
        // ORIGINAL VALUES
        // --------------------------------------------------------------------

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

        float originalFloat = 0.0f;
        int originalInt = 0;
        bool originalBool = false;

        float lastTargetFloat = 0.0f;
        int lastTargetInt = 0;
        bool lastTargetBool = false;


        // --------------------------------------------------------------------
        // SCANNING
        // --------------------------------------------------------------------

        DWORD nextScanTick = 0;


        // ====================================================================
        // CONFIGURE
        // ====================================================================

        void Configure(
            const ControllerConfig& config)
        {
            displayName =
                config.name
                ? config.name
                : "";

            id =
                config.id
                ? config.id
                : "";

            fieldName =
                config.fieldName
                ? config.fieldName
                : "";

            kind =
                config.kind;

            type =
                config.valueType;

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
                    PointerLooksValid(gameObject) &&
                    PointerLooksValid(transform);
            }

            return
                PointerLooksValid(object) &&
                PointerLooksValid(field);
        }


        // ====================================================================
        // CLEAR OBJECT
        // ====================================================================

        void ClearLocatedObject()
        {
            object = nullptr;
            field = nullptr;

            gameObject = nullptr;
            transform = nullptr;

            found = false;
            originalCached = false;
            touched = false;
        }


        // ====================================================================
        // LOCATE
        // ====================================================================

        bool Locate()
        {
            ClearLocatedObject();


            // =================================================================
            // TRANSFORM
            // =================================================================

            if (kind ==
                VarCtl::ControllerKind::Transform)
            {
                if (id.empty())
                    return false;

                Il2CppObject go =
                    FindGameObject(
                        id.c_str()
                    );

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
                if (fieldName.empty())
                    return false;

                bool located =
                    FindFieldAndObject(
                        fieldName.c_str(),
                        type,
                        object,
                        field
                    );

                found =
                    located &&
                    PointerLooksValid(object) &&
                    PointerLooksValid(field);

                return found;
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

                return;
            }


            // =================================================================
            // FIELD
            // =================================================================

            if (kind ==
                VarCtl::ControllerKind::Field)
            {
                if (!g_api.field_get_value)
                    return;

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
                    lastTargetFloat = value;

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
                    lastTargetInt = value;

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

                    g_api.field_get_value(
                        reinterpret_cast<Il2CppObject>(
                            object
                            ),
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


            // =================================================================
            // TRANSFORM
            // =================================================================

            if (kind ==
                VarCtl::ControllerKind::Transform)
            {
                return
                    GetTransformVector(
                        transform,
                        transformProperty
                    );
            }


            // =================================================================
            // FIELD VECTOR3
            // =================================================================

            if (kind ==
                VarCtl::ControllerKind::Field)
            {
                if (!g_api.field_get_value)
                {
                    return {
                        0.0f,
                        0.0f,
                        0.0f
                    };
                }

                if (type ==
                    VarCtl::ValueType::Vector3)
                {
                    VarCtl::Vector3 value{
                        0.0f,
                        0.0f,
                        0.0f
                    };

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

                    return {
                        value,
                        0.0f,
                        0.0f
                    };
                }
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

        bool WriteVector3(
            const VarCtl::Vector3& value)
        {
            if (!Found())
                return false;

            if (kind ==
                VarCtl::ControllerKind::Transform)
            {
                if (SetTransformVector(
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
        // INVALIDATE FIELD
        // ====================================================================

        void InvalidateField()
        {
            object = nullptr;
            field = nullptr;

            found = false;
            originalCached = false;
            touched = false;

            nextScanTick = 0;
        }


        // ====================================================================
        // INVALIDATE TRANSFORM
        // ====================================================================

        void InvalidateTransform()
        {
            gameObject = nullptr;
            transform = nullptr;

            found = false;
            originalCached = false;
            touched = false;

            nextScanTick = 0;
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

                lastTargetFloat = 0.0f;
                lastTargetInt = 0;
                lastTargetBool = false;
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
                    InvalidateTransform();
                    return;
                }
            }


            // =================================================================
            // CHECK FIELD
            // =================================================================

            if (kind ==
                VarCtl::ControllerKind::Field)
            {
                if (!InstanceAlive(
                    reinterpret_cast<Il2CppObject>(
                        object
                        )))
                {
                    InvalidateField();
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
                    if (WriteVector3(target))
                    {
                        lastTargetVector3 =
                            target;
                    }
                }

                return;
            }


            // =================================================================
            // FIELD
            // =================================================================

            if (kind ==
                VarCtl::ControllerKind::Field)
            {
                if (!g_api.field_set_value)
                    return;

                bool enabled =
                    active.load(
                        std::memory_order_relaxed
                    );


                switch (type)
                {
                    // =============================================================
                    // FLOAT
                    // =============================================================

                case VarCtl::ValueType::Float:
                {
                    float target =
                        enabled
                        ? overrideFloat * multiplier
                        : originalFloat;

                    if (target != lastTargetFloat)
                    {
                        float value = target;

                        g_api.field_set_value(
                            reinterpret_cast<Il2CppObject>(
                                object
                                ),
                            field,
                            &value
                        );

                        touched = true;
                        lastTargetFloat = target;
                    }

                    break;
                }


                // =============================================================
                // INT
                // =============================================================

                case VarCtl::ValueType::Int:
                {
                    int target =
                        enabled
                        ? overrideInt
                        : originalInt;

                    if (target != lastTargetInt)
                    {
                        int value = target;

                        g_api.field_set_value(
                            reinterpret_cast<Il2CppObject>(
                                object
                                ),
                            field,
                            &value
                        );

                        touched = true;
                        lastTargetInt = target;
                    }

                    break;
                }


                // =============================================================
                // BOOL
                // =============================================================

                case VarCtl::ValueType::Bool:
                {
                    bool target =
                        enabled
                        ? overrideBool
                        : originalBool;

                    if (target != lastTargetBool)
                    {
                        bool value = target;

                        g_api.field_set_value(
                            reinterpret_cast<Il2CppObject>(
                                object
                                ),
                            field,
                            &value
                        );

                        touched = true;
                        lastTargetBool = target;
                    }

                    break;
                }


                // =============================================================
                // VECTOR3
                // =============================================================

                case VarCtl::ValueType::Vector3:
                {
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
                        VarCtl::Vector3 value =
                            target;

                        g_api.field_set_value(
                            reinterpret_cast<Il2CppObject>(
                                object
                                ),
                            field,
                            &value
                        );

                        touched = true;
                        lastTargetVector3 =
                            target;
                    }

                    break;
                }
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


            // =================================================================
            // RESTORE ORIGINAL
            // =================================================================

            if (touched &&
                originalCached &&
                Found())
            {
                if (kind ==
                    VarCtl::ControllerKind::Transform)
                {
                    if (InstanceAlive(gameObject) &&
                        InstanceAlive(transform))
                    {
                        WriteVector3(
                            originalVector3
                        );
                    }
                }
                else if (
                    kind ==
                    VarCtl::ControllerKind::Field)
                {
                    if (InstanceAlive(
                        reinterpret_cast<Il2CppObject>(
                            object
                            )))
                    {
                        switch (type)
                        {
                        case VarCtl::ValueType::Float:
                        {
                            float value =
                                originalFloat;

                            g_api.field_set_value(
                                reinterpret_cast<Il2CppObject>(
                                    object
                                    ),
                                field,
                                &value
                            );

                            break;
                        }

                        case VarCtl::ValueType::Int:
                        {
                            int value =
                                originalInt;

                            g_api.field_set_value(
                                reinterpret_cast<Il2CppObject>(
                                    object
                                    ),
                                field,
                                &value
                            );

                            break;
                        }

                        case VarCtl::ValueType::Bool:
                        {
                            bool value =
                                originalBool;

                            g_api.field_set_value(
                                reinterpret_cast<Il2CppObject>(
                                    object
                                    ),
                                field,
                                &value
                            );

                            break;
                        }

                        case VarCtl::ValueType::Vector3:
                        {
                            VarCtl::Vector3 value =
                                originalVector3;

                            g_api.field_set_value(
                                reinterpret_cast<Il2CppObject>(
                                    object
                                    ),
                                field,
                                &value
                            );

                            break;
                        }
                        }
                    }
                }
            }


            // =================================================================
            // CLEAR
            // =================================================================

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
            g_api = {};
            return false;
        }


        if (!CreateRuntimeControllers())
        {
            g_api = {};
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

        g_api = {};
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
            .displayName
            .c_str();
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
        if (!g_runtimeControllers ||
            index >= g_runtimeControllerCount)
        {
            return 0;
        }

        Controller& controller =
            g_runtimeControllers[index];

        if (!controller.Found() ||
            controller.kind !=
            VarCtl::ControllerKind::Field ||
            controller.type !=
            VarCtl::ValueType::Int)
        {
            return 0;
        }

        if (!g_api.field_get_value)
            return 0;

        int value = 0;

        g_api.field_get_value(
            reinterpret_cast<Il2CppObject>(
                controller.object
                ),
            controller.field,
            &value
        );

        return value;
    }


    bool CurrentBoolAt(size_t index)
    {
        if (!g_runtimeControllers ||
            index >= g_runtimeControllerCount)
        {
            return false;
        }

        Controller& controller =
            g_runtimeControllers[index];

        if (!controller.Found() ||
            controller.kind !=
            VarCtl::ControllerKind::Field ||
            controller.type !=
            VarCtl::ValueType::Bool)
        {
            return false;
        }

        if (!g_api.field_get_value)
            return false;

        bool value = false;

        g_api.field_get_value(
            reinterpret_cast<Il2CppObject>(
                controller.object
                ),
            controller.field,
            &value
        );

        return value;
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