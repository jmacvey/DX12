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
};

VertexOut VS(VertexIn vin, uint instanceId : SV_InstanceId)
{
    VertexOut vout;
    vout.PosL = vin.PosL;
    InstanceData instance = gInstanceData[instanceId];

    float4 posW = mul(float4(vin.PosL, 1.0f), instance.World);

    #ifndef NORMAL_RENDER
        posW.xyz += gEyePos.xyz;
        vout.PosH = mul(posW, gViewProj).xyww;
    #endif

    #ifdef NORMAL_RENDER
        vout.PosH = mul(posW, gViewProj);
    #endif

    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    return gCubeMap.Sample(gsamLinearWrap, pin.PosL);
}