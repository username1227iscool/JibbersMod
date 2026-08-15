#include "EventHooks.h"
#include "Backend/il2cpp_api.h"
#include "MinHook.h"

#include <windows.h>
#include <atomic>

// ============================================================================
// EventHooks
//
// Hooks actual IL2CPP methods with MinHook instead of polling fields.
//
// IMPORTANT:
// 1. Set TARGET_NAMESPACE / TARGET_CLASS / TARGET_METHOD to the real IL2CPP
//    method you want to hook.
// 2. The TakeDamageFn signature MUST exactly match the game's native method
//    signature. The example below assumes:
//
//        void TakeDamage(float damage)
//
//    IL2CPP instance methods also receive the object pointer as the first
//    native argument and MethodInfo* as the final hidden argument.
// ============================================================================

namespace
{
    // ------------------------------------------------------------------------
    // IL2CPP API
    // ------------------------------------------------------------------------

    Il2CppApi g_api{};
    bool g_apiReady = false;

    // ------------------------------------------------------------------------
    // Target method
    // ------------------------------------------------------------------------

    constexpr const char* TARGET_NAMESPACE = "";
    constexpr const char* TARGET_CLASS = "UnityMessageListener";
    constexpr const char* TARGET_METHOD = "OnCollisionEnter";

    // ------------------------------------------------------------------------
    // IMPORTANT: Match this to the REAL native method signature.
    //
    // Example assumed IL2CPP C#:
    //
    //     public void TakeDamage(float damage)
    //
    // Native form:
    //
    //     void TakeDamage(void* thisPtr, float damage, void* methodInfo)
    //
    // On x64 Windows __fastcall is the normal ABI, but keeping it here makes
    // the intended calling convention explicit.
    // ------------------------------------------------------------------------

    using TakeDamageFn =
        void(__fastcall*)(void* self, float damage, void* methodInfo);

    TakeDamageFn g_originalTakeDamage = nullptr;
    void* g_targetAddress = nullptr;

    // ------------------------------------------------------------------------
    // IL2CPP thread attachment
    // ------------------------------------------------------------------------

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

    // ------------------------------------------------------------------------
    // TakeDamage detour
    // ------------------------------------------------------------------------
    //
    // This runs immediately when the game calls TakeDamage.
    //
    // g_godMode == false:
    //     run the game's original damage function.
    //
    // g_godMode == true:
    //     do NOT call the original function, completely suppressing damage.
    // ------------------------------------------------------------------------

    void __fastcall TakeDamageDetour(
        void* self,
        float damage,
        void* methodInfo)
    {
        // We have the real event and its arguments here.
        //
        // Example:
        //
        // OutputDebugStringA("TakeDamage called\n");

        if (EventHooks::g_godMode.load(std::memory_order_relaxed))
        {
            // Suppress the game's actual damage logic.
            return;
        }

        // Preserve normal game behavior.
        if (g_originalTakeDamage)
        {
            g_originalTakeDamage(self, damage, methodInfo);
        }
    }

    // ------------------------------------------------------------------------
    // Resolve target method
    // ------------------------------------------------------------------------

    bool ResolveTakeDamage()
    {
        Il2CppClass klass =
            FindIl2CppClass(
                g_api,
                TARGET_NAMESPACE,
                TARGET_CLASS
            );

        if (!klass)
            return false;

        Il2CppMethod method =
            g_api.class_get_method_from_name(
                klass,
                TARGET_METHOD,
                1
            );

        if (!method)
            return false;

        // The repo's Il2CppMethod typedef is currently void*, so method itself
        // points at the Il2CppMethod structure.
        //
        // In an IL2CPP build, methodPointer is stored at the beginning of the
        // method metadata structure used by this project. We read that native
        // pointer here so MinHook can hook the compiled function.
        //
        // If your game's Il2CppMethod layout differs, this is the one part
        // that must be adapted to the generated IL2CPP metadata layout.
        void* methodPointer =
            *reinterpret_cast<void**>(method);

        if (!methodPointer)
            return false;

        g_targetAddress = methodPointer;
        return true;
    }
}

// ============================================================================
// Public state
// ============================================================================

namespace EventHooks
{
    std::atomic<bool> g_godMode{ false };

    // ------------------------------------------------------------------------
    // Init
    // ------------------------------------------------------------------------

    bool Init()
    {
        if (g_targetAddress)
            return true;

        // Load GameAssembly / IL2CPP exports using the same loader as VarCtl.
        if (!LoadIl2CppApi(g_api, 60000))
            return false;

        g_apiReady = true;

        EnsureThreadAttached();

        if (!ResolveTakeDamage())
            return false;

        // MinHook must be initialized before creating hooks.
        MH_STATUS status = MH_Initialize();

        if (status != MH_OK &&
            status != MH_ERROR_ALREADY_INITIALIZED)
        {
            g_targetAddress = nullptr;
            return false;
        }

        status = MH_CreateHook(
            g_targetAddress,
            reinterpret_cast<void*>(&TakeDamageDetour),
            reinterpret_cast<void**>(&g_originalTakeDamage)
        );

        if (status != MH_OK)
        {
            g_targetAddress = nullptr;
            g_originalTakeDamage = nullptr;
            return false;
        }

        status = MH_EnableHook(g_targetAddress);

        if (status != MH_OK)
        {
            MH_RemoveHook(g_targetAddress);

            g_targetAddress = nullptr;
            g_originalTakeDamage = nullptr;
            return false;
        }

        return true;
    }

    // ------------------------------------------------------------------------
    // Shutdown
    // ------------------------------------------------------------------------

    void Shutdown()
    {
        if (!g_targetAddress)
            return;

        MH_DisableHook(g_targetAddress);
        MH_RemoveHook(g_targetAddress);

        g_targetAddress = nullptr;
        g_originalTakeDamage = nullptr;

        g_godMode.store(false, std::memory_order_relaxed);

        // Do NOT call MH_Uninitialize() here if dllmain.cpp is still doing it
        // globally on DLL_PROCESS_DETACH.
    }
}