#include "Events.h"

#include "EventHooks.h"

#include <windows.h>
#include <cstdio>

// ============================================================================
// EVENT CALLBACKS
// ============================================================================
//
// Put the code you want to execute when each event happens here.
//
// ============================================================================



// ----------------------------------------------------------------------------
// Example event with no arguments
//
// C#:
//     void SomeEvent()
// ----------------------------------------------------------------------------

void OnClothManagerUpdate(
    void* self,
    void** args,
    std::size_t argCount)
{
    OutputDebugStringA(
        "[Events] ClothManager.Update detected!\n"
    );

    // ============================================================
    // YOUR CODE HERE
    // ============================================================

    std::printf("Cloth update");

    (void)self;
    (void)args;
    (void)argCount;
}

// ============================================================================
// EVENT DEFINITIONS
// ============================================================================

namespace Events
{
    bool Init()
    {
        if (!EventHooks::Init())
            return false;

        // ================================================================
        // ClothManager.Update
        // ================================================================

        EventHooks::EventDefinition clothUpdate{};

        clothUpdate.name =
            "ClothManager.Update";

        clothUpdate.namespaceName =
            "";

        clothUpdate.className =
            "ClothManager";

        clothUpdate.methodName =
            "Update";

        clothUpdate.argumentCount =
            0;

        clothUpdate.callback =
            &OnClothManagerUpdate;

        if (!EventHooks::RegisterEvent(clothUpdate))
            return false;

        return true;
    }

    void Shutdown()
    {
        EventHooks::Shutdown();
    }
}