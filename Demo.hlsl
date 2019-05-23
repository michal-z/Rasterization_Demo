//-------------------------------------------------------------------------------------------------
#if PS_0 || VS_1

struct FRAGMENT
{
    float2 position;
};

#endif // #if PS_0 || VS_1
//-------------------------------------------------------------------------------------------------
#if VS_0 || PS_0

#define K_RSI_0 "RootFlags(0), " \
    "DescriptorTable(UAV(u0), visibility = SHADER_VISIBILITY_PIXEL)"

struct VERTEX_OUTPUT
{
    float4 position : SV_Position;
    float2 normalized_coords : NCOORDS;
};

#endif // #if VS_0 || PS_0
//-------------------------------------------------------------------------------------------------
#if VS_0

[RootSignature(K_RSI_0)]
VERTEX_OUTPUT VS0_Main(uint vertex_id : SV_VertexID)
{
    float2 positions[4] = { float2(-1.0f, -1.0f), float2(-1.0f, 1.0f), float2(1.0f, -1.0f), float2(1.0f, 1.0f) };
    VERTEX_OUTPUT output;
    output.normalized_coords = 0.7f * positions[vertex_id];
    output.position = float4(output.normalized_coords, 0.0f, 1.0f);
    return output;
}

#endif // #if VS_0
//-------------------------------------------------------------------------------------------------
#if PS_0

RWStructuredBuffer<FRAGMENT> uav_fragments : register(u0);

[RootSignature(K_RSI_0)]
float4 PS0_Main(VERTEX_OUTPUT input) : SV_Target0
{
    uint index = uav_fragments.IncrementCounter();
    uav_fragments[index].position = input.normalized_coords;
    return float4(0.0f, 0.5f, 0.0f, 1.0f);
}

#endif // #if PS_0
//-------------------------------------------------------------------------------------------------
#if VS_1 || PS_1

#define K_RSI_1 "RootFlags(0), " \
    "DescriptorTable(SRV(t0), visibility = SHADER_VISIBILITY_VERTEX)"

#endif // #if VS_1 || PS_1
//-------------------------------------------------------------------------------------------------
#if VS_1

StructuredBuffer<FRAGMENT> srv_fragments : register(t0);

[RootSignature(K_RSI_1)]
float4 VS1_Main(uint vertex_id : SV_VertexID) : SV_Position
{
    FRAGMENT frag = srv_fragments[vertex_id];
    return float4(frag.position, 0.0f, 1.0f);
}

#endif // #if VS_1
//-------------------------------------------------------------------------------------------------
#if PS_1

[RootSignature(K_RSI_1)]
float4 PS1_Main(float4 position : SV_Position) : SV_Target0
{
    return float4(0.6f, 0.5f, 0.0, 1.0);
}

#endif // #if PS_1
//-------------------------------------------------------------------------------------------------
