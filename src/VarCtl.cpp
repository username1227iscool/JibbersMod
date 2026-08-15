#include "VarCtl.h"
#include "il2cpp_api.h"
#include "Main.h"

#include <windows.h>
#include <cstring>
#include <atomic>
#include <cstddef>
#include <cstdio>

namespace
{
    // ========================================================================
    // GLOBAL IL2CPP STATE
    // ========================================================================

    Il2CppApi g_api{};

    bool g_apiReady = false;

    Il2CppClass g_monoBehaviourClass = nullptr;
    Il2CppClass g_gameObjectClass = nullptr;
    Il2CppClass g_componentClass = nullptr;
    Il2CppClass g_transformClass = nullptr;
    Il2CppClass g_unityObjectClass = nullptr;

    Il2CppMethod g_findObjectsOfType = nullptr;

    Il2CppMethod g_componentGetTransform = nullptr;
    Il2CppMethod g_componentGetGameObject = nullptr;

    Il2CppMethod g_transformGetChildCount = nullptr;
    Il2CppMethod g_transformGetChild = nullptr;
    Il2CppMethod g_transformGetParent = nullptr;

    Il2CppMethod g_gameObjectGetName = nullptr;

    Il2CppField g_cachedPtrField = nullptr;

    bool g_hierarchyPrinted = false;


    // ========================================================================
    // DEBUG OUTPUT
    // ========================================================================

    void DebugPrint(const char* text)
    {
        OutputDebugStringA(text);
    }


    void DebugPrintLine(const char* text)
    {
        OutputDebugStringA(text);
        OutputDebugStringA("\n");
    }


    // ========================================================================
    // IL2CPP THREAD ATTACH
    // ========================================================================

    void EnsureThreadAttached()
    {
        thread_local bool attached = false;

        if (attached ||
            !g_api.thread_attach ||
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
    // INVOKE MANAGED METHOD
    // ========================================================================

    Il2CppObject InvokeMethod(
        Il2CppMethod method,
        Il2CppObject object,
        void** args = nullptr)
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
                object,
                args,
                &exception
            );

        if (exception)
        {
            DebugPrintLine(
                "[VarCtl] runtime_invoke produced an exception."
            );

            return nullptr;
        }

        return result;
    }


    // ========================================================================
    // INVOKE METHOD RETURNING INT
    // ========================================================================

    int InvokeInt(
        Il2CppMethod method,
        Il2CppObject object,
        void** args = nullptr)
    {
        if (!method ||
            !g_api.runtime_invoke ||
            !g_api.object_unbox)
        {
            return 0;
        }

        Il2CppException exception = nullptr;

        Il2CppObject result =
            g_api.runtime_invoke(
                method,
                object,
                args,
                &exception
            );

        if (exception ||
            !result)
        {
            return 0;
        }

        void* unboxed =
            g_api.object_unbox(result);

        if (!unboxed)
            return 0;

        return *reinterpret_cast<int*>(unboxed);
    }


    // ========================================================================
    // IL2CPP STRING -> C STRING
    // ========================================================================

