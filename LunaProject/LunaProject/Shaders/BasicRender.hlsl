struct InstanceData
{
    float4x4 World;
    float4x4 TexTransform;
    uint matIndex;
    uint instancePad0;
    uint instancePad1;
    uint instancePad2;
};

StructuredBuffer<InstanceData> gInstanceData : register(t0);

cbuffer passSettings : register(b0)
{
    float4x4 gView;
    float4x4 gInvView;
    float4x4 gProj;
    float4x4 gInvProj;
    float4x4 gViewProj;
    float4x4 gInvViewProj;

    float3 gEyePos;
    float cbObjectPad0;
    float2 gRenderTargetSize;
    float2 gInvRenderTargetSize;

    float gNearZ;
    float gFarZ;
    float gTotalTime;
    float gDeltaTIme;

    float4 gAmbientLight;
}

struct VertexIn
{
    float3 PosL : POSITION;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float3 PosW : POSITION;
};

VertexOut VS(VertexIn vin, uint instanceId : SV_InstanceID)
{
    VertexOut vout;
    InstanceData instanceData = gInstanceData[instanceId];
    float4x4 world = instanceData.World;
    float4 posW = mul(float4(vin.PosL, 1.0f), world);
    vout.PosW = posW.xyz;
    vout.PosH = mul(posW, gViewProj);
    return vout;
}

float4 PS(VertexOut vin) : SV_Target
{
    return float4(0.1f, 0.7f, 0.3f, 1.0f);
}