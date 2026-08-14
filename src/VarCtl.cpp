#include "VarCtl.h"
#include "il2cpp_api.h"

#include <windows.h>
#include <cstring>
#include <atomic>

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

        g_api.thread_attach(g_api.domain_get());
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
            FindIl2CppClass(g_api, "UnityEngine", "MonoBehaviour");

        Il2CppClass unityObjectClass =
            FindIl2CppClass(g_api, "UnityEngine", "Object");

        if (!g_monoBehaviourClass || !unityObjectClass)
            return false;

        g_findObjectsOfType = FindObjectsOfTypeMethod(unityObjectClass);
        if (!g_findObjectsOfType)
            return false;

        g_cachedPtrField =
            FindFieldInHierarchy(g_api, unityObjectClass, "m_CachedPtr");

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
        g_api.field_get_value(instance, g_cachedPtrField, &cached);
        return cached != nullptr;
    }


    // ========================================================================
    // CHECK FIELD IS FLOAT
    // ========================================================================

    bool FieldIsFloat(Il2CppField field)
    {
        if (!g_api.field_get_type || !g_api.class_from_type || !g_api.class_get_name)
            return true;

        const Il2CppType* type = g_api.field_get_type(field);
        Il2CppClass klass = type ? g_api.class_from_type(type) : nullptr;
        const char* name = klass ? g_api.class_get_name(klass) : nullptr;
        return name && std::strcmp(name, "Single") == 0;
    }


    // ========================================================================
    // FIND A FIELD ON ANY MONOBEHAVIOUR
    // ========================================================================

    bool FindFieldAndObject(const char* fieldName, void*& outObject, Il2CppField& outField)
    {
        outObject = nullptr;
        outField = nullptr;

        if (!g_apiReady)
            return false;

        const Il2CppType* type = g_api.class_get_type(g_monoBehaviourClass);
        Il2CppReflectionType* typeObject = type ? g_api.type_get_object(type) : nullptr;
        if (!typeObject)
            return false;

        void* args[1] = { typeObject };
        Il2CppException exc = nullptr;

        Il2CppObject array = g_api.runtime_invoke(g_findObjectsOfType, nullptr, args, &exc);
        if (exc || !array)
            return false;

        uint32_t length = g_api.array_length(array);
        void** elements = ArrayElements(array);

        for (uint32_t i = 0; i < length; i++)
        {
            void* instance = elements[i];
            if (!instance)
                continue;

            Il2CppField field = FindFieldInHierarchy(g_api, g_api.object_get_class(instance), fieldName);
            if (!field || !FieldIsFloat(field))
                continue;

            outObject = instance;
            outField = field;
            return true;
        }

        return false;
    }


    // ========================================================================
    // CONTROLLER -- one instance per controlled variable
    // ========================================================================
    // NOTE: Controller is an aggregate (no constructors), so the array below
    // can list-initialize each entry directly, e.g.:
    //   { "Camera Distance", FieldName1, 2.0f, 10.0f }
    // maps positionally to displayName, fieldName, multiplier, overrideValue.
    // Every member after overrideValue is runtime state -- leave those out
    // of the initializer entirely and they'll take their default values.

    struct Controller
    {
        // ---- config: set these when adding an entry below ----
        const char* displayName;
        const char* fieldName;
        float multiplier;
        float overrideValue;
        float lowerLimit;
        float upperLimit;
        bool showSlider;

        // ---- runtime state: leave alone ----
        std::atomic<bool> active{ false };
        void* object = nullptr;
        Il2CppField field = nullptr;
        bool ready = false;
        bool originalCached = false;
        bool touched = false;
        float originalValue = 0.0f;
        float lastTarget = 0.0f;
        DWORD nextScanTick = 0;   // per-instance now, not a shared function-local static


        bool Found() const
        {
            return object != nullptr && field != nullptr;
        }

        bool Locate()
        {
            object = nullptr;
            field = nullptr;
            return FindFieldAndObject(fieldName, object, field);
        }

        void CacheOriginal()
        {
            if (!Found() || originalCached)
                return;

            float value = 0.0f;
            g_api.field_get_value(object, field, &value);

            originalValue = value;
            originalCached = true;
        }

        float Read()
        {
            if (!Found())
                return 0.0f;

            float value = 0.0f;
            g_api.field_get_value(object, field, &value);
            return value;
        }

        void Write(float value)
        {
            if (!Found())
                return;

            g_api.field_set_value(object, field, &value);
            touched = true;
        }

        void Tick()
        {
            if (!ready)
                return;

            EnsureThreadAttached();

            if (!InstanceAlive(object) || !Found())
            {
                DWORD now = GetTickCount();
                if (now < nextScanTick)
                    return;
                nextScanTick = now + 1000;

                if (!Locate())
                    return;

                originalCached = false;
                touched = false;
                lastTarget = 0.0f;
            }

            CacheOriginal();

            if (!originalCached)
                return;

            float target = active.load(std::memory_order_relaxed)
                ? overrideValue * multiplier
                : originalValue;

            if (target != lastTarget)
            {
                Write(target);
                lastTarget = target;
            }
        }

        void Shutdown()
        {
            if (!ready)
                return;

            if (touched && originalCached && InstanceAlive(object))
                Write(originalValue);

            object = nullptr;
            field = nullptr;
            originalCached = false;
            touched = false;
            ready = false;
        }
    };


    // ========================================================================
    // THE LIST -- add a new controlled variable by adding ONE line here.
    // This also absorbs what used to live in Main.h/Main.cpp (display name,
    // field name, and slider limits) -- one file, one place, one line.
    // ========================================================================
    //                displayName              fieldName      mult  override  lower  upper  slider
    Controller g_controllers[] =
    {
        { "Camera Distance",                  "distance",     1.0f,  10.0f,  -1.0f,  50.0f, true },
        { "Camera Rotation Speed",            "rotateSpeed",  1.0f,  10.0f,   0.0f, 360.0f, true },
        { "Camera Rotation ",                 "yAngle",       1.0f,  10.0f,   0.0f, 360.0f, true },
        { "Stop Camera",                      "speed",        1.0f,  0.0f,    0.0f, 360.0f, false }

        // Example of adding a third one -- just uncomment/edit and rebuild,
        // nothing else in this file or the header needs to change:
        // { "Ski Break Force", "skiBreakForce", 1.0f, 0.0f, 0.0f, 100.0f },
    };

    constexpr size_t g_controllerCount = sizeof(g_controllers) / sizeof(g_controllers[0]);
}


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

        for (auto& c : g_controllers)
        {
            c.ready = true;
            c.Locate();

            if (c.Found())
                c.CacheOriginal();
        }

        return true;
    }

    void Tick()
    {
        for (auto& c : g_controllers)
            c.Tick();
    }

    void Shutdown()
    {
        for (auto& c : g_controllers)
            c.Shutdown();
    }

    size_t Count()
    {
        return g_controllerCount;
    }

    const char* NameAt(size_t index)
    {
        return g_controllers[index].displayName;
    }

    bool ReadyAt(size_t index)
    {
        return g_controllers[index].ready;
    }

    bool FoundAt(size_t index)
    {
        return g_controllers[index].Found();
    }

    bool IsActiveAt(size_t index)
    {
        return g_controllers[index].active.load(std::memory_order_relaxed);
    }

    void SetActiveAt(size_t index, bool value)
    {
        g_controllers[index].active.store(value, std::memory_order_relaxed);
    }

    float OverrideValueAt(size_t index)
    {
        return g_controllers[index].overrideValue;
    }

    void SetOverrideValueAt(size_t index, float value)
    {
        g_controllers[index].overrideValue = value;
    }

    float* OverridePtrAt(size_t index)
    {
        return &g_controllers[index].overrideValue;
    }

    float LowerLimitAt(size_t index)
    {
        return g_controllers[index].lowerLimit;
    }

    float UpperLimitAt(size_t index)
    {
        return g_controllers[index].upperLimit;
    }

    float CurrentValueAt(size_t index)
    {
        return g_controllers[index].Read();
    }

    bool ShowSliderAt(size_t index)
    {
        return g_controllers[index].showSlider;
    }
}
