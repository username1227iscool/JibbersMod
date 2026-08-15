// ============================================================================
// Jibbers DLL Injector
// ----------------------------------------------------------------------------
// Waits for a target process to appear, then (on an Insert keypress) injects
// a DLL into it using the classic CreateRemoteThread + LoadLibraryW technique.
//
// Usage:
//   injector.exe [dllPath] [processName]
//     dllPath      - defaults to "jibbers supa cool slo mo.dll" next to the exe
//     processName  - defaults to "Jibbers.exe"
// ============================================================================

#include <windows.h>
#include <tlhelp32.h>
#include <conio.h>
#include <iostream>
#include <string>
#include <filesystem>

namespace
{
    // ------------------------------------------------------------------
    // ASCII art banner printed on startup. Purely cosmetic.
    //
    // EXTENSION POINT: You could add a version string / build date here,
    // or swap the banner based on a command-line flag (e.g. --quiet to
    // skip printing it entirely).
    // ------------------------------------------------------------------
    const wchar_t* const kBanner[] = {
        L"  .-.                                       --.",
        L" .-++--.                                 --++-.",
        L" --+++++--.                           .-++++++.",
        L" .-+++++++--.                       --++++++++-.",
        L" .+++++++++++-.    ...........    --++++++++++-.",
        L" .++++---++++++--+++++++++++++++--++++++--++++-.",
        L" .-+++-----++++++++++++++++++++++++++-----++++-.",
        L" --++++--++++++++++++++++++++++++++++++---++++-",
        L" .-++++++++++++++---..    ..----+++++++++++++-.",
        L"  .++++++++++--.                 .--+++++++++-.",
        L"  .-+++++++-.                       .-+++++++.",
        L"  .-+++++--                           --------.",
        L" .-++++++-  .--.                   .--.",
        L" .++++++-    .++----..-..---. .----++-  .-..---.",
        L" -+++++-.    .-+-++-..-+++++-. -++-+-. -++++++-.",
        L".-+++++-.      .--.  .-+++++++-..--- .-+++++++-.",
        L".-+++++-.            .-++++++++-.   -+++++++++-.",
        L" -+++++-.            .-++++++++++-.-++++++++++-.",
        L" .-+++++-            .-+++++++++++++++++++++++-.",
        L" .-++++++-           .-+++++-++++++++++--+++++-.",
        L"  .-++++++-.         .-+++++-.-+++++++-.-+++++-.",
        L"   .-+++++++-.       .-+++++-  .-+++-.  -+++++-.",
        L"    .-++++++++--.    .-+++++-   .-+-.   -+++++-.",
        L"      .-+++++++++++-..-+++++-     .     -+++++-.",
        L"        .-+++++++++-..-+++++-           -+++++-.",
        L"          ...-+++++-..-+++++-           -+++++-.",
        L"              ...---..-------           -------.",
    };

    // Prints the banner line-by-line with blank lines before/after.
    void PrintBanner()
    {
        std::wcout << std::endl;
        for (const wchar_t* line : kBanner)
            std::wcout << line << std::endl;
        std::wcout << std::endl;
    }

    // ------------------------------------------------------------------
    // Returns the directory the running .exe lives in (with trailing
    // slash), used to build the default DLL path.
    //
    // EXTENSION POINT: This is a natural place to also look for a config
    // file (e.g. "settings.ini" or "settings.json") sitting next to the
    // exe, so dllPath / processName / hotkey could be configurable
    // without needing command-line args.
    // ------------------------------------------------------------------
    std::wstring ExeDirectory()
    {
        wchar_t path[MAX_PATH]{};
        GetModuleFileNameW(nullptr, path, MAX_PATH);
        std::wstring full(path);
        size_t slash = full.find_last_of(L"\\/");
        return slash == std::wstring::npos ? L"" : full.substr(0, slash + 1);
    }

    // ------------------------------------------------------------------
    // Scans running processes for one whose exe name matches `name`
    // (case-insensitive) and returns its PID, or 0 if not found.
    //
    // EXTENSION POINT: Could be extended to accept a list of candidate
    // process names (e.g. different game build names/regions) and
    // return the first match, instead of a single hardcoded name.
    // ------------------------------------------------------------------
    DWORD FindProcess(const std::wstring& name)
    {
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snapshot == INVALID_HANDLE_VALUE)
            return 0;

        PROCESSENTRY32W entry{};
        entry.dwSize = sizeof(entry);
        DWORD pid = 0;

        if (Process32FirstW(snapshot, &entry))
        {
            do
            {
                if (_wcsicmp(entry.szExeFile, name.c_str()) == 0)
                {
                    pid = entry.th32ProcessID;
                    break;
                }
            } while (Process32NextW(snapshot, &entry));
        }