    const char* GetIl2CppString(
        Il2CppObject stringObject)
    {
        if (!stringObject)
            return "<null>";


        // Unity IL2CPP strings use UTF-16.
        //
        // We only need this temporarily for debugging.
        struct Il2CppStringLayout
        {
            void* klass;
            void* monitor;
            int32_t length;
            wchar_t chars[1];
        };


        auto* stringData =
            reinterpret_cast<Il2CppStringLayout*>(
                stringObject
                );


        static thread_local char buffer[512];


        if (stringData->length <= 0)
        {
            buffer[0] = '\0';
            return buffer;
        }


        int count =
            WideCharToMultiByte(
                CP_UTF8,
                0,
                stringData->chars,
                stringData->length,
                buffer,
                static_cast<int>(
                    sizeof(buffer) - 1
                    ),
                nullptr,
                nullptr
            );


        if (count <= 0)
        {
            buffer[0] = '\0';
            return buffer;
        }


        buffer[count] = '\0';

        return buffer;
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
        DebugPrintLine(
            "[VarCtl] Resolving Unity classes..."
        );


        g_unityObjectClass =
            FindIl2CppClass(
                g_api,
                "UnityEngine",
                "Object"
            );


        g_monoBehaviourClass =
            FindIl2CppClass(
                g_api,
                "UnityEngine",
                "MonoBehaviour"
            );


        g_gameObjectClass =
            FindIl2CppClass(
                g_api,
                "UnityEngine",
                "GameObject"
            );


        g_componentClass =
            FindIl2CppClass(
                g_api,
                "UnityEngine",
                "Component"
            );


        g_transformClass =
            FindIl2CppClass(
                g_api,
                "UnityEngine",
                "Transform"
            );


        if (!g_unityObjectClass)
        {
            DebugPrintLine(
                "[VarCtl] FAILED: UnityEngine.Object"
            );

            return false;
        }


        if (!g_gameObjectClass)
        {
            DebugPrintLine(
                "[VarCtl] FAILED: UnityEngine.GameObject"
            );

            return false;
        }


        if (!g_componentClass)
        {
            DebugPrintLine(
                "[VarCtl] FAILED: UnityEngine.Component"
            );

            return false;
        }


        if (!g_transformClass)
        {
            DebugPrintLine(
                "[VarCtl] FAILED: UnityEngine.Transform"
            );

            return false;
        }


        if (!g_monoBehaviourClass)
        {
            DebugPrintLine(
                "[VarCtl] FAILED: UnityEngine.MonoBehaviour"
            );

            return false;
        }


        // ------------------------------------------------------------
        // FindObjectsOfType(Type)
        // ------------------------------------------------------------

        g_findObjectsOfType =
            FindObjectsOfTypeMethod(
                g_unityObjectClass
            );


        if (!g_findObjectsOfType)
        {
            DebugPrintLine(
                "[VarCtl] FAILED: FindObjectsOfType"
            );

            return false;
        }


        // ------------------------------------------------------------
        // Component.get_transform
        // ------------------------------------------------------------

        g_componentGetTransform =
            g_api.class_get_method_from_name(
                g_componentClass,
                "get_transform",
                0
            );


        if (!g_componentGetTransform)
        {
            DebugPrintLine(
                "[VarCtl] FAILED: Component.get_transform"
            );

            return false;
        }


        // ------------------------------------------------------------
        // Component.get_gameObject
        // ------------------------------------------------------------

        g_componentGetGameObject =
            g_api.class_get_method_from_name(
                g_componentClass,
                "get_gameObject",
                0
            );


        if (!g_componentGetGameObject)
        {
            DebugPrintLine(
                "[VarCtl] FAILED: Component.get_gameObject"
            );

            return false;
        }


        // ------------------------------------------------------------
        // Transform.get_childCount
        // ------------------------------------------------------------

        g_transformGetChildCount =
            g_api.class_get_method_from_name(
                g_transformClass,
                "get_childCount",
                0
            );


        if (!g_transformGetChildCount)
        {
            DebugPrintLine(
                "[VarCtl] FAILED: Transform.get_childCount"
            );

            return false;
        }


        // ------------------------------------------------------------
        // Transform.GetChild(int)
        // ------------------------------------------------------------

        g_transformGetChild =
            g_api.class_get_method_from_name(
                g_transformClass,
                "GetChild",
                1
            );


        if (!g_transformGetChild)
        {
            DebugPrintLine(
                "[VarCtl] FAILED: Transform.GetChild"
            );

            return false;
        }


        // ------------------------------------------------------------
        // Transform.get_parent
        // ------------------------------------------------------------

        g_transformGetParent =
            g_api.class_get_method_from_name(
                g_transformClass,
                "get_parent",
                0
            );


        if (!g_transformGetParent)
        {
            DebugPrintLine(
                "[VarCtl] FAILED: Transform.get_parent"
            );

            return false;
        }


        // ------------------------------------------------------------
        // GameObject.get_name
        // ------------------------------------------------------------

        g_gameObjectGetName =
            g_api.class_get_method_from_name(
                g_gameObjectClass,
                "get_name",
                0
            );


        if (!g_gameObjectGetName)
        {
            DebugPrintLine(
                "[VarCtl] FAILED: GameObject.get_name"
            );

            return false;
        }


        DebugPrintLine(
            "[VarCtl] Unity hierarchy API resolved successfully."
        );

        return true;
    }


