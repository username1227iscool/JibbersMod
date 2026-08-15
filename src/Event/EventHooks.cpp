#include "EventHooks.h"

#include "Backend/il2cpp_api.h"
#include "MinHook.h"

#include <windows.h>

#include <atomic>
#include <cstddef>
#include <cstring>
#include <mutex>
#include <string>
#include <vector>

namespace
{
    // ========================================================================
    // IL2CPP
    // ========================================================================

    Il2CppApi g_api{};
    bool g_apiReady = false;

    // ========================================================================
    // Hook information
    // ========================================================================

    struct Hook
    {
        EventHooks::EventDefinition definition{};

        void* targetAddress = nullptr;
        void* originalFunction = nullptr;

        int argumentCount = 0;
    };

    std::vector<Hook> g_hooks;

    std::mutex g_hookMutex;

    bool g_initialized = false;

    // ========================================================================
    // Thread attachment
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

        // Already attached?
        if (g_api.thread_current &&
            g_api.thread_current())
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
    // Generic hook storage
    // ========================================================================
    //
    // IMPORTANT:
    //
    // MinHook jumps into a function with the SAME native ABI as the function
    // being hooked.
    //
    // We therefore provide several detours for different argument counts.
    //
    // The most useful common cases are:
    //
    //     void Foo()
    //     void Foo(void*)
    //     void Foo(void*, void*)
    //     void Foo(void*, void*, void*)
    //
    // The first argument is always "self".
    //
    // Additional arguments are passed through as void*.
    //
    // ========================================================================

    using Hook0 =
        void(__fastcall*)(void* self, void* methodInfo);

    using Hook1 =
        void(__fastcall*)(void* self, void* arg0, void* methodInfo);

    using Hook2 =
        void(__fastcall*)(
            void* self,
            void* arg0,
            void* arg1,
            void* methodInfo);

    using Hook3 =
        void(__fastcall*)(
            void* self,
            void* arg0,
            void* arg1,
            void* arg2,
            void* methodInfo);

    // ========================================================================
    // Find hook by original function
    // ========================================================================

    Hook* FindHook(void* address)
    {
        for (Hook& hook : g_hooks)
        {
            if (hook.targetAddress == address)
                return &hook;
        }

        return nullptr;
    }

    // ========================================================================
    // Dispatch
    // ========================================================================

    void Dispatch(
        Hook& hook,
        void* self,
        void** args,
        std::size_t argCount)
    {
        if (!hook.definition.callback)
            return;

        hook.definition.callback(
            self,
            args,
            argCount
        );
    }

    // ========================================================================
    // 0 argument detour
    // ========================================================================

    void __fastcall Detour0(
        void* self,
        void* methodInfo)
    {
        Hook* hook = FindHook(
            reinterpret_cast<void*>(&Detour0)
        );

        // FindHook cannot identify the original target here because MinHook
        // jumps directly to this detour. We therefore use the thread-local
        // dispatch target set immediately before the call.
    }

    // ========================================================================
    // Generic dispatch context
    // ========================================================================
    //
    // Because MinHook requires an actual native function signature, we use
    // thread-local state to tell the detour which registered hook it belongs
    // to.
    //
    // Each generated detour below has a fixed signature.
    //
    // ========================================================================

    thread_local Hook* g_currentHook = nullptr;

    // ========================================================================
    // 0 argument
    // ========================================================================

    void __fastcall EventDetour0(
        void* self,
        void* methodInfo)
    {
        Hook* hook = g_currentHook;

        if (hook)
        {
            Dispatch(
                *hook,
                self,
                nullptr,
                0
            );

            auto original =
                reinterpret_cast<Hook0>(
                    hook->originalFunction
                    );

            if (original)
            {
                original(
                    self,
                    methodInfo
                );
            }

            return;
        }
    }

    // ========================================================================
    // 1 argument
    // ========================================================================

    void __fastcall EventDetour1(
        void* self,
        void* arg0,
        void* methodInfo)
    {
        Hook* hook = g_currentHook;

        if (hook)
        {
            void* args[] =
            {
                arg0
            };

            Dispatch(
                *hook,
                self,
                args,
                1
            );

            auto original =
                reinterpret_cast<Hook1>(
                    hook->originalFunction
                    );

            if (original)
            {
                original(
                    self,
                    arg0,
                    methodInfo
                );
            }

            return;
        }
    }

    // ========================================================================
    // 2 arguments
    // ========================================================================

    void __fastcall EventDetour2(
        void* self,
        void* arg0,
        void* arg1,
        void* methodInfo)
    {
        Hook* hook = g_currentHook;

        if (hook)
        {
            void* args[] =
            {
                arg0,
                arg1
            };

            Dispatch(
                *hook,
                self,
                args,
                2
            );

            auto original =
                reinterpret_cast<Hook2>(
                    hook->originalFunction
                    );

            if (original)
            {
                original(
                    self,
                    arg0,
                    arg1,
                    methodInfo
                );
            }

            return;
        }
    }

