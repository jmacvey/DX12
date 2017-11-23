#ifndef GEOMETRIC_OBJECT_H
#define GEOMETIRC_OBJECT_H

#include "d3dUtil.h"
#include "GeometryGenerator.h"
#include "FrameResource.h"
#include <functional>

// fwd declarations
using Microsoft::WRL::ComPtr;

class GeometricObject;
template<typename vertexType>
std::vector<vertexType> getVertices(GeometricObject*, std::function<vertexType(const GeometryGenerator::Vertex&)>);

class GeometricObject {

	template<typename vertexType>
	friend std::vector<vertexType> getVertices(GeometricObject*, std::function<vertexType(const GeometryGenerator::Vertex&)>);

public:
	static const std::string SubmeshName;
	GeometricObject() = delete;
	GeometricObject(const std::string& name);

	GeometricObject(const GeometricObject& rhs) = delete;

	GeometricObject& operator=(const GeometricObject& rhs) = delete;

	void AddObject(GeometryGenerator::MeshData& meshData);
	void AddObject(const std::vector<GeometryGenerator::Vertex>& vertexList, const std::vector<uint16_t>& indexList);

	void BuildGeometry(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);

	std::vector<D3D12_INPUT_ELEMENT_DESC> GetInputLayout() const;
	void SetInputLayout(const std::vector<D3D12_INPUT_ELEMENT_DESC>& inputLayout);

	std::vector<uint16_t>& GetIndices();

	MeshGeometry* GetGeometry();

	SubmeshGeometry GetSubmesh(uint32_t submeshIndex) const;
private:

	void ResetBufferSizes();
	void TryInitializeGeometry();
	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;
	std::unique_ptr<MeshGeometry> mGeo;
	std::string mName;
	uint32_t mSize = 0;
	std::vector<GeometryGenerator::Vertex> mVertices;
	std::vector<uint16_t> mIndices;

};

template<typename vertexType>
std::vector<vertexType> getVertices(GeometricObject* geo, std::function<vertexType(const GeometryGenerator::Vertex&)> convertVertex) {
	std::vector<vertexType> toReturn;
	auto convertAndPush = [&](const GeometryGenerator::Vertex& toConvert) {
		vertexType v = convertVertex(toConvert);
		toReturn.emplace_back(std::move(v));
	};
	std::for_each(geo->mVertices.begin(), geo->mVertices.end(), convertAndPush);
	return toReturn;
};

#endif
