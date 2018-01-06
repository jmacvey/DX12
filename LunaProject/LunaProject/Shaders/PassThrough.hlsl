
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
    float4 PosH : SV_POSITION;
    float3 PosW : POSITION;
};

VertexOut VS(VertexIn vin)
{
    VertexOut vout;
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    vout.PosW = posW.xyz;
    vout.PosH = mul(posW, gViewProj);

    return vout;
}

float4 PS(VertexOut pin) : SV_TARGET
{
    return float4(0.0f, 0.0f, 0.0f, 0.0f);
}