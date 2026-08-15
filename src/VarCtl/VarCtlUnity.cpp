#include "VarCtlUnity.h"
#include "Main.h"

#include <windows.h>
#include <cstring>
#include <cstdint>

namespace
{
    // ========================================================================
    // IL2CPP STATE
    // ========================================================================

    Il2CppApi g_api{};
    bool g_ready = false;

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

    void EnsureAttached()
    {
        thread_local bool attached = false;

        if (attached)
            return;

        if (!g_api.thread_attach || !g_api.domain_get)
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
    // INVOKE
    // ========================================================================

    bool Invoke(
        Il2CppMethod method,
        void* instance,
        void** args,
        Il2CppObject* outResult = nullptr
    )
    {
        if (!method || !g_api.runtime_invoke)
            return false;

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
    // RESOLVE UNITY
    // ========================================================================

    bool Resolve()
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

        // --------------------------------------------------------------------
        // FindObjectsOfType
        // --------------------------------------------------------------------

        g_findObjectsOfType =
            g_api.class_get_method_from_name(
                g_objectClass,
                "FindObjectsOfType",
                1
            );

        // --------------------------------------------------------------------
        // GameObject.Find
        // --------------------------------------------------------------------

        g_gameObjectFind =
            g_api.class_get_method_from_name(
                g_gameObjectClass,
                "Find",
                1
            );

        // --------------------------------------------------------------------
        // GameObject.transform
        // --------------------------------------------------------------------

        g_gameObjectGetTransform =
            g_api.class_get_method_from_name(
                g_gameObjectClass,
                "get_transform",
                0
            );

        // --------------------------------------------------------------------
        // Transform position
        // --------------------------------------------------------------------

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

        // --------------------------------------------------------------------
        // Transform rotation
        // --------------------------------------------------------------------

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

        // --------------------------------------------------------------------
        // Transform scale
        // --------------------------------------------------------------------

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

        // --------------------------------------------------------------------
        // Unity Object.m_CachedPtr
        // --------------------------------------------------------------------

        g_cachedPtrField =
            FindFieldInHierarchy(
                g_api,
                g_objectClass,
                "m_CachedPtr"
            );

        // --------------------------------------------------------------------
        // Required methods
        // --------------------------------------------------------------------

        return
            g_gameObjectFind &&
            g_gameObjectGetTransform &&
            g_findObjectsOfType &&
            g_getLocalPosition &&
            g_setLocalPosition &&
            g_getLocalEulerAngles &&
            g_setLocalEulerAngles &&
            g_getLocalScale &&
            g_setLocalScale;
    }
}

// ============================================================================
// PUBLIC UNITY API
// ============================================================================

namespace VarCtlUnity
{
    // ========================================================================
    // INITIALIZE
    // ========================================================================

    bool Initialize()
    {
        if (g_ready)
            return true;

        if (!LoadIl2CppApi(g_api, 60000))
            return false;

        EnsureAttached();

        if (!Resolve())
        {
            g_api = {};
            return false;
        }

        g_ready = true;
        return true;
    }

    // ========================================================================
    // READY
    // ========================================================================

    bool IsReady()
    {
        return g_ready;
    }

    // ========================================================================
    // API
    // ========================================================================

    Il2CppApi& Api()
    {
        return g_api;
    }

    // ========================================================================
    // THREAD ATTACH
    // ========================================================================

    void EnsureThreadAttached()
    {
        EnsureAttached();
    }

    // ========================================================================
    // RESET
    // ========================================================================

    void Reset()
    {
        g_objectClass = nullptr;
        g_gameObjectClass = nullptr;
        g_transformClass = nullptr;
        g_monoBehaviourClass = nullptr;

        g_gameObjectFind = nullptr;
        g_gameObjectGetTransform = nullptr;
        g_findObjectsOfType = nullptr;

        g_getLocalPosition = nullptr;
        g_setLocalPosition = nullptr;

        g_getLocalEulerAngles = nullptr;
        g_setLocalEulerAngles = nullptr;

        g_getLocalScale = nullptr;
        g_setLocalScale = nullptr;

        g_cachedPtrField = nullptr;

        g_api = {};
        g_ready = false;
    }

    // ========================================================================
    // POINTER SANITY
    // ========================================================================

    bool PointerLooksValid(const void* pointer)
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
    // INSTANCE VALIDITY
    // ========================================================================

    bool InstanceAlive(Il2CppObject instance)
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
    // FIND GAMEOBJECT
    // ========================================================================

    Il2CppObject FindGameObject(const char* path)
    {
        if (!path || !*path)
            return nullptr;

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

    Il2CppObject GetTransform(Il2CppObject gameObject)
    {
        if (!PointerLooksValid(gameObject))
            return nullptr;

        if (!g_gameObjectGetTransform)
            return nullptr;

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
        VarCtl::Vector3 result
        {
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

        return FindFieldInHierarchy(
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

        if (!fieldName || !*fieldName)
            return false;

        if (!g_monoBehaviourClass ||
            !g_findObjectsOfType ||
            !g_api.array_length ||
            !g_api.class_get_type ||
            !g_api.type_get_object ||
            !g_api.object_get_class)
        {
            return false;
        }

        // --------------------------------------------------------------------
        // Get System.Type for MonoBehaviour
        // --------------------------------------------------------------------

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

        // --------------------------------------------------------------------
        // FindObjectsOfType
        // --------------------------------------------------------------------

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

        // --------------------------------------------------------------------
        // Array length
        // --------------------------------------------------------------------

        uint32_t length =
            g_api.array_length(array);

        if (length == 0)
            return false;

        // --------------------------------------------------------------------
        // Array elements
        // --------------------------------------------------------------------

        void** elements =
            ArrayElements(array);

        if (!elements)
            return false;

        // --------------------------------------------------------------------
        // Search
        // --------------------------------------------------------------------

        for (uint32_t i = 0; i < length; ++i)
        {
            void* instance =
                elements[i];

            if (!PointerLooksValid(instance))
                continue;

            Il2CppObject object =
                reinterpret_cast<Il2CppObject>(
                    instance
                    );

            Il2CppClass instanceClass =
                g_api.object_get_class(object);

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
}