    // ========================================================================
    // 3 arguments
    // ========================================================================

    void __fastcall EventDetour3(
        void* self,
        void* arg0,
        void* arg1,
        void* arg2,
        void* methodInfo)
    {
        Hook* hook = g_currentHook;

        if (hook)
        {
            void* args[] =
            {
                arg0,
                arg1,
                arg2
            };

            Dispatch(
                *hook,
                self,
                args,
                3
            );

            auto original =
                reinterpret_cast<Hook3>(
                    hook->originalFunction
                    );

            if (original)
            {
                original(
                    self,
                    arg0,
                    arg1,
                    arg2,
                    methodInfo
                );
            }

            return;
        }
    }

    // ========================================================================
    // IMPORTANT:
    //
    // MinHook detours are shared by argument count. We need to know which
    // EventDefinition caused the call.
    //
    // The clean solution is one generated trampoline per registered event.
    //
    // For the first implementation, we support a fixed number of event slots.
    // ========================================================================

    constexpr int MAX_EVENTS = 32;

    Hook* g_eventSlots[MAX_EVENTS]{};

    // ========================================================================
    // Slot detours
    // ========================================================================
    //
    // Each slot has its own function so we can identify the event without
    // relying on global "current hook" state.
    //
    // ========================================================================

#define DEFINE_EVENT_SLOT(N)                                                \
    void __fastcall EventSlot##N##0(void* self, void* methodInfo)           \
    {                                                                        \
        Hook* hook = g_eventSlots[N];                                       \
        if (!hook) return;                                                  \
                                                                             \
        if (hook->definition.callback)                                      \
        {                                                                    \
            hook->definition.callback(self, nullptr, 0);                    \
        }                                                                    \
                                                                             \
        auto original =                                                     \
            reinterpret_cast<Hook0>(hook->originalFunction);                \
                                                                             \
        if (original)                                                        \
            original(self, methodInfo);                                    \
    }                                                                        \
                                                                             \
    void __fastcall EventSlot##N##1(                                        \
        void* self,                                                         \
        void* arg0,                                                         \
        void* methodInfo)                                                   \
    {                                                                        \
        Hook* hook = g_eventSlots[N];                                       \
        if (!hook) return;                                                  \
                                                                             \
        void* args[] = { arg0 };                                            \
                                                                             \
        if (hook->definition.callback)                                      \
        {                                                                    \
            hook->definition.callback(self, args, 1);                       \
        }                                                                    \
                                                                             \
        auto original =                                                     \
            reinterpret_cast<Hook1>(hook->originalFunction);                \
                                                                             \
        if (original)                                                        \
            original(self, arg0, methodInfo);                              \
    }                                                                        \
                                                                             \
    void __fastcall EventSlot##N##2(                                        \
        void* self,                                                         \
        void* arg0,                                                         \
        void* arg1,                                                         \
        void* methodInfo)                                                   \
    {                                                                        \
        Hook* hook = g_eventSlots[N];                                       \
        if (!hook) return;                                                  \
                                                                             \
        void* args[] = { arg0, arg1 };                                      \
                                                                             \
        if (hook->definition.callback)                                      \
        {                                                                    \
            hook->definition.callback(self, args, 2);                       \
        }                                                                    \
                                                                             \
        auto original =                                                     \
            reinterpret_cast<Hook2>(hook->originalFunction);                \
                                                                             \
        if (original)                                                        \
            original(self, arg0, arg1, methodInfo);                        \
    }                                                                        \
                                                                             \
    void __fastcall EventSlot##N##3(                                        \
        void* self,                                                         \
        void* arg0,                                                         \
        void* arg1,                                                         \
        void* arg2,                                                         \
        void* methodInfo)                                                   \
    {                                                                        \
        Hook* hook = g_eventSlots[N];                                       \
        if (!hook) return;                                                  \
                                                                             \
        void* args[] = { arg0, arg1, arg2 };                                \
                                                                             \
        if (hook->definition.callback)                                      \
        {                                                                    \
            hook->definition.callback(self, args, 3);                       \
        }                                                                    \
                                                                             \
        auto original =                                                     \
            reinterpret_cast<Hook3>(hook->originalFunction);                \
                                                                             \
        if (original)                                                        \
            original(self, arg0, arg1, arg2, methodInfo);                  \
    }

    DEFINE_EVENT_SLOT(0)
        DEFINE_EVENT_SLOT(1)
        DEFINE_EVENT_SLOT(2)
        DEFINE_EVENT_SLOT(3)
        DEFINE_EVENT_SLOT(4)
        DEFINE_EVENT_SLOT(5)
        DEFINE_EVENT_SLOT(6)
        DEFINE_EVENT_SLOT(7)

