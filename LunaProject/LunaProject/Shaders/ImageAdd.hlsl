Texture2D gMap0 : register(t0);
Texture2D gMap1 : register(t1);

SamplerState gsamPointWrap : register(s0);
SamplerState gsamPointClamp : register(s1);
SamplerState gsamLinearWrap : register(s2);
SamplerState gsamLinearClamp : register(s3);
SamplerState gsamAnisotropicWrap : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);

static const float2 gTexCoords[6] =
{
    float2(0.0f, 1.0f), // bottom left
    float2(0.0f, 0.0f), // top left
    float2(1.0f, 0.0f), // top right
    float2(0.0f, 1.0f), // bottom left
    float2(1.0f, 0.0f), // top right
    float2(1.0f, 1.0f) // bottom right
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float2 TexC : TEXCOORD;
};

VertexOut VS(uint vid : SV_VertexID)
{
    VertexOut vout;
    vout.TexC = gTexCoords[vid];

    // x: 0 -> -1, 1 -> 1,
    // y: 0 -> 1, 1 -> -1


    // Map [0, 1]^2 to NDC space [-1, 1]^2
    vout.PosH = float4(2.0f*vout.TexC.x - 1.0f, 1.0f - 2.0f*vout.TexC.y, 0.0f, 1.0f);
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    float4 c = gMap0.Sample(gsamPointClamp, pin.TexC, 0.0f);
    float4 e = gMap1.Sample(gsamPointClamp, pin.TexC, 0.0f);
    return c * e;
}