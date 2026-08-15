#pragma once

#include <cstddef>

namespace EventHooks
{
    // ------------------------------------------------------------------------
    // Event callback
    //
    // self      = IL2CPP object that received the method call
    // args      = native arguments after self
    // argCount  = number of arguments
    //
    // Example:
    //
    // OnCollisionEnter(Collision collision)
    //
    // args[0] = Collision*
    // ------------------------------------------------------------------------

    using EventCallback =
        void(*)(void* self, void** args, std::size_t argCount);

    // ------------------------------------------------------------------------
    // Event definition
    // ------------------------------------------------------------------------

    struct EventDefinition
    {
        const char* name;
        const char* namespaceName;
        const char* className;
        const char* methodName;

        // Number of explicit C# arguments.
        //
        // Example:
        // void OnCollisionEnter(Collision collision)
        //                                      ^ 1 argument
        //
        // void Foo()
        //        ^ 0 arguments
        int argumentCount;

        EventCallback callback;
    };

    // ------------------------------------------------------------------------
    // Initialization
    // ------------------------------------------------------------------------

    bool Init();

    void Shutdown();

    // ------------------------------------------------------------------------
    // Register an event
    //
    // Normally called by Events.cpp.
    // ------------------------------------------------------------------------

    bool RegisterEvent(const EventDefinition& definition);

    // ------------------------------------------------------------------------
    // Remove all registered hooks.
    // ------------------------------------------------------------------------

    void RemoveAllEvents();
}