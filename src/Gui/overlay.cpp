#include "overlay.h"

#include <windows.h>
#include <d3d11.h>
#include <d3d12.h>
#include <dxgi.h>
#include <dxgi1_2.h>
#include <dxgi1_4.h>
#include <thread>
#include <vector>
#include <string>

#include "MinHook.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_dx12.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

namespace
{
    typedef HRESULT(WINAPI* PresentFn)(IDXGISwapChain*, UINT, UINT);
    typedef HRESULT(WINAPI* Present1Fn)(IDXGISwapChain1*, UINT, UINT, const DXGI_PRESENT_PARAMETERS*);
    typedef void(STDMETHODCALLTYPE* ExecuteCommandListsFn)(ID3D12CommandQueue*, UINT, ID3D12CommandList* const*);
    typedef LRESULT(CALLBACK* WndProcFn)(HWND, UINT, WPARAM, LPARAM);
    typedef BOOL(WINAPI* SetCursorPosFn)(int, int);
    typedef BOOL(WINAPI* ClipCursorFn)(const RECT*);

    enum class RenderApi { Unknown, D3D11, D3D12 };

    PresentFn g_originalPresent = nullptr;
    Present1Fn g_originalPresent1 = nullptr;
    ExecuteCommandListsFn g_originalExecuteCommandLists = nullptr;
    WndProcFn g_originalWndProc = nullptr;
    SetCursorPosFn g_originalSetCursorPos = nullptr;
    ClipCursorFn g_originalClipCursor = nullptr;

    RenderApi g_renderApi = RenderApi::Unknown;
    HWND g_window = nullptr;
    bool g_imguiReady = false;
    bool g_menuOpen = false;
    volatile bool g_shuttingDown = false;
    Overlay::FrameFn g_frameFn = nullptr;

    ID3D11Device* g_device = nullptr;
    ID3D11DeviceContext* g_context = nullptr;
    ID3D11RenderTargetView* g_rtv = nullptr;

    struct FrameContext
    {
        ID3D12CommandAllocator* CommandAllocator = nullptr;
        UINT64 FenceValue = 0;
    };

    struct DescriptorHeapAllocator
    {
        ID3D12DescriptorHeap* Heap = nullptr;
        D3D12_CPU_DESCRIPTOR_HANDLE HeapStartCpu{};
        D3D12_GPU_DESCRIPTOR_HANDLE HeapStartGpu{};
        UINT HandleIncrement = 0;
        std::vector<UINT> FreeIndices;

        void Create(ID3D12Device* device, ID3D12DescriptorHeap* heap)
        {
            Heap = heap;
            D3D12_DESCRIPTOR_HEAP_DESC desc = heap->GetDesc();
            HeapStartCpu = heap->GetCPUDescriptorHandleForHeapStart();
            HeapStartGpu = heap->GetGPUDescriptorHandleForHeapStart();
            HandleIncrement = device->GetDescriptorHandleIncrementSize(desc.Type);
            FreeIndices.clear();
            FreeIndices.reserve(desc.NumDescriptors);
            for (int n = static_cast<int>(desc.NumDescriptors); n > 0; n--)
                FreeIndices.push_back(static_cast<UINT>(n - 1));
        }

        void Alloc(D3D12_CPU_DESCRIPTOR_HANDLE* outCpu, D3D12_GPU_DESCRIPTOR_HANDLE* outGpu)
        {
            UINT idx = FreeIndices.back();
            FreeIndices.pop_back();
            outCpu->ptr = HeapStartCpu.ptr + static_cast<SIZE_T>(idx) * HandleIncrement;
            outGpu->ptr = HeapStartGpu.ptr + static_cast<UINT64>(idx) * HandleIncrement;
        }

        void Free(D3D12_CPU_DESCRIPTOR_HANDLE cpu, D3D12_GPU_DESCRIPTOR_HANDLE)
        {
            UINT idx = static_cast<UINT>((cpu.ptr - HeapStartCpu.ptr) / HandleIncrement);
            FreeIndices.push_back(idx);
        }
    };

    ID3D12Device* g_d3d12Device = nullptr;
    ID3D12CommandQueue* g_d3d12Queue = nullptr;
    IDXGISwapChain3* g_swapChain3 = nullptr;
    ID3D12DescriptorHeap* g_rtvHeap = nullptr;
    ID3D12DescriptorHeap* g_srvHeap = nullptr;
    ID3D12GraphicsCommandList* g_d3d12CmdList = nullptr;
    ID3D12Fence* g_fence = nullptr;
    HANDLE g_fenceEvent = nullptr;
    UINT64 g_fenceValue = 0;
    D3D12_CPU_DESCRIPTOR_HANDLE g_rtvHeapStart{};
    UINT g_rtvIncrement = 0;
    std::vector<FrameContext> g_frames;
    DescriptorHeapAllocator g_srvAllocator;

