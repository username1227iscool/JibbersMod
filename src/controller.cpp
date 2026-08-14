#include "controller.h"

#include <windows.h>
#include <thread>
#include "MinHook.h"

namespace
{
    struct XGamepad
    {
        WORD  wButtons;
        BYTE  bLeftTrigger;
        BYTE  bRightTrigger;
        SHORT sThumbLX;
        SHORT sThumbLY;
        SHORT sThumbRX;
        SHORT sThumbRY;
    };

    struct XState
    {
        DWORD dwPacketNumber;
        XGamepad Gamepad;
    };

    typedef DWORD(WINAPI* XInputGetStateFn)(DWORD, XState*);

    HMODULE g_xinput = nullptr;
    XInputGetStateFn g_getState = nullptr;

    XInputGetStateFn g_originalGetState = nullptr;
    XInputGetStateFn g_originalGetStateEx = nullptr;
    void* g_getStateAddress = nullptr;
    void* g_getStateExAddress = nullptr;
    bool g_maskInstalled = false;
    HMODULE g_ownLoaded = nullptr;

    void MaskBoundButton(XState* state)
    {
        uint32_t mask = Pad::g_boundButton.load(std::memory_order_relaxed);
        if (!mask || !state)
            return;

        state->Gamepad.wButtons &= ~static_cast<WORD>(mask & 0xFFFF);

        if (mask & Pad::kLeftTrigger)
            state->Gamepad.bLeftTrigger = 0;
        if (mask & Pad::kRightTrigger)
            state->Gamepad.bRightTrigger = 0;
    }

    DWORD WINAPI HookedXInputGetState(DWORD index, XState* state)
    {
        DWORD result = g_originalGetState(index, state);
        if (result == ERROR_SUCCESS)
            MaskBoundButton(state);
        return result;
    }

    DWORD WINAPI HookedXInputGetStateEx(DWORD index, XState* state)
    {
        DWORD result = g_originalGetStateEx(index, state);
        if (result == ERROR_SUCCESS)
            MaskBoundButton(state);
        return result;
    }

    std::thread g_thread;
    std::atomic<bool> g_running{ false };

    std::atomic<int> g_padIndex{ -1 };
    std::atomic<uint32_t> g_held{ 0 };
    std::atomic<uint32_t> g_pressed{ 0 };
    std::atomic<uint32_t> g_captured{ 0 };
    std::atomic<bool> g_listening{ false };

    constexpr BYTE kTriggerThreshold = 60;

    const wchar_t* const kXInputNames[] = { L"xinput1_4.dll", L"xinput1_3.dll", L"xinput9_1_0.dll", L"xinput1_2.dll" };

    bool InstallMask()
    {
        HMODULE mod = nullptr;
        for (const wchar_t* name : kXInputNames)
        {
            HMODULE candidate = GetModuleHandleW(name);
            if (candidate && candidate != g_ownLoaded)
            {
                mod = candidate;
                break;
            }
        }

        if (!mod)
            return false;

        MH_STATUS status = MH_Initialize();
        if (status != MH_OK && status != MH_ERROR_ALREADY_INITIALIZED)
            return false;

        g_getStateAddress = reinterpret_cast<void*>(GetProcAddress(mod, "XInputGetState"));
        if (!g_getStateAddress)
            return false;

        if (MH_CreateHook(g_getStateAddress, reinterpret_cast<void*>(HookedXInputGetState),
                reinterpret_cast<void**>(&g_originalGetState)) != MH_OK ||
            MH_EnableHook(g_getStateAddress) != MH_OK)
            return false;

        if (void* ex = reinterpret_cast<void*>(GetProcAddress(mod, MAKEINTRESOURCEA(100))))
        {
            g_getStateExAddress = ex;
            if (MH_CreateHook(ex, reinterpret_cast<void*>(HookedXInputGetStateEx),
                    reinterpret_cast<void**>(&g_originalGetStateEx)) == MH_OK)
                MH_EnableHook(ex);
        }

        g_xinput = mod;
        g_getState = g_originalGetState;
        g_maskInstalled = true;
        return true;
    }

