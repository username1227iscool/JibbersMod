#include "VarCtlController.h"
#include "VarCtlUnity.h"
#include "Main.h"

#include <atomic>
#include <cstring>
#include <string>
#include <new>

namespace
{
    Controller* g_runtimeControllers = nullptr;
    size_t g_runtimeControllerCount = 0;
}

namespace VarCtlRuntime
{
    bool Create()
    {
        if (g_runtimeControllers)
            return true;

        if (g_controllers_count == 0)
            return false;

        g_runtimeControllerCount =
            g_controllers_count;

        g_runtimeControllers =
            new (std::nothrow)
            Controller[g_runtimeControllerCount];

        if (!g_runtimeControllers)
        {
            g_runtimeControllerCount = 0;
            return false;
        }

        for (size_t i = 0;
            i < g_runtimeControllerCount;
            ++i)
        {
            g_runtimeControllers[i].Configure(
                g_controllers[i]
            );
        }

        return true;
    }

    void Destroy()
    {
        delete[] g_runtimeControllers;

        g_runtimeControllers = nullptr;
        g_runtimeControllerCount = 0;
    }

    Controller* Data()
    {
        return g_runtimeControllers;
    }

    size_t Count()
    {
        return g_runtimeControllerCount;
    }

    Controller* At(size_t index)
    {
        if (!g_runtimeControllers ||
            index >= g_runtimeControllerCount)
        {
            return nullptr;
        }

        return &g_runtimeControllers[index];
    }
}