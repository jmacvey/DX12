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
};

cbuffer cbMaterial : register(b2)
{
    float4 gDiffuseAlbedo;
    float3 gFresnelR0;
    float gRoughness;
    float4x4 gMatTransform;
};

Texture2D gDiffuseMap : register(t0);

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

struct TessFactors
{
    float EdgeTess[4] : SV_TessFactor;
    float InsideTess[2] : SV_InsideTessFactor;
};

TessFactors ConstantHS(InputPatch<VertexOut, 9> patchIn)
{
    TessFactors ts;

    const float tessFactor = 10.0f;
    
    [unroll]
    for (int i = 0; i < 4; ++i)
    {
        ts.EdgeTess[i] = tessFactor;
    }

    ts.InsideTess[0] = tessFactor;
    ts.InsideTess[1] = tessFactor;

    return ts;
}

struct HullOut
{
    float3 PosL : POSITION;
};

[domain("quad")]
[partitioning("integer")]
[outputtopology("triangle_ccw")]
[outputcontrolpoints(9)]
[patchconstantfunc("ConstantHS")]
[maxtessfactor(25.0f)]
HullOut HS(InputPatch<VertexOut, 9> patchIn, int i: SV_OutputControlPointID)
{
    HullOut hOut;
    hOut.PosL = patchIn[i].PosL;
    return hOut;
}

struct DomainOut
{
    float4 posH : SV_POSITION;
};

float3 CalculateBasis(float t)
{
    float invT = 1.0f - t;
    float3 basisVector;
    basisVector.x = invT * invT;
    basisVector.y = 2.0f * t * invT;
    basisVector.z = t * t;
    return basisVector;
}

float3 CalculateBezierSum(const OutputPatch<HullOut, 9> hullOut, float3 basisU, float3 basisV)
{
    float3 sum = float3(0.0f, 0.0f, 0.0f);
    sum += basisV.x * (basisU.x*hullOut[0].PosL + basisU.y*hullOut[1].PosL + basisU.z*hullOut[2].PosL);
    sum += basisV.y * (basisU.x*hullOut[3].PosL + basisU.y*hullOut[4].PosL + basisU.z*hullOut[5].PosL);
    sum += basisV.z * (basisU.x*hullOut[6].PosL + basisU.y*hullOut[7].PosL + basisU.z*hullOut[8].PosL);

    return sum;
}

[domain("quad")]
DomainOut DS(TessFactors ts, float2 uv : SV_DomainLocation, const
OutputPatch<HullOut, 9> hOut)
{
    float3 basisU = CalculateBasis(uv.x);
    float3 basisV = CalculateBasis(uv.y);

    float3 sum = CalculateBezierSum(hOut, basisU, basisV);

    float4 posW = mul(float4(sum, 1.0f), gWorld);
    float4 posH = mul(posW, gViewProj);

    DomainOut dOut;
    dOut.posH = posH;
    return dOut;
}

float4 PS(DomainOut pin) : SV_Target
{
    return gDiffuseAlbedo;
}