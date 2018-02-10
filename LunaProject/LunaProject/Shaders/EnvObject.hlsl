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
    float3 PosL : POSITION_L;
    float3 NormalW : NORMAL;
    float3 PosW : POSITION_W;
    float2 TexC : TEXCOORD;
};

VertexOut VS(VertexIn vin, uint instanceId : SV_InstanceId)
{
    VertexOut vout;
    vout.PosL = vin.PosL;
    InstanceData instance = gInstanceData[instanceId];
    float4x4 texTransform = instance.TexTransform;
    float4 posW = mul(float4(vin.PosL, 1.0f), instance.World);

    vout.NormalW = mul(vin.NormalL, (float3x3) instance.World);
    vout.PosW = posW.xyz;
    vout.PosH = mul(posW, gViewProj);
    vout.TexC = mul(float4(vin.TexC, 0.0f, 1.0f), texTransform).xy;
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    pin.NormalW = normalize(pin.NormalW);
    float3 toEyeW = normalize(gEyePos - pin.PosW);

    // ambient
    float4 diffuseAlbedo = gDiffuseMap.Sample(gsamLinearWrap, pin.TexC);
    float4 ambient = diffuseAlbedo;
    const float shininess = 0.2f;

    Material mat;
    mat.FresnelR0 = float3(0.8f, 0.8f, 0.8f);
    mat.DiffuseAlbedo = ambient;
    mat.Shininess = shininess;

    float4 directLight = ComputeLighting(gLights, mat, pin.PosW, pin.NormalW, toEyeW, 1.0f);

    float4 litColor = ambient + directLight;

    // add specular reflections
    float3 r = reflect(-toEyeW, pin.NormalW);
    float4 reflectionColor = gCubeMap.Sample(gsamLinearWrap, r);
    float3 fresnelFactor = SchlickFresnel(mat.FresnelR0, pin.NormalW, r);
    litColor.rgb += shininess * fresnelFactor * reflectionColor.rgb;

    litColor.a = diffuseAlbedo.a;

    return litColor;
}