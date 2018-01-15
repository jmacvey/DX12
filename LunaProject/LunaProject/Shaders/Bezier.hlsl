#ifndef NUM_DIR_LIGHTS
#define NUM_DIR_LIGHTS 1
#endif

#ifndef NUM_POINT_LIGHTS
#define NUM_POINT_LIGHTS 0
#endif

#ifndef NUM_SPOT_LIGHTS
#define NUM_SPOT_LIGHTS 0
#endif

#include "LightingUtil.hlsl"

cbuffer cbSettings : register(b0)
{
    float4x4 gWorld;
};

cbuffer passSettings : register(b1)
{
    float4x4 gView;
    float4x4 gProj;
    float4x4 gViewProj;
    float3 gEyePos;
    float cbPerPassPad0;
    Light gLights[MaxLights];
};


cbuffer cbMaterial : register(b2)
{
    float4 gDiffuseAlbedo;
    float3 gFresnelR0;
    float gRoughness;
    float4x4 gMatTransform;
};

Texture2D gDiffuseMap : register(t0);
SamplerState gsamAnisotropicWrap : register(s0);

struct VertexIn
{
    float3 PosL : POSITION;
};

struct VertexOut
{
    float3 PosL : POSITION;
};

VertexOut VS(VertexIn vin)
{
    VertexOut vout;
    vout.PosL = vin.PosL;
    return vout;
}

struct PatchTess
{
    float EdgeTess[4] : SV_TessFactor;
    float InsideTess[2] : SV_InsideTessFactor;
};

PatchTess ConstantHS(InputPatch<VertexOut, 16> patch,
    uint patchID : SV_PrimitiveID)
{
    PatchTess pt;

    [unroll]
    for (uint i = 0; i < 4; ++i)
    {
        pt.EdgeTess[i] = 25.0f;
    }

    pt.InsideTess[0] = 25.0f;
    pt.InsideTess[1] = 25.0f;

    return pt;
}

struct HullOut
{
    float3 PosL : POSITION;
};

[domain("quad")]
[partitioning("integer")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(16)]
[patchconstantfunc("ConstantHS")]
[maxtessfactor(64.0f)]
HullOut HS(InputPatch<VertexOut, 16> p,
    uint i : SV_OutputControlPointID,
    uint patchID : SV_PrimitiveID)
{
    HullOut hs;
    hs.PosL = p[i].PosL;
    return hs;
}

float4 CalcBasisVector(float t)
{
    float invT = 1.0f - t;
    float4 basis;
    basis.x = invT * invT * invT;
    basis.y = 3.0f * t * invT * invT;
    basis.z = 3.0f * t * t * invT;
    basis.w = t * t * t;
    return basis;
}

float4 dCalcBasisVector(float t)
{
    float invT = 1.0f - t;
    float4 basis;
    basis.x = -3.0f * invT * invT;
    basis.y = 3.0f * invT * invT - 6.0f * t * invT;
    basis.z = 6.0f * t * invT - 3.0f * t * t;
    basis.w = 3.0f * t * t;
    return basis;
}

float3 CalcCubicBezierSum(const OutputPatch<HullOut, 16> bezPatch, float4 basisU, float4 basisV)
{
    float3 sum = float3(0.0f, 0.0f, 0.0f);
    sum += basisV.x*(basisU.x*bezPatch[0].PosL + basisU.y*bezPatch[1].PosL + basisU.z*bezPatch[2].PosL + basisU.w*bezPatch[3].PosL);
    sum += basisV.y*(basisU.x*bezPatch[4].PosL + basisU.y*bezPatch[5].PosL + basisU.z*bezPatch[6].PosL + basisU.w*bezPatch[7].PosL);
    sum += basisV.z*(basisU.x*bezPatch[8].PosL + basisU.y*bezPatch[9].PosL + basisU.z*bezPatch[10].PosL + basisU.w*bezPatch[11].PosL);
    sum += basisV.w*(basisU.x*bezPatch[12].PosL + basisU.y*bezPatch[13].PosL + basisU.z*bezPatch[14].PosL + basisU.w*bezPatch[15].PosL);

    return sum;
}


struct DomainOut
{
    float4 posH : SV_POSITION;
    float3 posW : POSITION;
    float3 normalW : NORMAL;
    float2 texC : TEXCOORD;
};

[domain("quad")]
DomainOut DS(PatchTess pt,
float2 uv : SV_DomainLocation,
const OutputPatch<HullOut, 16> bezPatch)
{

    DomainOut dOut;
    float4 basisU = CalcBasisVector(uv.x);
    float4 basisV = CalcBasisVector(uv.y);

    float3 p = CalcCubicBezierSum(bezPatch, basisU, basisV);

    float4 dBasisU = dCalcBasisVector(uv.x);
    float4 dBasisV = dCalcBasisVector(uv.y);
    
    float3 dudp = CalcCubicBezierSum(bezPatch, dBasisU, basisV);
    float3 dvdp = CalcCubicBezierSum(bezPatch, basisU, dBasisV);
    float3 normal = cross(dudp, dvdp);

    float4 posW = mul(float4(p, 1.0f), gWorld);
    dOut.posH = mul(posW, gViewProj);
    dOut.posW = posW.xyz;
    dOut.normalW = normalize(mul(normal, (float3x3) gWorld));
    dOut.texC = uv;

    return dOut;
}

float4 PS(DomainOut pin) : SV_Target
{
    //return gDiffuseAlbedo;
    float4 diffuseAlbedo = gDiffuseMap.Sample(gsamAnisotropicWrap, pin.texC);
    diffuseAlbedo *= gDiffuseAlbedo;
    float3 toEyeW = normalize(gEyePos - pin.posW);
    const float shininess = 1.0f - gRoughness;
    Material mat = { diffuseAlbedo, gFresnelR0, shininess };
    float3 shadowFactor = 1.0f;
    float4 directLight = ComputeLighting(gLights, mat, pin.posW,
        pin.normalW, toEyeW, shadowFactor);

    directLight.a = diffuseAlbedo.a;
    return directLight;
}