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
    float EdgeTess[3] : SV_TessFactor;
    float InsideTess : SV_InsideTessFactor;
};


TessFactors ConstantHS(InputPatch<VertexOut, 3> patch,
    uint patchID : SV_PrimitiveID)
{
    float2 depthRange = float2(20.0f, 100.0f);
    float3 centerW = 0.33333333f * (patch[0].PosL + patch[1].PosL + patch[2].PosL);
    
    float d = distance(gEyePos, centerW);

    float tessFactor = 10.0f * saturate((depthRange.y - d) / (depthRange.y - depthRange.x));

    TessFactors ts;
    [unroll]
    for (int i = 0; i < 3; ++i)
    {
        ts.EdgeTess[i] = tessFactor;
    }
    ts.InsideTess = tessFactor;
    
    return ts;
}

struct HullOut
{
    float3 PosL : POSITION;
};

[domain("tri")]
[partitioning("integer")]
[outputtopology("triangle_ccw")]
[outputcontrolpoints(3)]
[patchconstantfunc("ConstantHS")]
[maxtessfactor(10.0f)]
HullOut HS(InputPatch<VertexOut, 3> p,
    uint i : SV_OutputControlPointID,
    uint patchID : SV_PrimitiveID)
{
    HullOut hs;
    hs.PosL = p[i].PosL;
    return hs;
}

struct DomainOut
{
    float4 PosH : SV_POSITION;
};

[domain("tri")]
DomainOut DS(TessFactors ts,
    float3 rst : SV_DomainLocation,
    OutputPatch<HullOut, 3> tri)
{
    DomainOut dOut;

    // use barycentric coordinates to calculate position of vertex
    float3 p = rst.x*tri[0].PosL + rst.y*tri[1].PosL + rst.z*tri[2].PosL;

    #ifdef LAND_GEOMETRY
        p.y = 0.3f * (p.z * sin(p.x) + p.x * cos(p.z));
    #endif

    #ifdef SPHERE_GEOMETRY
        float radius = distance(float3(0.0f, 0.0f, 0.0f), tri[0].PosL);
        p /= length(p);
        p *= radius;
    #endif
    
    float4 posW = mul(float4(p, 1.0f), gWorld);
    
    dOut.PosH = mul(posW, gViewProj);

    return dOut;
};

float4 PS(DomainOut pin) : SV_Target
{
    return gDiffuseAlbedo;
};