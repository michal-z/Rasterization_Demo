#include "External.h"
#include "Common.h"
#include "Library.h"

#define DEMO_NAME "Rasterization"
#define NUM_GRAPHICS_PIPELINES 4
#define NUM_TRIANGLES 1
#define FRAGMENTS_RT_RESOLUTION 256

struct FRAGMENTS
{
    ID3D12Resource *buffer;
    D3D12_CPU_DESCRIPTOR_HANDLE buffer_uav;
    D3D12_CPU_DESCRIPTOR_HANDLE buffer_srv;
    ID3D12Resource *target;
    D3D12_CPU_DESCRIPTOR_HANDLE target_rtv;
    D3D12_CPU_DESCRIPTOR_HANDLE target_srv;
    u32 counter;
};

struct DEMO_ROOT
{
    GRAPHICS_CONTEXT gfx;
    FRAGMENTS frags;
    ID3D12PipelineState *pipelines[NUM_GRAPHICS_PIPELINES];
    ID3D12RootSignature *root_signatures[NUM_GRAPHICS_PIPELINES];
    ID3D12Resource *geometry_buffer;
};

static void
Demo_Update(DEMO_ROOT &root)
{
    GRAPHICS_CONTEXT &gfx = root.gfx;
    FRAGMENTS &frags = root.frags;

    ID3D12GraphicsCommandList2 *cmdlist = Get_And_Reset_Command_List(gfx);

    Bind_GPU_Descriptor_Heap(gfx);


    cmdlist->RSSetViewports(1, &D3D12_VIEWPORT{ 0.0f, 0.0f, (f32)FRAGMENTS_RT_RESOLUTION, (f32)FRAGMENTS_RT_RESOLUTION, 0.0f, 1.0f });
    cmdlist->RSSetScissorRects(1, &D3D12_RECT{ 0, 0, (LONG)FRAGMENTS_RT_RESOLUTION, (LONG)FRAGMENTS_RT_RESOLUTION });

    ID3D12Resource *depth_buffer;
    D3D12_CPU_DESCRIPTOR_HANDLE depth_buffer_dsv;
    Get_Depth_Stencil_Buffer(gfx, depth_buffer, depth_buffer_dsv);

    cmdlist->OMSetRenderTargets(1, &frags.target_rtv, TRUE, &depth_buffer_dsv);
    cmdlist->ClearRenderTargetView(frags.target_rtv, XMVECTORF32{ 0.0f, 0.0f, 0.0f, 0.0f }, 0, nullptr);
    cmdlist->ClearDepthStencilView(depth_buffer_dsv, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    if (frags.counter == 0)
    {
        // clear atomic counter in the fragments buffer
        cmdlist->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(frags.buffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_DEST));

        D3D12_WRITEBUFFERIMMEDIATE_PARAMETER param = { frags.buffer->GetGPUVirtualAddress(), 0 };
        cmdlist->WriteBufferImmediate(1, &param, nullptr);

        cmdlist->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(frags.buffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));


        cmdlist->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
        cmdlist->SetPipelineState(root.pipelines[0]);
        cmdlist->SetGraphicsRootSignature(root.root_signatures[0]);
        cmdlist->SetGraphicsRootDescriptorTable(0, Copy_Descriptors_To_GPU(gfx, 1, frags.buffer_uav));

        D3D12_VERTEX_BUFFER_VIEW vb_view = {};
        vb_view.BufferLocation = root.geometry_buffer->GetGPUVirtualAddress();
        vb_view.StrideInBytes = sizeof(FRAGMENT);
        vb_view.SizeInBytes = 4 * vb_view.StrideInBytes;
        cmdlist->IASetVertexBuffers(0, 1, &vb_view);

        cmdlist->DrawInstanced(4, 1, 0, 0);


        cmdlist->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(frags.buffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE));
    }

    cmdlist->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
    cmdlist->SetPipelineState(root.pipelines[1]);
    cmdlist->SetGraphicsRootSignature(root.root_signatures[1]);
    cmdlist->SetGraphicsRootDescriptorTable(0, Copy_Descriptors_To_GPU(gfx, 1, frags.buffer_srv));

    frags.counter += 8;
    cmdlist->DrawInstanced(frags.counter, 1, 0, 0);
    if (frags.counter >= FRAGMENTS_RT_RESOLUTION * FRAGMENTS_RT_RESOLUTION)
    {
        frags.counter = 0;
    }

    cmdlist->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(frags.target, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));


    cmdlist->RSSetViewports(1, &D3D12_VIEWPORT{ 0.0f, 0.0f, (f32)gfx.resolution[0], (f32)gfx.resolution[1], 0.0f, 1.0f });
    cmdlist->RSSetScissorRects(1, &D3D12_RECT{ 0, 0, (LONG)gfx.resolution[0], (LONG)gfx.resolution[1] });

    ID3D12Resource *back_buffer;
    D3D12_CPU_DESCRIPTOR_HANDLE back_buffer_rtv;
    Get_Back_Buffer(gfx, back_buffer, back_buffer_rtv);

    cmdlist->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(back_buffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));


    cmdlist->OMSetRenderTargets(1, &back_buffer_rtv, 0, nullptr);
    cmdlist->ClearRenderTargetView(back_buffer_rtv, XMVECTORF32{ 0.0f, 0.2f, 0.4f, 1.0f }, 0, nullptr);


    cmdlist->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdlist->SetPipelineState(root.pipelines[2]);
    cmdlist->SetGraphicsRootSignature(root.root_signatures[2]);
    cmdlist->SetGraphicsRootDescriptorTable(0, Copy_Descriptors_To_GPU(gfx, 1, frags.target_srv));

    cmdlist->DrawInstanced(3, 1, 0, 0);


    cmdlist->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    cmdlist->SetPipelineState(root.pipelines[3]);
    cmdlist->SetGraphicsRootSignature(root.root_signatures[3]);

    D3D12_VERTEX_BUFFER_VIEW vb_view = {};
    vb_view.BufferLocation = root.geometry_buffer->GetGPUVirtualAddress();
    vb_view.StrideInBytes = sizeof(FRAGMENT);
    vb_view.SizeInBytes = 4 * vb_view.StrideInBytes;
    cmdlist->IASetVertexBuffers(0, 1, &vb_view);

    cmdlist->DrawInstanced(4, 1, 0, 0);


    cmdlist->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(back_buffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

    cmdlist->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(frags.target, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET));

    cmdlist->Close();


    gfx.cmdqueue->ExecuteCommandLists(1, (ID3D12CommandList **)&cmdlist);
}

