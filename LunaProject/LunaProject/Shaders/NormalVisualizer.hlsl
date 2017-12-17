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
	GSOutput element;

	[unroll]
	for (uint i = 0; i < 3; ++i) {
		ConvertVToG(vertex[i], element);
		lineStream.Append(element);

		element.PosW = element.PosW + normalLength*element.NormalW;
		element.PosH = mul(float4(element.PosW, 1.0f), gViewProj);
		lineStream.Append(element);
		lineStream.RestartStrip();
	}
}

float4 DefaultPS(GSOutput pin) : SV_Target {
	return float4(1.0f, 0.0f, 0.0f, 1.0f);
}