struct SVertexIn {
	float3 PosL : POSITION;
	float3 NormalL : NORMAL;
};

struct SVertexOut {
	float3 PosL : POSITION;
	float3 NormalL: NORMAL;
};

SVertexOut DefaultVS(SVertexIn v) {
	SVertexOut vOut;
	vOut.PosL = v.PosL;
	vOut.NormalL = v.NormalL;
	return vOut;
}