#undef DEFINE_EVENT_SLOT

        using DetourGetter =
        void* (*)(int slot, int argumentCount);

#define GET_SLOT_CASE(N)                                                    \
        case N:                                                              \
            switch (argumentCount)                                          \
            {                                                                \
                case 0: return reinterpret_cast<void*>(&EventSlot##N##0);   \
                case 1: return reinterpret_cast<void*>(&EventSlot##N##1);   \
                case 2: return reinterpret_cast<void*>(&EventSlot##N##2);   \
                case 3: return reinterpret_cast<void*>(&EventSlot##N##3);   \
            }                                                                \
            break;

    void* GetDetour(
        int slot,
        int argumentCount)
    {
        switch (slot)
        {
            GET_SLOT_CASE(0)
                GET_SLOT_CASE(1)
                GET_SLOT_CASE(2)
                GET_SLOT_CASE(3)
                GET_SLOT_CASE(4)
                GET_SLOT_CASE(5)
                GET_SLOT_CASE(6)
                GET_SLOT_CASE(7)

        default:
            break;
        }

        return nullptr;
    }

#undef GET_SLOT_CASE

    // ========================================================================
    // Resolve IL2CPP method
    // ========================================================================

    bool ResolveMethod(
        const EventHooks::EventDefinition& definition,
        void*& address)
    {
        Il2CppClass klass =
            FindIl2CppClass(
                g_api,
                definition.namespaceName,
                definition.className
            );

        if (!klass)
            return false;

        Il2CppMethod method =
            g_api.class_get_method_from_name(
                klass,
                definition.methodName,
                definition.argumentCount
            );

        if (!method)
            return false;

        // This matches the Il2CppMethod representation used by the current
        // project.
        void* methodPointer =
            *reinterpret_cast<void**>(method);

        if (!methodPointer)
            return false;

        address = methodPointer;

        return true;
    }
}

// ============================================================================
// Public EventHooks API
// ============================================================================

namespace EventHooks
{
    bool Init()
    {
        if (g_initialized)
            return true;

        if (!LoadIl2CppApi(g_api, 60000))
            return false;

        g_apiReady = true;

        EnsureThreadAttached();

        MH_STATUS status = MH_Initialize();

        if (status != MH_OK &&
            status != MH_ERROR_ALREADY_INITIALIZED)
        {
            return false;
        }

        g_initialized = true;

        return true;
    }

    bool RegisterEvent(
        const EventDefinition& definition)
    {
        if (!g_initialized)
            return false;

        if (!definition.className ||
            !definition.methodName ||
            !definition.callback)
        {
            return false;
        }

        if (definition.argumentCount < 0 ||
            definition.argumentCount > 3)
        {
            return false;
        }

        std::lock_guard<std::mutex> lock(g_hookMutex);

        // Find free slot.
        int slot = -1;

        for (int i = 0; i < 8; ++i)
        {
            if (!g_eventSlots[i])
            {
                slot = i;
                break;
            }
        }

        if (slot < 0)
            return false;

        Hook hook{};

        hook.definition = definition;
        hook.argumentCount = definition.argumentCount;

        if (!ResolveMethod(
            definition,
            hook.targetAddress))
        {
            return false;
        }

        void* detour =
            GetDetour(
                slot,
                definition.argumentCount
            );

        if (!detour)
            return false;

        MH_STATUS status =
            MH_CreateHook(
                hook.targetAddress,
                detour,
                &hook.originalFunction
            );

        if (status != MH_OK)
            return false;

        g_hooks.push_back(hook);

        g_eventSlots[slot] = &g_hooks.back();

        status =
            MH_EnableHook(
                hook.targetAddress
            );

        if (status != MH_OK)
        {
            g_eventSlots[slot] = nullptr;

            g_hooks.pop_back();

            MH_RemoveHook(
                hook.targetAddress
            );

            return false;
        }

        return true;
    }

    void RemoveAllEvents()
    {
        std::lock_guard<std::mutex> lock(g_hookMutex);

        for (Hook& hook : g_hooks)
        {
            if (hook.targetAddress)
            {
                MH_DisableHook(
                    hook.targetAddress
                );

                MH_RemoveHook(
                    hook.targetAddress
                );
            }
        }

        for (auto& slot : g_eventSlots)
            slot = nullptr;

        g_hooks.clear();
    }

    void Shutdown()
    {
        if (!g_initialized)
            return;

        RemoveAllEvents();

        g_initialized = false;

        // Leave MH_Uninitialize() to your DLL shutdown code if that is where
        // your project currently handles MinHook shutdown.
    }
}