static void
Init_Fragment_Resources(GRAPHICS_CONTEXT &gfx, FRAGMENTS &frags)
{
    frags.counter = 0;

    // buffer that stores window-space position of each fragment
    {
        auto buffer_desc = CD3DX12_RESOURCE_DESC::Buffer((FRAGMENTS_RT_RESOLUTION * FRAGMENTS_RT_RESOLUTION + 1) * sizeof(FRAGMENT));
        buffer_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        VHR(gfx.device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_ALLOW_SHADER_ATOMICS, &buffer_desc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(&frags.buffer)));

        Allocate_Descriptors(gfx, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, frags.buffer_srv);
        Allocate_Descriptors(gfx, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, frags.buffer_uav);

        D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
        uav_desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
        uav_desc.Buffer.FirstElement = 1;
        uav_desc.Buffer.NumElements = FRAGMENTS_RT_RESOLUTION * FRAGMENTS_RT_RESOLUTION;
        uav_desc.Buffer.StructureByteStride = sizeof(FRAGMENT);
        uav_desc.Buffer.CounterOffsetInBytes = 0;
        gfx.device->CreateUnorderedAccessView(frags.buffer, frags.buffer, &uav_desc, frags.buffer_uav);

        D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
        srv_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srv_desc.Buffer.FirstElement = 1;
        srv_desc.Buffer.NumElements = FRAGMENTS_RT_RESOLUTION * FRAGMENTS_RT_RESOLUTION;
        srv_desc.Buffer.StructureByteStride = sizeof(FRAGMENT);
        gfx.device->CreateShaderResourceView(frags.buffer, &srv_desc, frags.buffer_srv);
    }
    // render target for fragments (rtv and srv)
    {
        auto image_desc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, FRAGMENTS_RT_RESOLUTION, FRAGMENTS_RT_RESOLUTION, 1, 1);
        image_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
        VHR(gfx.device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE, &image_desc, D3D12_RESOURCE_STATE_RENDER_TARGET, &CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R8G8B8A8_UNORM, XMVECTORF32{ 0.0f, 0.0f, 0.0f, 0.0f }), IID_PPV_ARGS(&frags.target)));

        Allocate_Descriptors(gfx, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 1, frags.target_rtv);
        Allocate_Descriptors(gfx, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, frags.target_srv);

        gfx.device->CreateRenderTargetView(frags.target, nullptr, frags.target_rtv);
        gfx.device->CreateShaderResourceView(frags.target, nullptr, frags.target_srv);
    }
}