    bool IsMouseMessage(UINT msg)
    {
        switch (msg)
        {
        case WM_MOUSEMOVE:
        case WM_LBUTTONDOWN: case WM_LBUTTONUP: case WM_LBUTTONDBLCLK:
        case WM_RBUTTONDOWN: case WM_RBUTTONUP: case WM_RBUTTONDBLCLK:
        case WM_MBUTTONDOWN: case WM_MBUTTONUP: case WM_MBUTTONDBLCLK:
        case WM_XBUTTONDOWN: case WM_XBUTTONUP: case WM_XBUTTONDBLCLK:
        case WM_MOUSEWHEEL: case WM_MOUSEHWHEEL:
            return true;
        }
        return false;
    }

    bool IsKeyboardMessage(UINT msg)
    {
        switch (msg)
        {
        case WM_KEYDOWN: case WM_KEYUP:
        case WM_SYSKEYDOWN: case WM_SYSKEYUP:
        case WM_CHAR: case WM_SYSCHAR:
        case WM_DEADCHAR: case WM_SYSDEADCHAR:
            return true;
        }
        return false;
    }

    constexpr int kMenuKey = VK_INSERT;

    bool IsMenuKeyMessage(UINT msg, WPARAM wParam)
    {
        if (static_cast<int>(wParam) != kMenuKey)
            return false;

        return msg == WM_KEYDOWN || msg == WM_KEYUP || msg == WM_SYSKEYDOWN || msg == WM_SYSKEYUP;
    }

    LRESULT CALLBACK HookedWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        if (IsMenuKeyMessage(msg, wParam))
        {
            if (msg == WM_KEYUP || msg == WM_SYSKEYUP)
                Overlay::ToggleMenu();
            return 0;
        }

        if (g_menuOpen && g_imguiReady)
        {
            ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam);

            if (IsMouseMessage(msg) || IsKeyboardMessage(msg))
                return 0;

