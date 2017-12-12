#include "buffers.hlsl"

struct SVertexIn {
	float3 PosL : POSITION;
	float3 NormalL : NORMAL;
};

struct SVertexOut {
	float3 PosL : POSITION;
	float3 NormalL: NORMAL;
};

struct GSOutput
{
	float4 PosH : SV_POSITION;
	float3 PosW : POSITION;
	float3 NormalW: NORMAL;
};


SVertexOut SVS(SVertexIn v) {
	SVertexOut vOut;
	vOut.PosL = v.PosL;
	vOut.NormalL = v.NormalL;
	return vOut;
}

void ConvertVertexToGSOut(SVertexOut input, out GSOutput output) {
	output.PosW = mul(float4(input.PosL, 1.0f), gWorld).xyz;
	output.NormalW = mul(input.NormalL, (float3x3)gWorldInvTranspose);

	output.PosH = mul(float4(output.PosW, 1.0f), gViewProj);
}

//       1
//       *
//      / \
//     /   \
// m0 *-----* m1
//   / \   / \
//  /   \ /   \  
// *-----*-----*
// 0     m2     2
// 0 -> 0, m0 -> 1, m2 -> 2, m1 -> 3, 2 -> 4, 1 -> 5
void Subdivide(SVertexOut inVerts[3], out SVertexOut outVerts[6]) {
	SVertexOut m[3];

	m[0].PosL = 0.5f*(inVerts[0].PosL + inVerts[1].PosL);
	m[1].PosL = 0.5f*(inVerts[1].PosL + inVerts[2].PosL);
	m[2].PosL = 0.5f*(inVerts[2].PosL + inVerts[0].PosL);
	
	[unroll]
	for (int i = 0; i < 3; ++i) {
		m[i].PosL = normalize(m[i].PosL);
		m[i].NormalL = m[i].PosL;
	}

	outVerts[0] = inVerts[0];
	outVerts[1] = m[0];
	outVerts[2] = m[2];
	outVerts[3] = m[1];
	outVerts[4] = inVerts[2];
	outVerts[5] = inVerts[1];
}

void OutputSubdivision(SVertexOut v[6], inout TriangleStream<GSOutput> triStream) {
	GSOutput gOut[6];

	[unroll]
	for (int i = 0; i < 6; ++i) {
		ConvertVertexToGSOut(v[i], gOut[i]);
	}

	// draw output as two strips
	[unroll]
	for (int j = 0; j < 5; ++j) {
		triStream.Append(gOut[j]);
	}

	triStream.RestartStrip();
	triStream.Append(gOut[1]);
	triStream.Append(gOut[5]);
	triStream.Append(gOut[3]);

	triStream.RestartStrip();
}

void MapToTriangle(SVertexOut inputs[6], uint indices[3], out SVertexOut newTriangle[3]) {
	// map inputs at specified indices to output for new triangle
	[unroll]
	for (int i = 0; i < 3; ++i) {
		newTriangle[i] = inputs[indices[i]];
	}
}

//       1
//       *
//      / \
//     /   \
// m0 *-----* m1
//   / \   / \
//  /   \ /   \  
// *-----*-----*
// 0     m2     2
// Index Mappings To Output: 0: 0, m0: 1, m2: 2, m1: 3, 2: 4, 1: 5
void SubdivideN(SVertexOut inputs[3], uint N, inout TriangleStream<GSOutput> triStream) {
	uint indices[3];

	SVertexOut firstSub[6];

	Subdivide(inputs, firstSub);
	if (N == 1) {
		OutputSubdivision(firstSub, triStream);
		return;
	}

	SVertexOut nextSub[6];

	[unroll]
	for (int j = 0; j < 3; ++j) {
		indices[0] = j;
		indices[1] = j + 1;
		indices[2] = j + 2;
		
		// create new inputs from outputs
		MapToTriangle(firstSub, indices, inputs);

		// subdivide and output
		Subdivide(inputs, nextSub);
		OutputSubdivision(nextSub, triStream);
	}

	// subdivide last triangle in the strip
	indices[0] = 1;
	indices[1] = 5;
	indices[2] = 3;
	MapToTriangle(firstSub, indices, inputs);
	Subdivide(inputs, nextSub);
	OutputSubdivision(nextSub, triStream);
}

// up to 64 vertices
[maxvertexcount(64)]
void SGS(
	triangle SVertexOut input[3] : SV_POSITION, 
	inout TriangleStream<GSOutput> triStream)
{
	// center of object is just the translation vector
	// since we expect objects to be centered at origin
	float3 center = float3(gWorld._m00, gWorld._m01, gWorld._m02);
	float dist = length(gEyePosW - center);

	if (dist < 50) {
		SubdivideN(input, 2, triStream);
	} else if (dist < 80) {
		SubdivideN(input, 1, triStream);
	}
	else {
		GSOutput gOut;
		[unroll]
		for (int i = 0; i < 3; ++i) {
			ConvertVertexToGSOut(input[i], gOut);
			triStream.Append(gOut);
		}
	}
}

float4 SPS(GSOutput pin) : SV_Target
{
	return float4(0.0f, 0.0f, 0.0f, 0.0f);
}