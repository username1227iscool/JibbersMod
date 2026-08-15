#pragma once
#include <atomic>

// ============================================================================
// EventHooks -- detects (and optionally intercepts) game events by
// redirecting the game's own compiled methods with MinHook, instead of
// polling a field every frame like VarCtl does.
// ============================================================================

namespace EventHooks
{
    bool Init();
    void Shutdown();

    // Example toggle: when true, the TakeDamage hook below suppresses the
    // real damage call entirely instead of just logging it. Flip this from
    // your GUI the same way VarCtl's active flags are flipped.
    extern std::atomic<bool> g_godMode;
}