            if (msg == WM_INPUT)
                return DefWindowProcW(hwnd, msg, wParam, lParam);
        }

        return CallWindowProcW(g_originalWndProc, hwnd, msg, wParam, lParam);
    }

    BOOL WINAPI HookedSetCursorPos(int x, int y)
    {
        if (g_menuOpen)
            return TRUE;
        return g_originalSetCursorPos(x, y);
    }

    RECT g_gameClipRect{};
    bool g_gameWantsClip = false;

    BOOL WINAPI HookedClipCursor(const RECT* rect)
    {
        g_gameWantsClip = rect != nullptr;
        if (rect)
            g_gameClipRect = *rect;

        if (g_menuOpen)
            return g_originalClipCursor(nullptr);
        return g_originalClipCursor(rect);
    }

    void RestoreGameCursorClip()
    {
        if (g_gameWantsClip && g_originalClipCursor)
            g_originalClipCursor(&g_gameClipRect);
    }

    void RenderBody()
    {
        ImGuiIO& io = ImGui::GetIO();
        io.MouseDrawCursor = g_menuOpen;

        if (g_menuOpen && g_originalClipCursor)
            g_originalClipCursor(nullptr);

        if (g_frameFn)
            g_frameFn();
    }

    void SetupRenderTarget(IDXGISwapChain* swapChain)
    {
        ID3D11Texture2D* backBuffer = nullptr;
        swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer));
        if (backBuffer)
        {
            g_device->CreateRenderTargetView(backBuffer, nullptr, &g_rtv);
            backBuffer->Release();
        }
    }

    void ConfigureImGui()
    {
        ImGui::CreateContext();

        ImGui::GetIO().IniFilename = nullptr;
    }

    bool InitD3D11(IDXGISwapChain* swapChain)
    {
        DXGI_SWAP_CHAIN_DESC desc{};
        swapChain->GetDesc(&desc);
        g_window = desc.OutputWindow;

        if (FAILED(swapChain->GetDevice(__uuidof(ID3D11Device), reinterpret_cast<void**>(&g_device))))
            return false;
        g_device->GetImmediateContext(&g_context);

        SetupRenderTarget(swapChain);

        ConfigureImGui();
        ImGui_ImplWin32_Init(g_window);
        ImGui_ImplDX11_Init(g_device, g_context);

        g_originalWndProc = reinterpret_cast<WndProcFn>(
            SetWindowLongPtrW(g_window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(HookedWndProc)));

        return true;
    }

    void RenderD3D11()
    {
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        RenderBody();

        ImGui::Render();
        g_context->OMSetRenderTargets(1, &g_rtv, nullptr);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    }

    void SrvDescriptorAlloc(ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE* outCpu, D3D12_GPU_DESCRIPTOR_HANDLE* outGpu)
    {
        g_srvAllocator.Alloc(outCpu, outGpu);
    }

    void SrvDescriptorFree(ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE cpu, D3D12_GPU_DESCRIPTOR_HANDLE gpu)
    {
        g_srvAllocator.Free(cpu, gpu);
    }

    bool InitD3D12(IDXGISwapChain* swapChain)
    {
        if (!g_d3d12Queue)
            return false;

        if (FAILED(swapChain->QueryInterface(IID_PPV_ARGS(&g_swapChain3))))
            return false;

        DXGI_SWAP_CHAIN_DESC desc{};
        swapChain->GetDesc(&desc);
        g_window = desc.OutputWindow;
        UINT bufferCount = desc.BufferCount;
        DXGI_FORMAT rtvFormat = desc.BufferDesc.Format;

        if (FAILED(swapChain->GetDevice(IID_PPV_ARGS(&g_d3d12Device))))
            return false;

        D3D12_DESCRIPTOR_HEAP_DESC rtvDesc{};
        rtvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvDesc.NumDescriptors = bufferCount;
        rtvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        if (FAILED(g_d3d12Device->CreateDescriptorHeap(&rtvDesc, IID_PPV_ARGS(&g_rtvHeap))))
            return false;
        g_rtvHeapStart = g_rtvHeap->GetCPUDescriptorHandleForHeapStart();
        g_rtvIncrement = g_d3d12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

        D3D12_DESCRIPTOR_HEAP_DESC srvDesc{};
        srvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        srvDesc.NumDescriptors = 64;
        srvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        if (FAILED(g_d3d12Device->CreateDescriptorHeap(&srvDesc, IID_PPV_ARGS(&g_srvHeap))))
            return false;
        g_srvAllocator.Create(g_d3d12Device, g_srvHeap);

        g_frames.resize(bufferCount);
        for (UINT i = 0; i < bufferCount; i++)
        {
            if (FAILED(g_d3d12Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&g_frames[i].CommandAllocator))))
                return false;
        }

        if (FAILED(g_d3d12Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, g_frames[0].CommandAllocator, nullptr, IID_PPV_ARGS(&g_d3d12CmdList))))
            return false;
        g_d3d12CmdList->Close();

        if (FAILED(g_d3d12Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&g_fence))))
            return false;
        g_fenceEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
        if (!g_fenceEvent)
            return false;

        ConfigureImGui();
        ImGui_ImplWin32_Init(g_window);

        ImGui_ImplDX12_InitInfo initInfo{};
        initInfo.Device = g_d3d12Device;
        initInfo.CommandQueue = g_d3d12Queue;
        initInfo.NumFramesInFlight = static_cast<int>(bufferCount);
        initInfo.RTVFormat = rtvFormat;
        initInfo.SrvDescriptorHeap = g_srvHeap;
        initInfo.SrvDescriptorAllocFn = SrvDescriptorAlloc;
        initInfo.SrvDescriptorFreeFn = SrvDescriptorFree;
        if (!ImGui_ImplDX12_Init(&initInfo))
            return false;

        g_originalWndProc = reinterpret_cast<WndProcFn>(
            SetWindowLongPtrW(g_window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(HookedWndProc)));

        return true;
    }

    void RenderD3D12(IDXGISwapChain* swapChain)
    {
        UINT idx = g_swapChain3->GetCurrentBackBufferIndex();
        if (idx >= g_frames.size())
            return;

        FrameContext& frame = g_frames[idx];

        if (frame.FenceValue != 0 && g_fence->GetCompletedValue() < frame.FenceValue)
        {
            g_fence->SetEventOnCompletion(frame.FenceValue, g_fenceEvent);
            WaitForSingleObject(g_fenceEvent, INFINITE);
        }

        ID3D12Resource* backBuffer = nullptr;
        if (FAILED(swapChain->GetBuffer(idx, IID_PPV_ARGS(&backBuffer))) || !backBuffer)
            return;

        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        RenderBody();

        ImGui::Render();

        D3D12_CPU_DESCRIPTOR_HANDLE rtv = g_rtvHeapStart;
        rtv.ptr += static_cast<SIZE_T>(idx) * g_rtvIncrement;
        g_d3d12Device->CreateRenderTargetView(backBuffer, nullptr, rtv);

        frame.CommandAllocator->Reset();
        g_d3d12CmdList->Reset(frame.CommandAllocator, nullptr);

        D3D12_RESOURCE_BARRIER barrier{};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = backBuffer;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        g_d3d12CmdList->ResourceBarrier(1, &barrier);

        g_d3d12CmdList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
        g_d3d12CmdList->SetDescriptorHeaps(1, &g_srvHeap);
        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), g_d3d12CmdList);

        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
        g_d3d12CmdList->ResourceBarrier(1, &barrier);

        g_d3d12CmdList->Close();
        ID3D12CommandList* lists[] = { g_d3d12CmdList };
        g_d3d12Queue->ExecuteCommandLists(1, lists);

        backBuffer->Release();

        frame.FenceValue = ++g_fenceValue;
        g_d3d12Queue->Signal(g_fence, frame.FenceValue);
    }

    void STDMETHODCALLTYPE HookedExecuteCommandLists(ID3D12CommandQueue* queue, UINT count, ID3D12CommandList* const* lists)
    {
        if (!g_d3d12Queue && queue)
        {
            D3D12_COMMAND_QUEUE_DESC desc = queue->GetDesc();
            if (desc.Type == D3D12_COMMAND_LIST_TYPE_DIRECT)
                g_d3d12Queue = queue;
        }
        g_originalExecuteCommandLists(queue, count, lists);
    }

    void DetectRenderApi(IDXGISwapChain* swapChain)
    {
        ID3D12Device* dev12 = nullptr;
        if (SUCCEEDED(swapChain->GetDevice(__uuidof(ID3D12Device), reinterpret_cast<void**>(&dev12))) && dev12)
        {
            g_renderApi = RenderApi::D3D12;
            dev12->Release();
            return;
        }

        ID3D11Device* dev11 = nullptr;
        if (SUCCEEDED(swapChain->GetDevice(__uuidof(ID3D11Device), reinterpret_cast<void**>(&dev11))) && dev11)
        {
            g_renderApi = RenderApi::D3D11;
            dev11->Release();
        }
    }

    void RenderFrame(IDXGISwapChain* swapChain)
    {
        if (g_shuttingDown)
            return;

        static bool inRender = false;
        if (inRender)
            return;
        inRender = true;

        if (g_renderApi == RenderApi::Unknown)
            DetectRenderApi(swapChain);

        if (g_renderApi == RenderApi::D3D11)
        {
            if (!g_imguiReady)
                g_imguiReady = InitD3D11(swapChain);
            if (g_imguiReady)
                RenderD3D11();
        }
        else if (g_renderApi == RenderApi::D3D12)
        {
            if (!g_imguiReady)
                g_imguiReady = InitD3D12(swapChain);
            if (g_imguiReady)
                RenderD3D12(swapChain);
        }

        inRender = false;
    }

    HRESULT WINAPI HookedPresent(IDXGISwapChain* swapChain, UINT syncInterval, UINT flags)
    {
        RenderFrame(swapChain);
        return g_originalPresent(swapChain, syncInterval, flags);
    }

    HRESULT WINAPI HookedPresent1(IDXGISwapChain1* swapChain, UINT syncInterval, UINT flags, const DXGI_PRESENT_PARAMETERS* params)
    {
        RenderFrame(swapChain);
        return g_originalPresent1(swapChain, syncInterval, flags, params);
    }

    bool ResolvePresentAddresses(void** outPresent, void** outPresent1)
    {
        WNDCLASSEXW wc{};
        wc.cbSize = sizeof(wc);
        wc.lpfnWndProc = DefWindowProcW;
        wc.hInstance = GetModuleHandleW(nullptr);
        wc.lpszClassName = L"JibbersTimeDummyWindow";
        RegisterClassExW(&wc);

        HWND dummyWindow = CreateWindowExW(0, wc.lpszClassName, L"", WS_OVERLAPPEDWINDOW, 0, 0, 100, 100, nullptr, nullptr, wc.hInstance, nullptr);

        DXGI_SWAP_CHAIN_DESC desc{};
        desc.BufferCount = 2;
        desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        desc.OutputWindow = dummyWindow;
        desc.SampleDesc.Count = 1;
        desc.Windowed = TRUE;
        desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

        IDXGISwapChain* swapChain = nullptr;
        ID3D11Device* device = nullptr;
        ID3D11DeviceContext* context = nullptr;
        D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;

        HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, &featureLevel, 1,
            D3D11_SDK_VERSION, &desc, &swapChain, &device, nullptr, &context);

        bool ok = false;
        if (SUCCEEDED(hr))
        {
            void** vtable = *reinterpret_cast<void***>(swapChain);
            *outPresent = vtable[8];

            IDXGISwapChain1* swapChain1 = nullptr;
            if (SUCCEEDED(swapChain->QueryInterface(IID_PPV_ARGS(&swapChain1))) && swapChain1)
            {
                void** vtable1 = *reinterpret_cast<void***>(swapChain1);
                *outPresent1 = vtable1[22];
                swapChain1->Release();
            }

            ok = true;
            swapChain->Release();
            device->Release();
            context->Release();
        }

        DestroyWindow(dummyWindow);
        UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return ok;
    }

    bool ResolveExecuteCommandListsAddress(void** outAddress)
    {
        HMODULE d3d12 = LoadLibraryW(L"d3d12.dll");
        if (!d3d12)
            return false;

        auto createDevice = reinterpret_cast<PFN_D3D12_CREATE_DEVICE>(GetProcAddress(d3d12, "D3D12CreateDevice"));
        if (!createDevice)
            return false;

        ID3D12Device* device = nullptr;
        if (FAILED(createDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device))) || !device)
            return false;

        D3D12_COMMAND_QUEUE_DESC qd{};
        qd.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        ID3D12CommandQueue* queue = nullptr;
        bool ok = false;
        if (SUCCEEDED(device->CreateCommandQueue(&qd, IID_PPV_ARGS(&queue))) && queue)
        {
            void** vtable = *reinterpret_cast<void***>(queue);
            *outAddress = vtable[10];
            ok = true;
            queue->Release();
        }

        device->Release();
        return ok;
    }

    void HookThread()
    {
        void* presentAddress = nullptr;
        void* present1Address = nullptr;
        for (int i = 0; i < 200 && !presentAddress; i++)
        {
            if (ResolvePresentAddresses(&presentAddress, &present1Address))
                break;
            Sleep(50);
        }

        if (!presentAddress)
        {
            return;
        }

        MH_STATUS status = MH_Initialize();
        if (status != MH_OK && status != MH_ERROR_ALREADY_INITIALIZED)
        {
            return;
        }

        if (MH_CreateHook(presentAddress, reinterpret_cast<void*>(HookedPresent), reinterpret_cast<void**>(&g_originalPresent)) != MH_OK)
        {
            return;
        }

        if (present1Address)
            MH_CreateHook(present1Address, reinterpret_cast<void*>(HookedPresent1), reinterpret_cast<void**>(&g_originalPresent1));

        void* executeAddress = nullptr;
        if (ResolveExecuteCommandListsAddress(&executeAddress))
            MH_CreateHook(executeAddress, reinterpret_cast<void*>(HookedExecuteCommandLists), reinterpret_cast<void**>(&g_originalExecuteCommandLists));

        MH_CreateHookApi(L"user32", "SetCursorPos", reinterpret_cast<void*>(HookedSetCursorPos), reinterpret_cast<void**>(&g_originalSetCursorPos));
        MH_CreateHookApi(L"user32", "ClipCursor", reinterpret_cast<void*>(HookedClipCursor), reinterpret_cast<void**>(&g_originalClipCursor));

        MH_EnableHook(MH_ALL_HOOKS);
    }
}

namespace Overlay
{
    void Start(FrameFn frameFn)
    {
        g_frameFn = frameFn;
        std::thread(HookThread).detach();
    }

    void Stop()
    {
        g_shuttingDown = true;
        Sleep(200);

        if (g_window && g_originalWndProc)
        {
            SetWindowLongPtrW(g_window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(g_originalWndProc));
            g_originalWndProc = nullptr;
        }

        g_menuOpen = false;
        g_frameFn = nullptr;
    }

    bool Ready() { return g_imguiReady; }
    bool MenuOpen() { return g_menuOpen; }

    void SetMenuOpen(bool open)
    {
        bool wasOpen = g_menuOpen;
        g_menuOpen = open;

        if (wasOpen && !open)
            RestoreGameCursorClip();
    }

    void ToggleMenu() { SetMenuOpen(!g_menuOpen); }

    const char* RenderApiName()
    {
        switch (g_renderApi)
        {
        case RenderApi::D3D11: return "D3D11";
        case RenderApi::D3D12: return "D3D12";
        default: return "unknown";
        }
    }
}
