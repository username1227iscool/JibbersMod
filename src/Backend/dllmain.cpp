#include "Gui/gui.h"
#include "Gui/overlay.h"
#include "VarCtl/VarCtl.h"
#include "controller.h"

#include "Event/EventHooks.h"
#include "Event/Events.h"

#ifdef _DEBUG
#include "Debug.h"
#endif

#include <windows.h>
#include <cstdio>
#include "MinHook.h"

namespace
{
    DWORD WINAPI InitThread(LPVOID)
    {
#ifdef _DEBUG
        InitDebugConsole();

        printf("[Jummpers] Debug build starting...\n");
#endif

        Pad::Start();

        Overlay::Start(Gui::Frame);

        VarCtl::Init();

        if (!Events::Init())
        {
#ifdef _DEBUG
            printf("[Jummpers] Event system initialization FAILED!\n");
#endif
        }
        else
        {
#ifdef _DEBUG
            printf("[Jummpers] Event system initialized.\n");
#endif
        }

        return 0;
    }
}

BOOL APIENTRY DllMain(
    HMODULE module,
    DWORD reason,
    LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(module);

        CreateThread(
            nullptr,
            0,
            InitThread,
            nullptr,
            0,
            nullptr
        );
    }
    else if (reason == DLL_PROCESS_DETACH)
    {
        Events::Shutdown();

        MH_DisableHook(MH_ALL_HOOKS);
        MH_Uninitialize();

#ifdef _DEBUG
        ShutdownDebugConsole();
#endif
    }

    return TRUE;
}