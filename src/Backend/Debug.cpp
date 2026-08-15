#include "Debug.h"

#ifdef _DEBUG

#include <windows.h>
#include <cstdio>

void InitDebugConsole()
{
    if (!AllocConsole())
        return;

    FILE* stream = nullptr;

    freopen_s(
        &stream,
        "CONOUT$",
        "w",
        stdout
    );

    freopen_s(
        &stream,
        "CONOUT$",
        "w",
        stderr
    );

    freopen_s(
        &stream,
        "CONIN$",
        "r",
        stdin
    );

    SetConsoleTitleA("Jummpers Debug Console");

    printf("========================================\n");
    printf("       Jummpers DEBUG BUILD\n");
    printf("========================================\n");
    printf("Debug console initialized.\n");
}

void ShutdownDebugConsole()
{
    FreeConsole();
}

#endif