#include "Events.h"

#include "EventHooks.h"

#include <windows.h>
#include <cstdio>

void OnCustomContactFrictionAwake(
    void* self,
    void** args,
    std::size_t argCount)
{
    std::printf("[Event] CustomContactFriction.Awake fired!\n");
    fflush(stdout);

    (void)self;
    (void)args;
    (void)argCount;
}

namespace Events// EVENT DEFINITIONS
{
    bool Init()
    {
        if (!EventHooks::Init())
            return false;

        EventHooks::EventDefinition customContactFriction{};// ClothManager.Update

        customContactFriction.name = "CustomContactFriction.Awake";
        customContactFriction.namespaceName = "";
        customContactFriction.className = "CustomContactFriction";
        customContactFriction.methodName = "Awake";
        customContactFriction.argumentCount = 0;
        customContactFriction.callback = &OnCustomContactFrictionAwake;

        if (!EventHooks::RegisterEvent(customContactFriction))
        {
            std::printf("[Event] FAILED to hook CustomContactFriction.Awake!\n");
            std::fflush(stdout);

            return false;
        }

        std::printf("[Event] Hooked CustomContactFriction.Awake\n");
        std::fflush(stdout);

        return true;

    }
    void Shutdown(){EventHooks::Shutdown();}
}