    // ========================================================================
    // GET GAMEOBJECT FROM COMPONENT
    // ========================================================================

    Il2CppObject GetGameObjectFromComponent(
        Il2CppObject component)
    {
        if (!component ||
            !g_componentGetGameObject)
        {
            return nullptr;
        }

        return InvokeMethod(
            g_componentGetGameObject,
            component
        );
    }


    // ========================================================================
    // GET TRANSFORM FROM COMPONENT
    // ========================================================================

    Il2CppObject GetTransformFromComponent(
        Il2CppObject component)
    {
        if (!component ||
            !g_componentGetTransform)
        {
            return nullptr;
        }

        return InvokeMethod(
            g_componentGetTransform,
            component
        );
    }


    // ========================================================================
    // GET GAMEOBJECT NAME
    // ========================================================================

    const char* GetGameObjectName(
        Il2CppObject gameObject)
    {
        if (!gameObject ||
            !g_gameObjectGetName)
        {
            return "<null>";
        }

        Il2CppObject nameObject =
            InvokeMethod(
                g_gameObjectGetName,
                gameObject
            );

        return GetIl2CppString(
            nameObject
        );
    }


    // ========================================================================
    // GET TRANSFORM NAME
    // ========================================================================

    const char* GetTransformName(
        Il2CppObject transform)
    {
        if (!transform)
            return "<null transform>";

        Il2CppObject gameObject =
            GetGameObjectFromComponent(
                transform
            );

        if (!gameObject)
            return "<no GameObject>";

        return GetGameObjectName(
            gameObject
        );
    }


    // ========================================================================
    // PRINT ONE TRANSFORM
    // ========================================================================

    void PrintTransform(
        Il2CppObject transform,
        int depth)
    {
        if (!transform)
            return;


        char indent[256];

        int spaces =
            depth * 2;


        if (spaces >=
            static_cast<int>(
                sizeof(indent) - 1
                ))
        {
            spaces =
                static_cast<int>(
                    sizeof(indent) - 1
                    );
        }


        for (int i = 0; i < spaces; ++i)
            indent[i] = ' ';


        indent[spaces] = '\0';


        const char* name =
            GetTransformName(
                transform
            );


        char message[1024];


        sprintf_s(
            message,
            sizeof(message),
            "%s%s\n",
            indent,
            name
        );


        DebugPrint(message);
    }


    // ========================================================================
    // RECURSIVELY PRINT TRANSFORM CHILDREN
    // ========================================================================

    void PrintTransformHierarchy(
        Il2CppObject transform,
        int depth)
    {
        if (!transform)
            return;


        PrintTransform(
            transform,
            depth
        );


        int childCount =
            InvokeInt(
                g_transformGetChildCount,
                transform
            );


        for (int i = 0;
            i < childCount;
            ++i)
        {
            // GetChild(int)
            //
            // IMPORTANT:
            // runtime_invoke wants a pointer
            // to the argument.

            void* args[1];

            args[0] = &i;


            Il2CppObject child =
                InvokeMethod(
                    g_transformGetChild,
                    transform,
                    args
                );


            if (!child)
                continue;


            PrintTransformHierarchy(
                child,
                depth + 1
            );
        }
    }


    // ========================================================================
    // FIND GAMEOBJECT BY NAME
    // ========================================================================

