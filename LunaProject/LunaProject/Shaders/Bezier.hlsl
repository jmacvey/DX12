cbuffer cbSettings : register(b0)
{
    float4x4 gWorld;
};

cbuffer passSettings : register(b1)
{
    float4x4 gView;
    float4x4 gProj;
    float4x4 gViewProj;
};

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
    float4 PosH : SV_POSITION;
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

    float4 posW = mul(float4(p, 1.0f), gWorld);
    dOut.PosH = mul(posW, gViewProj);

    return dOut;
}

float4 PS(DomainOut ds) : SV_TARGET
{
    return float4(1.0f, 1.0f, 1.0f, 1.0f);
}
