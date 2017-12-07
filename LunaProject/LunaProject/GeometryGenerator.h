#ifndef GEOMETRY_GENERATOR_H
#define GEOMETRY_GENERATOR_H

using namespace DirectX;

class GeometryGenerator {
public:

	using uint16 = std::uint16_t;
	using uint32 = std::uint32_t;

	struct Vertex {
		Vertex() {}
		Vertex(
			const XMFLOAT3& p,
			const XMFLOAT3& n,
			const XMFLOAT3& t,
			const XMFLOAT2& uv
		) : Position(p), Normal(n), TangentU(t), TexC(uv) {};

		Vertex(
			float px, float py, float pz,
			float nx, float ny, float nz,
			float tx, float ty, float tz,
			float u, float v
		) : Position(px, py, pz),
			Normal(nx, ny, nz),
			TangentU(tx, ty, tz),
			TexC(u, v) {};

		XMFLOAT3 Position;
		XMFLOAT3 Normal;
		XMFLOAT3 TangentU;
		XMFLOAT2 TexC;
	};

	struct MeshData {
		std::vector<Vertex> Vertices;
		std::vector<uint32> Indices32;

		std::vector<uint16>& GetIndices16() {
			if (mIndices16.empty()) {
				std::for_each(
					Indices32.begin(),
					Indices32.end(),
					[&](size_t index) { mIndices16.emplace_back(static_cast<uint16>(index)); });
			}
			return mIndices16;
		}
	private:
		std::vector<uint16> mIndices16;
	};

	MeshData CreateBox(float width, float height, float depth, uint32 numSubdivisions);

	MeshData CreateSphere(float radius, uint32 sliceCount, uint32 stackCount);

	MeshData CreateGeosphere(float radius, uint32 numSubdivisions);

	MeshData CreateCylinder(float bottomRadius, float topRadius, float height, uint32 sliceCount, uint32 stackCount, bool withTopBottom = true);

	MeshData CreateGrid(float width, float depth, uint32 m, uint32 n);

	MeshData CreateHyperboloidOneSheet(float height, float a, float b, float c, uint32 sliceCount, uint32 stackCount);

	MeshData CreateEllipsoid(float a, float b, float c, uint32 sliceCount, uint32 stackCount);

	MeshData CreatePyramid(float width, float height, float p, uint32 numSubdivisions);

	MeshData CreateObjectFromXYTrace(const std::vector<XMFLOAT2>& points, uint32 sliceCount, float degrees);
private:

	void Subdivide(MeshData& meshData);

	Vertex MidPoint(const Vertex& v0, const Vertex& v1);

	void BuildCylinderTopCap(float bottomRadius, float topRadius, float height, uint32 sliceCount, uint32 stackCount, MeshData& meshData);

	void BuildCylinderBottomCap(float bottomRadius, float topRadius, float height, uint32 sliceCount, uint32 stackCount, MeshData& meshData);

	void BuildHyperboloidCap(float height, float a, float b, float c, uint32 sliceCount, uint32 stackCount, MeshData& meshData, bool bottom = false);

	float CalculateMagRhoForHyperboloid(float theta, float k, float a, float b, float c);

	float CalculateMagRhoForEllipsoid(float theta, float k, float a, float b, float c);
};

#endif