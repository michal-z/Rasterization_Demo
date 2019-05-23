#include "External.h"
#include "Library.h"

static void Init_Descriptor_Heaps(GRAPHICS_CONTEXT& gfx);

void Init_Graphics_Context(HWND window, GRAPHICS_CONTEXT& gfx)
{
    IDXGIFactory4* factory;
#ifdef _DEBUG
    VHR(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&factory)));
#else
    VHR(CreateDXGIFactory2(0, IID_PPV_ARGS(&factory)));
#endif
#ifdef _DEBUG
    {
        ID3D12Debug* dbg;
        D3D12GetDebugInterface(IID_PPV_ARGS(&dbg));
        if (dbg)
        {
            dbg->EnableDebugLayer();
            ID3D12Debug1* dbg1;
            dbg->QueryInterface(IID_PPV_ARGS(&dbg1));
            if (dbg1)
            {
                dbg1->SetEnableGPUBasedValidation(TRUE);
            }
            SAFE_RELEASE(dbg);
            SAFE_RELEASE(dbg1);
        }
    }
#endif
    if (FAILED(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_1, IID_PPV_ARGS(&gfx.device))))
    {
        MessageBox(window, "This application requires Windows 10 1709 (RS3) or newer.", "D3D12CreateDevice failed", MB_OK | MB_ICONERROR);
        return;
    }

    D3D12_COMMAND_QUEUE_DESC cmdqueue_desc = {};
    cmdqueue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    cmdqueue_desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    cmdqueue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    VHR(gfx.device->CreateCommandQueue(&cmdqueue_desc, IID_PPV_ARGS(&gfx.cmdqueue)));

    DXGI_SWAP_CHAIN_DESC swapchain_desc = {};
    swapchain_desc.BufferCount = 4;
    swapchain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapchain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapchain_desc.OutputWindow = window;
    swapchain_desc.SampleDesc.Count = 1;
    swapchain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    swapchain_desc.Windowed = TRUE;

    IDXGISwapChain* temp_swapchain;
    VHR(factory->CreateSwapChain(gfx.cmdqueue, &swapchain_desc, &temp_swapchain));
    VHR(temp_swapchain->QueryInterface(IID_PPV_ARGS(&gfx.swapchain)));
    SAFE_RELEASE(temp_swapchain);
    SAFE_RELEASE(factory);

    RECT rect;
    GetClientRect(window, &rect);
    gfx.resolution[0] = (u32)rect.right;
    gfx.resolution[1] = (u32)rect.bottom;

    for (u32 i = 0; i < 2; ++i)
    {
        VHR(gfx.device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&gfx.cmdalloc[i])));
    }

    gfx.descriptor_size = gfx.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    gfx.descriptor_size_rtv = gfx.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    Init_Descriptor_Heaps(gfx);

    // swap buffer render targets
    {
        D3D12_CPU_DESCRIPTOR_HANDLE handle;
        Allocate_Descriptors(gfx, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 4, handle);

        for (u32 i = 0; i < 4; ++i)
        {
            VHR(gfx.swapchain->GetBuffer(i, IID_PPV_ARGS(&gfx.swapbuffers[i])));
            gfx.device->CreateRenderTargetView(gfx.swapbuffers[i], nullptr, handle);
            handle.ptr += gfx.descriptor_size_rtv;
        }
    }
    // depth-stencil target
    {
        auto image_desc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, gfx.resolution[0], gfx.resolution[1]);
        image_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL | D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
        VHR(gfx.device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE, &image_desc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &CD3DX12_CLEAR_VALUE(DXGI_FORMAT_D32_FLOAT, 1.0f, 0), IID_PPV_ARGS(&gfx.ds_buffer)));

        D3D12_CPU_DESCRIPTOR_HANDLE handle;
        Allocate_Descriptors(gfx, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, handle);

        D3D12_DEPTH_STENCIL_VIEW_DESC view_desc = {};
        view_desc.Format = DXGI_FORMAT_D32_FLOAT;
        view_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        view_desc.Flags = D3D12_DSV_FLAG_NONE;
        gfx.device->CreateDepthStencilView(gfx.ds_buffer, &view_desc, handle);
    }

    VHR(gfx.device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, gfx.cmdalloc[0], nullptr, IID_PPV_ARGS(&gfx.cmdlist)));
    gfx.cmdlist->Close();

    VHR(gfx.device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&gfx.frame_fence)));
    gfx.frame_fence_event = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
}

