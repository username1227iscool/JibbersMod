#include "gui.h"
#include "overlay.h"
#include "VarCtl.h"
#include "controller.h"

#include <windows.h>
#include "MinHook.h"

namespace
{
    DWORD WINAPI InitThread(LPVOID)
    {
        Pad::Start();

        Overlay::Start(Gui::Frame);

        VarCtl::Init();
        return 0;
    }
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(module);
        CreateThread(nullptr, 0, InitThread, nullptr, 0, nullptr);
    }
    else if (reason == DLL_PROCESS_DETACH)
    {
        MH_DisableHook(MH_ALL_HOOKS);
        MH_Uninitialize();
    }

    return TRUE;
}
