
cbuffer cbPerObject : register(b0) {
	float4x4 gWorldViewProj;
	float gTime;
};

struct VertexIn {
	float3 PosL : POSITION;
	float4 Color : COLOR;
};

struct VertexOut {
	float4 PosH : SV_POSITION;
	float4 Color : COLOR;
};

float4 EaseToColor(float4 colorIn, float4 colorTint) {
	float4 c = colorTint - colorIn;
	return c * min(1.0, gTime / 10.0) + colorIn;
}

//float4 SmoothPulse(float4 colorIn) {
//	const float pi = 3.14159;
//	
//	float s = 0.5f*sin(2 * gTime - 0.25f*pi) + 0.5f;
//
//	float4 c = lerp(colorIn, gPulseColor, s);
//	
//	return c;
//}

VertexOut VS(VertexIn vin) {
	VertexOut vout;

	vout.PosH = mul(float4(vin.PosL, 1.0f), gWorldViewProj);

	vout.Color = vin.Color;

	return vout;
};

float4 PS(VertexOut pin) : SV_Target {
	// return EaseToColor(pin.Color, float4(1.0f, .7f, 0.0f, 0.0f));
	// return SmoothPulse(pin.Color);
	return pin.Color;
};