void Shutdown_Graphics_Context(GRAPHICS_CONTEXT& gfx)
{
    // @Incomplete: Release all resources.
    SAFE_RELEASE(gfx.cmdlist);
    SAFE_RELEASE(gfx.cmdalloc[0]);
    SAFE_RELEASE(gfx.cmdalloc[1]);
    SAFE_RELEASE(gfx.rt_heap.heap);
    SAFE_RELEASE(gfx.ds_heap.heap);
    for (u32 i = 0; i < 4; ++i)
    {
        SAFE_RELEASE(gfx.swapbuffers[i]);
    }
    CloseHandle(gfx.frame_fence_event);
    SAFE_RELEASE(gfx.frame_fence);
    SAFE_RELEASE(gfx.swapchain);
    SAFE_RELEASE(gfx.cmdqueue);
    SAFE_RELEASE(gfx.device);
}

void Present_Frame(GRAPHICS_CONTEXT& gfx, u32 swap_interval)
{
    gfx.swapchain->Present(swap_interval, 0);
    gfx.cmdqueue->Signal(gfx.frame_fence, ++gfx.frame_count);

    const u64 gpu_frame_count = gfx.frame_fence->GetCompletedValue();

    if ((gfx.frame_count - gpu_frame_count) >= 2)
    {
        gfx.frame_fence->SetEventOnCompletion(gpu_frame_count + 1, gfx.frame_fence_event);
        WaitForSingleObject(gfx.frame_fence_event, INFINITE);
    }

    gfx.frame_index = !gfx.frame_index;
    gfx.back_buffer_index = gfx.swapchain->GetCurrentBackBufferIndex();
    gfx.gpu_descriptor_heaps[gfx.frame_index].size = 0;
}

void Wait_For_GPU(GRAPHICS_CONTEXT& gfx)
{
    gfx.cmdqueue->Signal(gfx.frame_fence, ++gfx.frame_count);
    gfx.frame_fence->SetEventOnCompletion(gfx.frame_count, gfx.frame_fence_event);
    WaitForSingleObject(gfx.frame_fence_event, INFINITE);
}

DESCRIPTOR_HEAP& Get_Descriptor_Heap(GRAPHICS_CONTEXT& gfx, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags, u32& out_descriptor_size)
{
    if (type == D3D12_DESCRIPTOR_HEAP_TYPE_RTV)
    {
        out_descriptor_size = gfx.descriptor_size_rtv;
        return gfx.rt_heap;
    }
    else if (type == D3D12_DESCRIPTOR_HEAP_TYPE_DSV)
    {
        out_descriptor_size = gfx.descriptor_size_rtv;
        return gfx.ds_heap;
    }
    else if (type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
    {
        out_descriptor_size = gfx.descriptor_size;
        if (flags == D3D12_DESCRIPTOR_HEAP_FLAG_NONE)
        {
            return gfx.cpu_descriptor_heap;
        }
        else if (flags == D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)
        {
            return gfx.gpu_descriptor_heaps[gfx.frame_index];
        }
    }
    assert(0);
    out_descriptor_size = 0;
    return gfx.cpu_descriptor_heap;
}

static void Init_Descriptor_Heaps(GRAPHICS_CONTEXT& gfx)
{
    // render target descriptor heap (RTV)
    {
        gfx.rt_heap.capacity = 16;

        D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
        heap_desc.NumDescriptors = gfx.rt_heap.capacity;
        heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        VHR(gfx.device->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&gfx.rt_heap.heap)));
        gfx.rt_heap.cpu_start = gfx.rt_heap.heap->GetCPUDescriptorHandleForHeapStart();
    }
    // depth-stencil descriptor heap (DSV)
    {
        gfx.ds_heap.capacity = 8;

        D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
        heap_desc.NumDescriptors = gfx.ds_heap.capacity;
        heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        VHR(gfx.device->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&gfx.ds_heap.heap)));
        gfx.ds_heap.cpu_start = gfx.ds_heap.heap->GetCPUDescriptorHandleForHeapStart();
    }
    // non-shader visible descriptor heap (CBV, SRV, UAV)
    {
        gfx.cpu_descriptor_heap.capacity = 10000;

        D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
        heap_desc.NumDescriptors = gfx.cpu_descriptor_heap.capacity;
        heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        VHR(gfx.device->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&gfx.cpu_descriptor_heap.heap)));
        gfx.cpu_descriptor_heap.cpu_start = gfx.cpu_descriptor_heap.heap->GetCPUDescriptorHandleForHeapStart();
    }
    // shader visible descriptor heaps (CBV, SRV, UAV)
    {
        for (u32 i = 0; i < 2; ++i)
        {
            gfx.gpu_descriptor_heaps[i].capacity = 10000;

            D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
            heap_desc.NumDescriptors = gfx.gpu_descriptor_heaps[i].capacity;
            heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
            heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
            VHR(gfx.device->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&gfx.gpu_descriptor_heaps[i].heap)));

            gfx.gpu_descriptor_heaps[i].cpu_start = gfx.gpu_descriptor_heaps[i].heap->GetCPUDescriptorHandleForHeapStart();
            gfx.gpu_descriptor_heaps[i].gpu_start = gfx.gpu_descriptor_heaps[i].heap->GetGPUDescriptorHandleForHeapStart();
        }
    }
}

