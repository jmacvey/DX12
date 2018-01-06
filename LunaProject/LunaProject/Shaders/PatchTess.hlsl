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
};

struct PatchTess
{
    float EdgeTess[4] : SV_TessFactor;
    float InsideTess[2] : SV_InsideTessFactor;
};

// hull shader defines the tesselation factors

PatchTess ConstantHS(InputPatch<VertexOut, 4> patch,
    uint patchID : SV_PrimitiveID)
{
    PatchTess pt;

    float3 centerL = 0.25f * (
        patch[0].PosL +
        patch[1].PosL +
        patch[2].PosL +
        patch[3].PosL);

    float4 centerW = mul(float4(centerL, 1.0f), gWorld);

    float d = distance(centerW.xyz, gEyePos);
    float d0 = 20.0f;
    float d1 = 100.0f;

    float tess = 64.0f * saturate((d1 - d) / (d1 - d0));

    [unroll]
    for (uint i = 0; i < 4; ++i)
    {
        pt.EdgeTess[i] = tess;
    }
    pt.InsideTess[0] = tess;
    pt.InsideTess[1] = tess;

    return pt;
}

struct HullOut
{
    float3 PosL : POSITION;
};

[domain("quad")]
[partitioning("integer")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(4)]
[patchconstantfunc("ConstantHS")]
[maxtessfactor(64.0f)]
HullOut HS(InputPatch<VertexOut, 4> p,
    uint i : SV_OutputControlPointID,
    uint patchID : SV_PrimitiveID)
{
    HullOut hout;
    hout.PosL = p[i].PosL;
    return hout;
}

struct DomainOut
{
    float4 PosH : SV_POSITION;
};

[domain("quad")]
DomainOut DS(PatchTess patchTess,
float2 uv : SV_DomainLocation,
const OutputPatch<HullOut, 4> quad)
{
    DomainOut dout;

    // bilinear interpolation
    float3 v1 = lerp(quad[0].PosL, quad[1].PosL, uv.x);
    float3 v2 = lerp(quad[2].PosL, quad[3].PosL, uv.x);
    float3 p = lerp(v1, v2, uv.y);

    // displacement mapping
    p.y = 0.3f * (p.z * sin(p.x) + p.x * cos(p.z));

    // output
    float4 posW = mul(float4(p, 1.0f), gWorld);
    dout.PosH = mul(posW, gViewProj);
    
    return dout;

}

float4 PS(DomainOut pin) : SV_Target
{
    return float4(1.0f, 1.0f, 1.0f, 1.0f);
}