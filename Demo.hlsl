//-------------------------------------------------------------------------------------------------
#if PS_0 || VS_1

#include "Common.h"

#endif // #if PS_0 || VS_1
//=================================================================================================
#if VS_0 || PS_0

#define RSI_0 "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT), " \
    "DescriptorTable(UAV(u0), visibility = SHADER_VISIBILITY_PIXEL)"

struct VERTEX_OUTPUT
{
    float4 position : SV_Position;
    float2 normalized_coords : NCOORDS;
    float3 color : COLOR;
};

#endif // #if VS_0 || PS_0
//-------------------------------------------------------------------------------------------------
#if VS_0

struct VERTEX_INPUT
{
    float2 position : POSITION;
    float3 color : COLOR;
};

[RootSignature(RSI_0)]
VERTEX_OUTPUT
VS0_Main(VERTEX_INPUT input)
{
    VERTEX_OUTPUT output;
    output.normalized_coords = input.position;
    output.color = input.color;
    output.position = float4(output.normalized_coords, 0.0f, 1.0f);
    return output;
}

#endif // #if VS_0
//-------------------------------------------------------------------------------------------------
#if PS_0

RWStructuredBuffer<FRAGMENT> uav_fragments : register(u0);

[RootSignature(RSI_0)]
float4
PS0_Main(VERTEX_OUTPUT input) : SV_Target0
{
    uint index = uav_fragments.IncrementCounter();
    uav_fragments[index].position = input.normalized_coords;
    uav_fragments[index].color = input.color;
    return float4(0.0f, 0.5f, 0.0f, 1.0f);
}

#endif // #if PS_0
//=================================================================================================
#if VS_1 || PS_1

#define RSI_1 "RootFlags(0), " \
    "DescriptorTable(SRV(t0), visibility = SHADER_VISIBILITY_VERTEX)"

struct VERTEX_OUTPUT
{
    float4 position : SV_Position;
    float3 color : COLOR;
};

#endif // #if VS_1 || PS_1
//-------------------------------------------------------------------------------------------------
#if VS_1

StructuredBuffer<FRAGMENT> srv_fragments : register(t0);

[RootSignature(RSI_1)]
VERTEX_OUTPUT
VS1_Main(uint vertex_id : SV_VertexID)
{
    FRAGMENT frag = srv_fragments[vertex_id];
    VERTEX_OUTPUT output;
    output.position = float4(frag.position, 0.0f, 1.0f);
    output.color = frag.color;
    return output;
}

#endif // #if VS_1
//-------------------------------------------------------------------------------------------------
#if PS_1

[RootSignature(RSI_1)]
float4
PS1_Main(VERTEX_OUTPUT input) : SV_Target0
{
    return float4(input.color, 1.0);
}

#endif // #if PS_1
//=================================================================================================
#if VS_2 || PS_2

#define RSI_2 "RootFlags(0), " \
    "DescriptorTable(SRV(t0), visibility = SHADER_VISIBILITY_PIXEL), " \
    "StaticSampler(s0, filter = FILTER_MIN_MAG_MIP_POINT, visibility = SHADER_VISIBILITY_PIXEL)"

struct VERTEX_OUTPUT
{
    float4 position : SV_Position;
    float2 texcoord : TEXCOORD;
};

#endif
//-------------------------------------------------------------------------------------------------
#if VS_2

[RootSignature(RSI_2)]
VERTEX_OUTPUT
VS2_Main(uint vertex_id : SV_VertexID)
{
    float2 positions[3] = { float2(-1.0f, -1.0f), float2(-1.0f, 3.0f), float2(3.0f, -1.0f) };
    float2 texcoords[3] = { float2(0.0f, 2.0f), float2(0.0f, 0.0f), float2(2.0f, 2.0f) };
    VERTEX_OUTPUT output;
    output.position = float4(positions[vertex_id], 0.0f, 1.0f);
    output.texcoord = texcoords[vertex_id];
    return output;
}

#endif
//-------------------------------------------------------------------------------------------------
#if PS_2

Texture2D srv_t0 : register(t0);
SamplerState sam_s0 : register(s0);

[RootSignature(RSI_2)]
float4
PS2_Main(VERTEX_OUTPUT input) : SV_Target0
{
    return srv_t0.Sample(sam_s0, input.texcoord);
}

#endif
//-------------------------------------------------------------------------------------------------