static void
Init_Pipelines(GRAPHICS_CONTEXT &gfx, ID3D12PipelineState **pipelines, ID3D12RootSignature **root_signatures)
{
    const D3D12_INPUT_ELEMENT_DESC input_elements[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    const struct
    {
        const char *vs;
        const char *ps;
        const D3D12_INPUT_ELEMENT_DESC *in_elements;
        u32 in_elements_count;
        D3D12_PRIMITIVE_TOPOLOGY_TYPE topology_type;
        D3D12_FILL_MODE fill_mode;
        bool depth_test;
    } desc[NUM_GRAPHICS_PIPELINES] =
    {
        { "0.vs.cso", "0.ps.cso", input_elements, 2, D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE, D3D12_FILL_MODE_SOLID, true },
        { "1.vs.cso", "1.ps.cso", nullptr, 0, D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT, D3D12_FILL_MODE_SOLID, false },
        { "2.vs.cso", "2.ps.cso", nullptr, 0, D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE, D3D12_FILL_MODE_SOLID, false },
        { "3.vs.cso", "3.ps.cso", input_elements, 2, D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE, D3D12_FILL_MODE_WIREFRAME, false },
    };

    for (u32 pipeline_index = 0; pipeline_index < NUM_GRAPHICS_PIPELINES; ++pipeline_index)
    {
        u32 vs_size, ps_size;
        u8 *vs_data, *ps_data;
        char path[MAX_PATH];

        snprintf(path, sizeof(path), "Data/Shaders/%s", desc[pipeline_index].vs);
        Load_File(path, vs_size, vs_data);

        snprintf(path, sizeof(path), "Data/Shaders/%s", desc[pipeline_index].ps);
        Load_File(path, ps_size, ps_data);

        D3D12_GRAPHICS_PIPELINE_STATE_DESC pso_desc = {};
        if (desc[pipeline_index].in_elements)
        {
            pso_desc.InputLayout = { desc[pipeline_index].in_elements, desc[pipeline_index].in_elements_count };
        }
        pso_desc.VS = { vs_data, vs_size };
        pso_desc.PS = { ps_data, ps_size };
        pso_desc.RasterizerState.FillMode = desc[pipeline_index].fill_mode;
        pso_desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
        pso_desc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        if (desc[pipeline_index].depth_test)
        {
            pso_desc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
            pso_desc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
        }
        pso_desc.SampleMask = 0xffffffff;
        pso_desc.PrimitiveTopologyType = desc[pipeline_index].topology_type;
        pso_desc.NumRenderTargets = 1;
        pso_desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        pso_desc.SampleDesc.Count = 1;

        VHR(gfx.device->CreateGraphicsPipelineState(&pso_desc, IID_PPV_ARGS(&pipelines[pipeline_index])));
        VHR(gfx.device->CreateRootSignature(0, vs_data, vs_size, IID_PPV_ARGS(&root_signatures[pipeline_index])));

        Mem_Free(vs_data);
        Mem_Free(ps_data);
    }
}

static void
Demo_Init(DEMO_ROOT &root)
{
    GRAPHICS_CONTEXT &gfx = root.gfx;

    // geometry buffer
    {
        const auto buffer_desc = CD3DX12_RESOURCE_DESC::Buffer(64 * 1024);
        VHR(gfx.device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE, &buffer_desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&root.geometry_buffer)));

        f32 *ptr;
        VHR(root.geometry_buffer->Map(0, &CD3DX12_RANGE(0, 0), (void **)&ptr));
        /*
        *ptr++ = 0.0f; *ptr++ = 0.5f; *ptr++ = 0.0f; *ptr++ = 1.0f; *ptr++ = 0.0f; *ptr++ = 0.0f;
        *ptr++ = 0.25f; *ptr++ = 0.25f; *ptr++ = 0.0f; *ptr++ = 0.0f; *ptr++ = 1.0f; *ptr++ = 0.0f;
        *ptr++ = -0.25f; *ptr++ = 0.25f; *ptr++ = 0.0f; *ptr++ = 0.0f; *ptr++ = 0.0f; *ptr++ = 1.0f;

        *ptr++ = 0.0f; *ptr++ = 0.2f; *ptr++ = 0.0f; *ptr++ = 1.0f; *ptr++ = 0.0f; *ptr++ = 0.0f;
        *ptr++ = 0.2f; *ptr++ = -0.05f; *ptr++ = 0.0f; *ptr++ = 0.0f; *ptr++ = 1.0f; *ptr++ = 0.0f;
        *ptr++ = -0.2f; *ptr++ = -0.05f; *ptr++ = 0.0f; *ptr++ = 0.0f; *ptr++ = 0.0f; *ptr++ = 1.0f;

        *ptr++ = 0.0f; *ptr++ = -0.1f; *ptr++ = 0.0f; *ptr++ = 1.0f; *ptr++ = 0.0f; *ptr++ = 0.0f;
        *ptr++ = 0.15f; *ptr++ = -0.35f; *ptr++ = 0.0f; *ptr++ = 0.0f; *ptr++ = 1.0f; *ptr++ = 0.0f;
        *ptr++ = -0.15f; *ptr++ = -0.35f; *ptr++ = 0.0f; *ptr++ = 0.0f; *ptr++ = 0.0f; *ptr++ = 1.0f;

        *ptr++ = 0.0f; *ptr++ = -0.4f; *ptr++ = 0.0f; *ptr++ = 1.0f; *ptr++ = 0.0f; *ptr++ = 0.0f;
        *ptr++ = 0.15f; *ptr++ = -0.65f; *ptr++ = 0.0f; *ptr++ = 0.0f; *ptr++ = 1.0f; *ptr++ = 0.0f;
        *ptr++ = -0.15f; *ptr++ = -0.65f; *ptr++ = 0.0f; *ptr++ = 0.0f; *ptr++ = 0.0f; *ptr++ = 1.0f;
        */

        *ptr++ = -0.95f; *ptr++ = -0.95f; *ptr++ = 0.0f; *ptr++ = 1.0f; *ptr++ = 0.0f; *ptr++ = 0.0f;
        *ptr++ = -0.95f; *ptr++ = 0.95f; *ptr++ = 0.0f; *ptr++ = 0.0f; *ptr++ = 1.0f; *ptr++ = 0.0f;
        *ptr++ = 0.95f; *ptr++ = -0.95f; *ptr++ = 0.0f; *ptr++ = 0.0f; *ptr++ = 0.0f; *ptr++ = 1.0f;
        *ptr++ = 0.95f; *ptr++ = 0.95f; *ptr++ = 0.0f; *ptr++ = 1.0f; *ptr++ = 1.0f; *ptr++ = 0.0f;

        /*
        *ptr++ = -0.9f; *ptr++ = 0.9f; *ptr++ = 0.0f; *ptr++ = 1.0f; *ptr++ = 0.0f; *ptr++ = 0.0f;
        *ptr++ = -0.8f; *ptr++ = 0.9f; *ptr++ = 0.0f; *ptr++ = 0.0f; *ptr++ = 1.0f; *ptr++ = 0.0f;
        *ptr++ = -0.9f; *ptr++ = -0.9f; *ptr++ = 0.0f; *ptr++ = 0.0f; *ptr++ = 0.0f; *ptr++ = 1.0f;
        */
    }

    Init_Pipelines(gfx, root.pipelines, root.root_signatures);
    Init_Fragment_Resources(gfx, root.frags);
}

static i32
Demo_Run(DEMO_ROOT &root)
{
    HWND window = Create_Window(DEMO_NAME, 1024, 1024);

    Init_Graphics_Context(window, root.gfx);
    Demo_Init(root);

    for (;;)
    {
        MSG message = {};
        if (PeekMessage(&message, 0, 0, 0, PM_REMOVE))
        {
            DispatchMessage(&message);
            if (message.message == WM_QUIT)
            {
                break;
            }
        }
        else
        {
            f64 time;
            f32 delta_time;
            Update_Frame_Stats(window, DEMO_NAME, time, delta_time);
            Demo_Update(root);
            Present_Frame(root.gfx, 1);
        }
    }
    return 0;
}

i32 CALLBACK
WinMain(HINSTANCE, HINSTANCE, LPSTR, i32)
{
    SetProcessDPIAware();
    DEMO_ROOT root = {};
    return Demo_Run(root);
}