std::vector<u8> Load_File(const char* filename)
{
    FILE* file = fopen(filename, "rb");
    assert(file);
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    assert(size != -1);
    std::vector<u8> content(size);
    fseek(file, 0, SEEK_SET);
    fread(&content[0], 1, content.size(), file);
    fclose(file);
    return content;
}

void Update_Frame_Stats(HWND window, const char* name, f64& out_time, f32& out_delta_time)
{
    static f64 previous_time = -1.0;
    static f64 header_refresh_time = 0.0;
    static u32 frame_count = 0;

    if (previous_time < 0.0)
    {
        previous_time = Get_Time();
        header_refresh_time = previous_time;
    }

    out_time = Get_Time();
    out_delta_time = (f32)(out_time - previous_time);
    previous_time = out_time;

    if ((out_time - header_refresh_time) >= 1.0)
    {
        const f64 fps = frame_count / (out_time - header_refresh_time);
        const f64 ms = (1.0 / fps) * 1000.0;
        char header[256];
        snprintf(header, sizeof(header), "[%.1f fps  %.3f ms] %s", fps, ms, name);
        SetWindowText(window, header);
        header_refresh_time = out_time;
        frame_count = 0;
    }
    frame_count++;
}

f64 Get_Time()
{
    static LARGE_INTEGER start_counter;
    static LARGE_INTEGER frequency;
    if (start_counter.QuadPart == 0)
    {
        QueryPerformanceFrequency(&frequency);
        QueryPerformanceCounter(&start_counter);
    }
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return (counter.QuadPart - start_counter.QuadPart) / (f64)frequency.QuadPart;
}

static LRESULT CALLBACK Process_Window_Message(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
    switch (message)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    case WM_KEYDOWN:
        if (wparam == VK_ESCAPE)
        {
            PostQuitMessage(0);
            return 0;
        }
        break;
    }
    return DefWindowProc(window, message, wparam, lparam);
}

HWND Create_Window(const char* name, u32 width, u32 height)
{
    WNDCLASS winclass = {};
    winclass.lpfnWndProc = Process_Window_Message;
    winclass.hInstance = GetModuleHandle(nullptr);
    winclass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    winclass.lpszClassName = name;
    if (!RegisterClass(&winclass))
    {
        assert(0);
    }

    RECT rect = { 0, 0, (LONG)width, (LONG)height };
    if (!AdjustWindowRect(&rect, WS_OVERLAPPED | WS_SYSMENU | WS_CAPTION | WS_MINIMIZEBOX, 0))
    {
        assert(0);
    }

    HWND window = CreateWindowEx(0, name, name, WS_OVERLAPPED | WS_SYSMENU | WS_CAPTION | WS_MINIMIZEBOX | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, rect.right - rect.left, rect.bottom - rect.top, nullptr, nullptr, nullptr, 0);
    assert(window);

    return window;
}