        CloseHandle(snapshot);
        return pid;
    }

    // ------------------------------------------------------------------
    // Injects the DLL at `dllPath` into the process identified by `pid`
    // using the standard CreateRemoteThread + LoadLibraryW approach:
    //   1. Open the target process with the minimum required rights.
    //   2. Allocate a small buffer in its address space.
    //   3. Write the DLL path (wide string) into that buffer.
    //   4. Start a remote thread whose entry point is LoadLibraryW,
    //      passing the buffer as the argument -- this makes the target
    //      process load the DLL itself.
    //   5. Wait for that thread to finish and check its exit code
    //      (the HMODULE returned by LoadLibraryW, or 0 on failure).
    //
    // EXTENSION POINT: To inject more than one DLL, you could loop this
    // function over a list of paths. To support both 32-bit and 64-bit
    // builds of the target, you'd need to check the target process's
    // architecture (e.g. IsWow64Process) against the DLL's architecture
    // before attempting injection, since a mismatch will simply fail.
    // ------------------------------------------------------------------
    bool Inject(DWORD pid, const std::wstring& dllPath)
    {
        HANDLE process = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION |
            PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ, FALSE, pid);
        if (!process)
        {
            std::wcerr << L"OpenProcess failed (" << GetLastError() << L") -- run as administrator" << std::endl;
            return false;
        }

        // Allocate enough room in the target for the DLL path string,
        // including the null terminator.
        size_t bytes = (dllPath.size() + 1) * sizeof(wchar_t);
        void* remote = VirtualAllocEx(process, nullptr, bytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        bool ok = false;

        if (remote && WriteProcessMemory(process, remote, dllPath.c_str(), bytes, nullptr))
        {
            // kernel32.dll is loaded at the same base address across
            // processes in a session, so a local GetProcAddress lookup
            // gives a valid function pointer for the remote process too
            // (as long as both processes are the same bitness).
            auto loadLibrary = reinterpret_cast<LPTHREAD_START_ROUTINE>(
                GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "LoadLibraryW"));

            HANDLE thread = CreateRemoteThread(process, nullptr, 0, loadLibrary, remote, 0, nullptr);
            if (thread)
            {
                // EXTENSION POINT: 10 seconds is hardcoded here. Could be
                // made configurable, or replaced with a progress message
                // if you want feedback during a long wait.
                WaitForSingleObject(thread, 10000);

                DWORD exitCode = 0;
                GetExitCodeThread(thread, &exitCode);
                ok = exitCode != 0;
                if (!ok)
                    std::wcerr << L"LoadLibraryW returned null inside the game" << std::endl;

                CloseHandle(thread);
            }
            else
            {
                std::wcerr << L"CreateRemoteThread failed (" << GetLastError() << L")" << std::endl;
            }
        }

        if (remote)
            VirtualFreeEx(process, remote, 0, MEM_RELEASE);
        CloseHandle(process);
        return ok;
    }
}

// ------------------------------------------------------------------
// Blocks until Insert is pressed, released, then pressed again --
// i.e. waits for a fresh keypress rather than firing on a key that
// was already held down when the function was entered.
//
// EXTENSION POINT: The hotkey (VK_INSERT) is hardcoded. This could be
// made configurable via a command-line arg or config file if you want
// a different trigger key.
// ------------------------------------------------------------------
static void WaitForInsert()
{
    while (GetAsyncKeyState(VK_INSERT) & 0x8000)
        Sleep(30);

    while (!(GetAsyncKeyState(VK_INSERT) & 0x8000))
        Sleep(30);

    while (GetAsyncKeyState(VK_INSERT) & 0x8000)
        Sleep(30);
}

// Prints a "press any key to close" prompt and waits, so the console
// window doesn't vanish immediately when double-clicked from Explorer.
static int Finish(int code)
{
    std::wcout << std::endl << L"press any key to close" << std::endl;
    _getch();
    return code;
}

int wmain(int argc, wchar_t** argv)
{
    PrintBanner();

    // ------------------------------------------------------------------
    // Resolve DLL path / target process name, falling back to defaults
    // if not given on the command line.
    //
    // EXTENSION POINT: Could add proper flag parsing here (e.g. "--dll",
    // "--process", "--auto" to skip the Insert-key wait) instead of
    // relying on positional args.
    // ------------------------------------------------------------------
    std::wstring dllPath;

    if (argc > 1)
    {
        // DLL explicitly provided as an argument
        dllPath = argv[1];
    }
    else
    {
        // Automatically find the first .dll in the EXE directory
        for (const auto& entry : std::filesystem::directory_iterator(ExeDirectory()))
        {
            if (entry.path().extension() == L".dll")
            {
                dllPath = entry.path().wstring();
                break;
            }
        }
    }

    if (dllPath.empty())
    {
        std::wcerr << L"No DLL found.\n";
        return 1;
    }
    std::wstring processName = argc > 2 ? argv[2] : L"Jibbers.exe";

    if (GetFileAttributesW(dllPath.c_str()) == INVALID_FILE_ATTRIBUTES)
    {
        std::wcerr << L"dll not found: " << dllPath << std::endl;
        return Finish(1);
    }
    std::wcout << L"waiting for " << processName << L"..." << std::endl;

    // ------------------------------------------------------------------
    // Poll for the target process every 500ms, up to 5 minutes total
    // (600 * 500ms). Gives up and exits if it never shows up.
    //
    // EXTENSION POINT: Could replace polling with WMI/ETW process-start
    // notifications for a more efficient wait, or make the timeout
    // configurable.
    // ------------------------------------------------------------------
    DWORD pid = 0;
    for (int i = 0; i < 600 && pid == 0; i++)
    {
        pid = FindProcess(processName);
        if (!pid)
            Sleep(500);
    }

    if (!pid)
    {
        std::wcerr << processName << L" never started" << std::endl;
        return Finish(1);
    }

    std::wcout << std::endl << L"press insert to inject" << std::endl;
    std::wcout << L"(wait until you're actually in the game or dat shii will crash)" << std::endl;

    WaitForInsert();

    // Re-check the PID right before injecting, in case the target
    // process was closed/restarted while waiting for the keypress.
    if (DWORD current = FindProcess(processName))
        pid = current;
    else
    {
        std::wcerr << std::endl << processName << L" is no longer running" << std::endl;
        return Finish(1);
    }

    std::wcout << L"injecting into pid " << pid << std::endl;

    if (!Inject(pid, dllPath))
        return Finish(1);

    // EXTENSION POINT: After a successful injection you could loop back
    // to WaitForInsert() to allow re-injecting (e.g. after a DLL reload
    // during development) instead of exiting immediately.
    std::wcout << L"injected press insert key for gui" << std::endl;
    return Finish(0);
}