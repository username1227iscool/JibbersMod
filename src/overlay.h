#pragma once

namespace Overlay
{
    typedef void (*FrameFn)();

    void Start(FrameFn frameFn);
    void Stop();

    bool Ready();
    bool MenuOpen();
    void SetMenuOpen(bool open);
    void ToggleMenu();

    const char* RenderApiName();
}