    Il2CppObject FindGameObjectByName(
        const char* wantedName)
    {
        if (!wantedName ||
            !g_findObjectsOfType ||
            !g_gameObjectClass)
        {
            return nullptr;
        }


        DebugPrint(
            "[VarCtl] Searching for GameObject: "
        );

        DebugPrintLine(
            wantedName
        );


        // ------------------------------------------------------------
        // Convert GameObject class to System.Type
        // ------------------------------------------------------------

        const Il2CppType* gameObjectType =
            g_api.class_get_type(
                g_gameObjectClass
            );


        if (!gameObjectType)
        {
            DebugPrintLine(
                "[VarCtl] Failed to get GameObject type."
            );

            return nullptr;
        }


        Il2CppReflectionType* reflectionType =
            g_api.type_get_object(
                gameObjectType
            );


        if (!reflectionType)
        {
            DebugPrintLine(
                "[VarCtl] Failed to create reflection type."
            );

            return nullptr;
        }


        // FindObjectsOfType(Type)
        void* args[1];

        args[0] =
            reflectionType;


        Il2CppObject array =
            InvokeMethod(
                g_findObjectsOfType,
                nullptr,
                args
            );


        if (!array)
        {
            DebugPrintLine(
                "[VarCtl] FindObjectsOfType returned null."
            );

            return nullptr;
        }


        uint32_t count =
            g_api.array_length(
                array
            );


        char message[256];

        sprintf_s(
            message,
            sizeof(message),
            "[VarCtl] Found %u GameObjects.\n",
            count
        );

        DebugPrint(message);


        void** elements =
            ArrayElements(
                array
            );


        if (!elements)
        {
            DebugPrintLine(
                "[VarCtl] ArrayElements returned null."
            );

            return nullptr;
        }


        for (uint32_t i = 0;
            i < count;
            ++i)
        {
            Il2CppObject gameObject =
                reinterpret_cast<Il2CppObject>(
                    elements[i]
                    );


            if (!gameObject)
                continue;


            const char* name =
                GetGameObjectName(
                    gameObject
                );


            if (!name)
                continue;


            if (std::strcmp(
                name,
                wantedName
            ) == 0)
            {
                DebugPrint(
                    "[VarCtl] FOUND: "
                );

                DebugPrintLine(
                    name
                );

                return gameObject;
            }
        }


        DebugPrint(
            "[VarCtl] Could not find GameObject: "
        );

        DebugPrintLine(
            wantedName
        );


        return nullptr;
    }


    // ========================================================================
    // PRINT BODIES HIERARCHY
    // ========================================================================

    bool DebugPrintBodiesHierarchy()
    {
        DebugPrintLine(
            ""
        );

        DebugPrintLine(
            "============================================================"
        );

        DebugPrintLine(
            "[VarCtl] SEARCHING UNITY HIERARCHY"
        );

        DebugPrintLine(
            "============================================================"
        );


        Il2CppObject bodies =
            FindGameObjectByName(
                "Bodies"
            );


        if (!bodies)
        {
            DebugPrintLine(
                "[VarCtl] Bodies was not found yet."
            );

            DebugPrintLine(
                "============================================================"
            );

            return false;
        }


        Il2CppObject bodiesTransform =
            GetTransformFromComponent(
                bodies
            );


        if (!bodiesTransform)
        {
            DebugPrintLine(
                "[VarCtl] Bodies has no Transform."
            );

            return false;
        }


        DebugPrintLine(
            ""
        );

        DebugPrintLine(
            "[VarCtl] BODIES HIERARCHY:"
        );

        DebugPrintLine(
            ""
        );


        PrintTransformHierarchy(
            bodiesTransform,
            0
        );


        DebugPrintLine(
            ""
        );

        DebugPrintLine(
            "============================================================"
        );

        DebugPrintLine(
            "[VarCtl] END HIERARCHY"
        );

        DebugPrintLine(
            "============================================================"
        );


        return true;
    }


    // ========================================================================
    // CHECK UNITY OBJECT
    // ========================================================================

    bool InstanceAlive(
        void* instance)
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
            g_api.field_get_type(
                field
            );


        if (!il2cppType)
            return false;


        Il2CppClass fieldClass =
            g_api.class_from_type(
                il2cppType
            );


        if (!fieldClass)
            return false;


        const char* typeName =
            g_api.class_get_name(
                fieldClass
            );


        if (!typeName)
            return false;


