
#include "buffers.hlsl"
#include "LocalToWorldModels.hlsl"

static const float normalLength = 2.0f;

struct GSOutput {
	float4 PosH : SV_POSITION;
	float3 PosW : POSITION;
	float3 NormalW : NORMAL;
};

void ConvertVToG(SVertexOut input, out GSOutput output) {
	output.PosW = mul(float4(input.PosL, 1.0f), gWorld).xyz;
	output.PosH = mul(float4(output.PosW, 1.0f), gViewProj);
	output.NormalW = normalize(mul(input.NormalL, (float3x3)gWorldInvTranspose));
}

[maxvertexcount(2)]
void DefaultGS(
	triangle SVertexOut vertex[3] : SV_POSITION,
	inout LineStream<GSOutput> lineStream) {

	// vertex world positions
	float3 vwp[3];

	[unroll]
	for (int i = 0; i < 3; ++i) {
		vwp[i] = mul(float4(vertex[i].PosL, 1.0f), gWorld).xyz;
	}

	float3 u = vwp[1] - vwp[0];
	float3 v = vwp[2] - vwp[0];

	float3 normal = normalLength*normalize(cross(u, v));

	float3 center = 0.333333f*(vwp[0] + vwp[1] + vwp[2]);

	GSOutput element;
	element.PosW = center;
	element.PosH = mul(float4(element.PosW, 1.0f), gViewProj);
	element.NormalW = v;
	lineStream.Append(element);

	element.PosW += normal;
	element.PosH = mul(float4(element.PosW, 1.0f), gViewProj);
	lineStream.Append(element);
	lineStream.RestartStrip();
}

float4 DefaultPS(GSOutput pin) : SV_Target{
	return float4(1.0f, 0.0f, 0.0f, 1.0f);
}