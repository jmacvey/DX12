#include "Common.hlsl"

struct VertexIn
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
    float2 TexC : TEXCOORD;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float3 PosL : POSITION;
    float2 TexC : TEXCOORD;
};

VertexOut VS(VertexIn vin, uint instanceId : SV_InstanceId)
{
    VertexOut vout;
    vout.PosL = vin.PosL;
    InstanceData instance = gInstanceData[instanceId];
    float4x4 texTransform = instance.TexTransform;
    float4 posW = mul(float4(vin.PosL, 1.0f), instance.World);

    vout.PosH = mul(posW, gViewProj);
    vout.TexC = mul(float4(vin.TexC, 0.0f, 1.0f), texTransform).xy;
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    return gDiffuseMap.Sample(gsamLinearWrap, pin.TexC);
}