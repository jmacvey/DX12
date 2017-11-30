cbuffer cbPerObject : register(b0) {
	float4x4 gWorld;
	float4x4 gTexTransform;
}

cbuffer cbPass : register(b1) {
	float4x4 gView;
	float4x4 gInvView;
	float4x4 gProj;
	float4x4 gInvProj;
	float4x4 gViewProj;
	float4x4 gInvViewProj;
	float3 gEyePos;
	float cbPerObjectPad1;
	float2 gRenderTargetSize;
	float2 gInvRenderTargetSize;
	float gNearZ;
	float gFarZ;
	float gTotalTime;
	float gDeltaTime;
	float4 gAmbientLight;
	float4 gFogColor;
	float gFogStart;
	float gFogRnage;
	float2 cbPerObjectPad2;
}

struct VertexIn {
	float3 PosL : POSITION;
	float3 NormalL : NORMAL;
};

struct VertexOut {
	float4 PosH : SV_POSITION;
	float3 PosW: POSITION;
	float3 NormalW : NORMAL;
};

VertexOut VS(VertexIn v) {
	VertexOut vOut = (VertexOut)0.0f;
	float4 posW = mul(float4(v.PosL, 1.0f), gWorld);
	vOut.PosW = posW.xyz;
	vOut.PosH = mul(posW, gViewProj);
	vOut.NormalW = mul(v.NormalL, (float3x3)gWorld);
	 return vOut;
	VertexOut vout = (VertexOut)0.0f;
}

float4 PS(VertexOut v) : SV_Target {
	return float4(0.0f, 0.0f, 0.0f, 0.0f);
}