    bool LoadXInput()
    {
        if (InstallMask())
            return true;

        for (const wchar_t* name : kXInputNames)
        {
            HMODULE mod = LoadLibraryW(name);
            if (!mod)
                continue;

            auto fn = reinterpret_cast<XInputGetStateFn>(GetProcAddress(mod, "XInputGetState"));
            if (!fn)
                continue;

            g_ownLoaded = mod;
            g_xinput = mod;
            g_getState = fn;
            return true;
        }
        return false;
    }

    uint32_t ReadPad(int& outIndex)
    {
        int start = g_padIndex.load();
        for (int attempt = 0; attempt < 4; attempt++)
        {
            int i = (start >= 0) ? ((start + attempt) % 4) : attempt;

            XState state{};
            if (g_getState(static_cast<DWORD>(i), &state) != ERROR_SUCCESS)
                continue;

            uint32_t buttons = state.Gamepad.wButtons;
            if (state.Gamepad.bLeftTrigger >= kTriggerThreshold)
                buttons |= Pad::kLeftTrigger;
            if (state.Gamepad.bRightTrigger >= kTriggerThreshold)
                buttons |= Pad::kRightTrigger;

            outIndex = i;
            return buttons;
        }

        outIndex = -1;
        return 0;
    }

    void PollLoop()
    {
        uint32_t previous = 0;

        while (g_running.load())
        {
            if (Pad::g_boundButton.load() == 0 && !g_listening.load())
            {
                g_padIndex.store(-1);
                g_held.store(0);
                previous = 0;
                Sleep(250);
                continue;
            }

            if (!g_maskInstalled)
            {
                static DWORD nextTry = 0;
                DWORD now = GetTickCount();
                if (now >= nextTry)
                {
                    nextTry = now + 2000;
                    InstallMask();
                }
            }

            int index = -1;
            uint32_t buttons = ReadPad(index);

            g_padIndex.store(index);
            g_held.store(buttons);

            uint32_t edges = buttons & ~previous;
            previous = buttons;

            if (edges)
            {
                if (g_listening.load())
                {
                    uint32_t single = edges & (~edges + 1u);
                    g_captured.store(single);
                    g_listening.store(false);
                }
                else
                {
                    g_pressed.fetch_or(edges);
                }
            }

            Sleep(index >= 0 ? 8 : 500);
        }
    }
}

namespace Pad
{
    std::atomic<uint32_t> g_boundButton{ 0x0040 };

    const ButtonInfo kButtons[] = {
        { 0x1000, "A" },
        { 0x2000, "B" },
        { 0x4000, "X" },
        { 0x8000, "Y" },
        { 0x0100, "LB" },
        { 0x0200, "RB" },
        { kLeftTrigger,  "LT" },
        { kRightTrigger, "RT" },
        { 0x0040, "L3" },
        { 0x0080, "R3" },
        { 0x0020, "back" },
        { 0x0010, "start" },
        { 0x0001, "dpad up" },
        { 0x0002, "dpad down" },
        { 0x0004, "dpad left" },
        { 0x0008, "dpad right" },
    };

    const int kButtonCount = static_cast<int>(sizeof(kButtons) / sizeof(kButtons[0]));

    const char* NameOf(uint32_t mask)
    {
        for (int i = 0; i < kButtonCount; i++)
            if (kButtons[i].mask == mask)
                return kButtons[i].name;
        return "none";
    }

    void Start()
    {
        if (g_running.load())
            return;

        if (!LoadXInput())
        {
            return;
        }

        g_running.store(true);
        g_thread = std::thread(PollLoop);
    }

    void Stop()
    {
        if (!g_running.exchange(false))
            return;

        if (g_thread.joinable())
            g_thread.join();
    }

    bool Connected() { return g_padIndex.load() >= 0; }
    int  Index() { return g_padIndex.load(); }
    uint32_t HeldButtons() { return g_held.load(); }
    bool IsHeld(uint32_t mask) { return mask != 0 && (g_held.load() & mask) == mask; }

    bool ConsumePress(uint32_t mask)
    {
        if (mask == 0)
            return false;
        return (g_pressed.fetch_and(~mask) & mask) != 0;
    }

    void Listen()
    {
        g_captured.store(0);
        g_pressed.store(0);
        g_listening.store(true);
    }

    void CancelListen() { g_listening.store(false); }
    bool Listening() { return g_listening.load(); }
    uint32_t TakeCaptured() { return g_captured.exchange(0); }
}
