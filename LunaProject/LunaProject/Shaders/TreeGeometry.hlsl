#include "buffers.hlsl"

struct GeoVertexIn {
	float3 PosW : POSITION;
	float2 SizeW : SIZE;
};

struct GeoVertexOut {
	float3 CenterW : POSITION;
	float2 SizeW : SIZE;
};

struct GeoOut {
	float4 PosH : SV_POSITION;
	float3 PosW : POSITION;
	float3 NormalW : NORMAL;
	float2 TexC : TEXCOORD;
	uint PrimID : SV_PrimitiveID;
};

GeoVertexOut GeoVS(GeoVertexIn vIn) {
	GeoVertexOut vOut;
	vOut.CenterW = vIn.PosW;
	vOut.SizeW = vIn.SizeW;
	return vOut;
};

[maxvertexcount(4)]
void GeoGS(point GeoVertexOut geoIn[1],
	uint primID: SV_PrimitiveID,
	inout TriangleStream<GeoOut> triStream) {

	// compute local coordinate space of triangle
	float3 up = float3(0.0f, 1.0f, 0.0f);
	float3 look = gEyePosW - geoIn[0].CenterW;
	look.y = 0.0f; // y-axis aligned
	look = normalize(look);
	float3 w = cross(up, look);

	float halfWidth = 0.5f*geoIn[0].SizeW.x;
	float halfHeight = 0.5f*geoIn[0].SizeW.y;

	float4 geos[4];
	geos[0] = float4(geoIn[0].CenterW + halfWidth*w - halfHeight*up, 1.0f);
	geos[1] = float4(geoIn[0].CenterW + halfWidth*w + halfHeight*up, 1.0f);
	geos[2] = float4(geoIn[0].CenterW - halfWidth*w - halfHeight*up, 1.0f);
	geos[3] = float4(geoIn[0].CenterW - halfWidth*w + halfHeight*up, 1.0f);

	float2 texC[4];
	texC[0] = float2(0.0f, 1.0f);
	texC[1] = float2(0.0f, 0.0f);
	texC[2] = float2(1.0f, 1.0f);
	texC[3] = float2(1.0f, 0.0f);

	GeoOut geoOut;
	[unroll]
	for (int i = 0; i < 4; ++i) {
		geoOut.PosH = mul(geos[i], gViewProj);
		geoOut.PosW = geos[i].xyz;
		geoOut.NormalW = look;
		geoOut.TexC = texC[i];
		geoOut.PrimID = primID;

		triStream.Append(geoOut);
	}
}

float4 GeoPS(GeoOut pin) : SV_Target {
	float3 uvw = float3(pin.TexC, pin.PrimID % 3);
	float4 diffuseAlbedo = gDiffuseMapArray.Sample(gsamAnisotropicWrap, uvw);

#ifdef ALPHA_TEST
	clip(diffuseAlbedo.a - 0.1f);
#endif

	pin.NormalW = normalize(pin.NormalW);

	float3 toEyeW = gEyePosW - pin.PosW;
	float distToEye = length(toEyeW);
	toEyeW /= distToEye;

	float4 ambient = gAmbientLight*diffuseAlbedo;

	const float shininess = 1.0f - gRoughness;
	Material mat = { diffuseAlbedo, gFresnelR0, shininess };
	float3 shadowFactor = 1.0f;
	float4 directLight = ComputeLighting(gLights, mat, pin.PosW,
		pin.NormalW, toEyeW, shadowFactor);

	float4 litColor = ambient + directLight;

#ifdef FOG
	float4 fogAmount = saturate((distToEye - gFogStart) / gFogRange);
	litColor = lerp(litColor, gFogColor, fogAmount);
#endif

	litColor.a = diffuseAlbedo.a;

	return litColor;
}