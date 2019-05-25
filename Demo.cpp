﻿#include "External.h"
#include "Library.h"

#define K_DEMO_NAME "Rasterization"
#define K_NUM_GRAPHICS_PIPELINES 2

struct DEMO_ROOT
{
    GRAPHICS_CONTEXT gfx;
    ID3D12Resource* fragments_buffer;
    D3D12_CPU_DESCRIPTOR_HANDLE fragments_uav;
    D3D12_CPU_DESCRIPTOR_HANDLE fragments_srv;
    ID3D12PipelineState* pipelines[K_NUM_GRAPHICS_PIPELINES];
    ID3D12RootSignature* root_signatures[K_NUM_GRAPHICS_PIPELINES];
    u32 fragment_counter;
};

static void Demo_Update(DEMO_ROOT& root)
{
    GRAPHICS_CONTEXT& gfx = root.gfx;

    ID3D12GraphicsCommandList2* cmdlist = Get_And_Reset_Command_List(gfx);

    Bind_Descriptor_Heap(gfx);

    cmdlist->RSSetViewports(1, &D3D12_VIEWPORT{ 0.0f, 0.0f, (f32)gfx.resolution[0], (f32)gfx.resolution[1], 0.0f, 1.0f });
    cmdlist->RSSetScissorRects(1, &D3D12_RECT{ 0, 0, (LONG)gfx.resolution[0], (LONG)gfx.resolution[1] });

    ID3D12Resource* back_buffer;
    D3D12_CPU_DESCRIPTOR_HANDLE back_buffer_descriptor;
    Get_Back_Buffer(gfx, back_buffer, back_buffer_descriptor);

    cmdlist->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(back_buffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));


    cmdlist->OMSetRenderTargets(1, &back_buffer_descriptor, 0, nullptr);
    cmdlist->ClearRenderTargetView(back_buffer_descriptor, XMVECTORF32{ 0.0f, 0.2f, 0.4f, 1.0f }, 0, nullptr);


    if (root.fragment_counter < 128)
    {
        // clear atomic counter in the fragments buffer
        cmdlist->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(root.fragments_buffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_DEST));

        D3D12_WRITEBUFFERIMMEDIATE_PARAMETER param = { root.fragments_buffer->GetGPUVirtualAddress(), 0 };
        cmdlist->WriteBufferImmediate(1, &param, nullptr);

        cmdlist->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(root.fragments_buffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));


        cmdlist->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
        cmdlist->SetPipelineState(root.pipelines[0]);
        cmdlist->SetGraphicsRootSignature(root.root_signatures[0]);

        // bind fragments buffer uav
        cmdlist->SetGraphicsRootDescriptorTable(0, Copy_Descriptors_To_GPU(gfx, 1, root.fragments_uav));

        cmdlist->DrawInstanced(4, 1, 0, 0);
    }


    cmdlist->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(root.fragments_buffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE));

    cmdlist->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
    cmdlist->SetPipelineState(root.pipelines[1]);
    cmdlist->SetGraphicsRootSignature(root.root_signatures[1]);

    // bind fragments buffer srv
    cmdlist->SetGraphicsRootDescriptorTable(0, Copy_Descriptors_To_GPU(gfx, 1, root.fragments_srv));

    cmdlist->DrawInstanced(root.fragment_counter += 32, 1, 0, 0);
    if (root.fragment_counter >= gfx.resolution[0] * gfx.resolution[1])
    {
        root.fragment_counter = 0;
    }

    cmdlist->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(root.fragments_buffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

    cmdlist->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(back_buffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

    cmdlist->Close();

    gfx.cmdqueue->ExecuteCommandLists(1, (ID3D12CommandList**)&cmdlist);
}

static void Demo_Init(DEMO_ROOT& root)
{
    GRAPHICS_CONTEXT& gfx = root.gfx;

    /* VS_0, PS_0 */
    {
        std::vector<u8> vs_code = Load_File("Data/Shaders/0.vs.cso");
        std::vector<u8> ps_code = Load_File("Data/Shaders/0.ps.cso");

        D3D12_GRAPHICS_PIPELINE_STATE_DESC pso_desc = {};
        pso_desc.VS = { vs_code.data(), vs_code.size() };
        pso_desc.PS = { ps_code.data(), ps_code.size() };
        pso_desc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
        pso_desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
        pso_desc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        pso_desc.SampleMask = 0xffffffff;
        pso_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        pso_desc.NumRenderTargets = 1;
        pso_desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        pso_desc.SampleDesc.Count = 1;

        VHR(gfx.device->CreateGraphicsPipelineState(&pso_desc, IID_PPV_ARGS(&root.pipelines[0])));
        VHR(gfx.device->CreateRootSignature(0, vs_code.data(), vs_code.size(), IID_PPV_ARGS(&root.root_signatures[0])));
    }
    /* VS_1, PS_1 */
    {
        std::vector<u8> vs_code = Load_File("Data/Shaders/1.vs.cso");
        std::vector<u8> ps_code = Load_File("Data/Shaders/1.ps.cso");

        D3D12_GRAPHICS_PIPELINE_STATE_DESC pso_desc = {};
        pso_desc.VS = { vs_code.data(), vs_code.size() };
        pso_desc.PS = { ps_code.data(), ps_code.size() };
        pso_desc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
        pso_desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
        pso_desc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        pso_desc.SampleMask = 0xffffffff;
        pso_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
        pso_desc.NumRenderTargets = 1;
        pso_desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        pso_desc.SampleDesc.Count = 1;

        VHR(gfx.device->CreateGraphicsPipelineState(&pso_desc, IID_PPV_ARGS(&root.pipelines[1])));
        VHR(gfx.device->CreateRootSignature(0, vs_code.data(), vs_code.size(), IID_PPV_ARGS(&root.root_signatures[1])));
    }
    /* buffer that stores window-space position of each fragment */
    {
        auto buffer_desc = CD3DX12_RESOURCE_DESC::Buffer((gfx.resolution[0] * gfx.resolution[1] + 1) * sizeof(XMFLOAT2));
        buffer_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        VHR(gfx.device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_ALLOW_SHADER_ATOMICS, &buffer_desc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(&root.fragments_buffer)));

        Allocate_Descriptors(gfx, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, root.fragments_srv);
        Allocate_Descriptors(gfx, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, root.fragments_uav);

        D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
        uav_desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
        uav_desc.Buffer.FirstElement = 1;
        uav_desc.Buffer.NumElements = gfx.resolution[0] * gfx.resolution[1];
        uav_desc.Buffer.StructureByteStride = sizeof(XMFLOAT2);
        uav_desc.Buffer.CounterOffsetInBytes = 0;
        gfx.device->CreateUnorderedAccessView(root.fragments_buffer, root.fragments_buffer, &uav_desc, root.fragments_uav);

        D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
        srv_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srv_desc.Buffer.FirstElement = 1;
        srv_desc.Buffer.NumElements = gfx.resolution[0] * gfx.resolution[1];
        srv_desc.Buffer.StructureByteStride = sizeof(XMFLOAT2);
        gfx.device->CreateShaderResourceView(root.fragments_buffer, &srv_desc, root.fragments_srv);
    }
}

static i32 Demo_Run()
{
    HWND window = Create_Window(K_DEMO_NAME, 800, 800);

    DEMO_ROOT root = {};
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
            Update_Frame_Stats(window, K_DEMO_NAME, time, delta_time);
            Demo_Update(root);
            Present_Frame(root.gfx, 1);
        }
    }
    return 0;
}

i32 CALLBACK WinMain(HINSTANCE, HINSTANCE, LPSTR, i32)
{
    SetProcessDPIAware();
    return Demo_Run();
}