        switch (requestedType)
        {
        case VarCtl::ValueType::Float:
            return std::strcmp(
                typeName,
                "Single"
            ) == 0;


        case VarCtl::ValueType::Int:
            return std::strcmp(
                typeName,
                "Int32"
            ) == 0;


        case VarCtl::ValueType::Bool:
            return std::strcmp(
                typeName,
                "Boolean"
            ) == 0;


        case VarCtl::ValueType::Vector3:
            return std::strcmp(
                typeName,
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


        Il2CppObject array =
            g_api.runtime_invoke(
                g_findObjectsOfType,
                nullptr,
                args,
                nullptr
            );


        if (!array)
            return false;


        uint32_t length =
            g_api.array_length(
                array
            );


        void** elements =
            ArrayElements(
                array
            );


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


        std::atomic<bool> active{ false };

        void* object = nullptr;
        Il2CppField field = nullptr;

        bool ready = false;
        bool originalCached = false;
        bool touched = false;

        float originalFloat = 0.0f;
        int originalInt = 0;
        bool originalBool = false;

        VarCtl::Vector3 originalVector3
        {
            0.0f,
            0.0f,
            0.0f
        };


        float lastTargetFloat = 0.0f;
        int lastTargetInt = 0;
        bool lastTargetBool = false;

        VarCtl::Vector3 lastTargetVector3
        {
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


                return static_cast<float>(
                    value
                    );
            }


            case VarCtl::ValueType::Bool:
            {
                bool value = false;


                g_api.field_get_value(
                    object,
                    field,
                    &value
                );


                return value
                    ? 1.0f
                    : 0.0f;
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


        int ReadInt()
        {
            if (!Found())
                return 0;


            int value = 0;


            if (type ==
                VarCtl::ValueType::Int)
            {
                g_api.field_get_value(
                    object,
                    field,
                    &value
                );
            }


            return value;
        }


        bool ReadBool()
        {
            if (!Found())
                return false;


            bool value = false;


            if (type ==
                VarCtl::ValueType::Bool)
            {
                g_api.field_get_value(
                    object,
                    field,
                    &value
                );
            }


            return value;
        }


        VarCtl::Vector3 ReadVector3()
        {
            if (!Found())
            {
                return
                {
                    0.0f,
                    0.0f,
                    0.0f
                };
            }


            VarCtl::Vector3 value{};


            if (type ==
                VarCtl::ValueType::Vector3)
            {
                g_api.field_get_value(
                    object,
                    field,
                    &value
                );
            }


            return value;
        }


        void WriteFloat(
            float value)
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


        void WriteInt(
            int value)
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


        void WriteBool(
            bool value)
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


        void WriteVector3(
            const VarCtl::Vector3& value)
        {
            if (!Found())
                return;


            VarCtl::Vector3 copy =
                value;


            g_api.field_set_value(
                object,
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


            if (!InstanceAlive(object) ||
                !Found())
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


                lastTargetVector3 =
                {
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


            // ------------------------------------------------------------
            // FLOAT
            // ------------------------------------------------------------

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


            // ------------------------------------------------------------
            // INT
            // ------------------------------------------------------------

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


            // ------------------------------------------------------------
            // BOOL
            // ------------------------------------------------------------

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


            // ------------------------------------------------------------
            // VECTOR3
            // ------------------------------------------------------------

            if (type ==
                VarCtl::ValueType::Vector3)
            {
                VarCtl::Vector3 target =
                    enabled
                    ? VarCtl::Vector3
                {
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
                    WriteVector3(
                        target
                    );


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
                    WriteFloat(
                        originalFloat
                    );
                    break;


                case VarCtl::ValueType::Int:
                    WriteInt(
                        originalInt
                    );
                    break;


                case VarCtl::ValueType::Bool:
                    WriteBool(
                        originalBool
                    );
                    break;


                case VarCtl::ValueType::Vector3:
                    WriteVector3(
                        originalVector3
                    );
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
}


// ============================================================================
// PUBLIC VarCtl API
// ============================================================================

namespace VarCtl
{
    bool Init()
    {
        // ------------------------------------------------------------
        // Load IL2CPP
        // ------------------------------------------------------------

        if (!g_apiReady)
        {
            if (!LoadIl2CppApi(
                g_api,
                60000))
            {
                DebugPrintLine(
                    "[VarCtl] Failed to load IL2CPP API."
                );

                return false;
            }


            EnsureThreadAttached();


            if (!ResolveClasses())
            {
                DebugPrintLine(
                    "[VarCtl] Failed to resolve Unity classes."
                );

                return false;
            }


            // --------------------------------------------------------
            // Resolve cached pointer field
            // --------------------------------------------------------

            g_cachedPtrField =
                FindFieldInHierarchy(
                    g_api,
                    g_unityObjectClass,
                    "m_CachedPtr"
                );


            g_apiReady = true;


            DebugPrintLine(
                "[VarCtl] IL2CPP initialized."
            );
        }


        // ------------------------------------------------------------
        // Initialize controllers
        // ------------------------------------------------------------

        for (size_t i = 0;
            i < g_controllers_count;
            ++i)
        {
            Controller& controller =
                reinterpret_cast<Controller*>(
                    g_controllers
                    )[i];

            controller.ready = true;
        }


        return true;
    }


    // ========================================================================
    // TICK
    // ========================================================================

    void Tick()
    {
        if (!g_apiReady)
            return;


        EnsureThreadAttached();


        // ------------------------------------------------------------
        // Debug hierarchy
        //
        // We try this every second until Bodies exists.
        // Once successful, we stop printing it.
        // ------------------------------------------------------------

        static DWORD nextHierarchyScan = 0;


        if (!g_hierarchyPrinted)
        {
            DWORD now =
                GetTickCount();


            if (now >= nextHierarchyScan)
            {
                nextHierarchyScan =
                    now + 1000;


                if (DebugPrintBodiesHierarchy())
                {
                    g_hierarchyPrinted = true;
                }
            }
        }


        // ------------------------------------------------------------
        // Controllers
        // ------------------------------------------------------------

        for (size_t i = 0;
            i < g_controllers_count;
            ++i)
        {
            Controller& controller =
                reinterpret_cast<Controller*>(
                    g_controllers
                    )[i];

            controller.Tick();
        }
    }


    // ========================================================================
    // SHUTDOWN
    // ========================================================================

    void Shutdown()
    {
        for (size_t i = 0;
            i < g_controllers_count;
            ++i)
        {
            Controller& controller =
                reinterpret_cast<Controller*>(
                    g_controllers
                    )[i];

            controller.Shutdown();
        }


        g_apiReady = false;

        g_hierarchyPrinted = false;
    }


    // ========================================================================
    // COUNT
    // ========================================================================

    size_t Count()
    {
        return g_controllers_count;
    }


    // ========================================================================
    // NAME
    // ========================================================================

    const char* NameAt(
        size_t index)
    {
        if (index >= g_controllers_count)
            return "";


        return reinterpret_cast<Controller*>(
            g_controllers
            )[index].displayName;
    }


    // ========================================================================
    // READY
    // ========================================================================

    bool ReadyAt(
        size_t index)
    {
        if (index >= g_controllers_count)
            return false;


        return reinterpret_cast<Controller*>(
            g_controllers
            )[index].ready;
    }


    // ========================================================================
    // FOUND
    // ========================================================================

    bool FoundAt(
        size_t index)
    {
        if (index >= g_controllers_count)
            return false;


        return reinterpret_cast<Controller*>(
            g_controllers
            )[index].Found();
    }


    // ========================================================================
    // TYPE
    // ========================================================================

    ValueType TypeAt(
        size_t index)
    {
        if (index >= g_controllers_count)
            return ValueType::Float;


        return reinterpret_cast<Controller*>(
            g_controllers
            )[index].type;
    }


    // ========================================================================
    // ACTIVE
    // ========================================================================

    bool IsActiveAt(
        size_t index)
    {
        if (index >= g_controllers_count)
            return false;


        return reinterpret_cast<Controller*>(
            g_controllers
            )[index]
            .active.load(
                std::memory_order_relaxed
            );
    }


    void SetActiveAt(
        size_t index,
        bool value)
    {
        if (index >= g_controllers_count)
            return;


        reinterpret_cast<Controller*>(
            g_controllers
            )[index]
            .active.store(
                value,
                std::memory_order_relaxed
            );
    }


    // ========================================================================
    // FLOAT OVERRIDE
    // ========================================================================

    float OverrideValueAt(
        size_t index)
    {
        if (index >= g_controllers_count)
            return 0.0f;


        return reinterpret_cast<Controller*>(
            g_controllers
            )[index].overrideFloat;
    }


    void SetOverrideValueAt(
        size_t index,
        float value)
    {
        if (index >= g_controllers_count)
            return;


        reinterpret_cast<Controller*>(
            g_controllers
            )[index].overrideFloat =
            value;
    }


    float* OverridePtrAt(
        size_t index)
    {
        if (index >= g_controllers_count)
            return nullptr;


        return &reinterpret_cast<Controller*>(
            g_controllers
            )[index].overrideFloat;
    }


    // ========================================================================
    // INT OVERRIDE
    // ========================================================================

    int OverrideIntAt(
        size_t index)
    {
        if (index >= g_controllers_count)
            return 0;


        return reinterpret_cast<Controller*>(
            g_controllers
            )[index].overrideInt;
    }


    void SetOverrideIntAt(
        size_t index,
        int value)
    {
        if (index >= g_controllers_count)
            return;


        reinterpret_cast<Controller*>(
            g_controllers
            )[index].overrideInt =
            value;
    }


    int* OverrideIntPtrAt(
        size_t index)
    {
        if (index >= g_controllers_count)
            return nullptr;


        return &reinterpret_cast<Controller*>(
            g_controllers
            )[index].overrideInt;
    }


    // ========================================================================
    // BOOL OVERRIDE
    // ========================================================================

    bool OverrideBoolAt(
        size_t index)
    {
        if (index >= g_controllers_count)
            return false;


        return reinterpret_cast<Controller*>(
            g_controllers
            )[index].overrideBool;
    }


    void SetOverrideBoolAt(
        size_t index,
        bool value)
    {
        if (index >= g_controllers_count)
            return;


        reinterpret_cast<Controller*>(
            g_controllers
            )[index].overrideBool =
            value;
    }


    // ========================================================================
    // VECTOR3 OVERRIDE
    // ========================================================================

    Vector3 OverrideVector3At(
        size_t index)
    {
        if (index >= g_controllers_count)
        {
            return
            {
                0.0f,
                0.0f,
                0.0f
            };
        }


        return reinterpret_cast<Controller*>(
            g_controllers
            )[index].overrideVector3;
    }


    void SetOverrideVector3At(
        size_t index,
        Vector3 value)
    {
        if (index >= g_controllers_count)
            return;


        reinterpret_cast<Controller*>(
            g_controllers
            )[index].overrideVector3 =
            value;
    }


    float* OverrideVector3PtrAt(
        size_t index)
    {
        if (index >= g_controllers_count)
            return nullptr;


        return &reinterpret_cast<Controller*>(
            g_controllers
            )[index].overrideVector3.x;
    }


    // ========================================================================
    // LIMITS
    // ========================================================================

    float LowerLimitAt(
        size_t index)
    {
        if (index >= g_controllers_count)
            return 0.0f;


        return reinterpret_cast<Controller*>(
            g_controllers
            )[index].lowerLimit;
    }


    float UpperLimitAt(
        size_t index)
    {
        if (index >= g_controllers_count)
            return 0.0f;


        return reinterpret_cast<Controller*>(
            g_controllers
            )[index].upperLimit;
    }


    // ========================================================================
    // CURRENT FLOAT
    // ========================================================================

    float CurrentValueAt(
        size_t index)
    {
        if (index >= g_controllers_count)
            return 0.0f;


        return reinterpret_cast<Controller*>(
            g_controllers
            )[index].Read();
    }


    // ========================================================================
    // CURRENT INT
    // ========================================================================

    int CurrentIntAt(
        size_t index)
    {
        if (index >= g_controllers_count)
            return 0;


        return reinterpret_cast<Controller*>(
            g_controllers
            )[index].ReadInt();
    }


    // ========================================================================
    // CURRENT BOOL
    // ========================================================================

    bool CurrentBoolAt(
        size_t index)
    {
        if (index >= g_controllers_count)
            return false;


        return reinterpret_cast<Controller*>(
            g_controllers
            )[index].ReadBool();
    }


    // ========================================================================
    // CURRENT VECTOR3
    // ========================================================================

    Vector3 CurrentVector3At(
        size_t index)
    {
        if (index >= g_controllers_count)
        {
            return
            {
                0.0f,
                0.0f,
                0.0f
            };
        }


        return reinterpret_cast<Controller*>(
            g_controllers
            )[index].ReadVector3();
    }


    // ========================================================================
    // SHOW SLIDER
    // ========================================================================

    bool ShowSliderAt(
        size_t index)
    {
        if (index >= g_controllers_count)
            return false;


        return reinterpret_cast<Controller*>(
            g_controllers
            )[index].showSlider;
    }
}