#pragma once
#include <atomic>
#include <cstdint>

namespace Pad
{
    constexpr uint32_t kLeftTrigger = 0x10000;
    constexpr uint32_t kRightTrigger = 0x20000;

    struct ButtonInfo
    {
        uint32_t mask;
        const char* name;
    };

    extern const ButtonInfo kButtons[];
    extern const int kButtonCount;

    const char* NameOf(uint32_t mask);

    extern std::atomic<uint32_t> g_boundButton;

    void Start();
    void Stop();

    bool Connected();
    int  Index();
    uint32_t HeldButtons();

    bool ConsumePress(uint32_t mask);
    bool IsHeld(uint32_t mask);

    void Listen();
    void CancelListen();
    bool Listening();
    uint32_t TakeCaptured();
}
