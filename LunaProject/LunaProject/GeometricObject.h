#ifndef GEOMETRIC_OBJECT_H
#define GEOMETIRC_OBJECT_H

#include "d3dUtil.h"
#include "GeometryGenerator.h"

using Microsoft::WRL::ComPtr;
class GeometricObject {
public:
	static const std::string SubmeshName;
	GeometricObject() = delete;
	GeometricObject(const std::string& name);

	GeometricObject(const GeometricObject& rhs) = delete;

	GeometricObject& operator=(const GeometricObject& rhs) = delete;

	void BuildGeometry(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, GeometryGenerator::MeshData& meshData);

	std::vector<D3D12_INPUT_ELEMENT_DESC> GetInputLayout() const;
	void SetInputLayout(const std::vector<D3D12_INPUT_ELEMENT_DESC>& inputLayout);

	MeshGeometry* GetGeometry();


private:
	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;
	std::unique_ptr<MeshGeometry> mGeo;
	std::string mName;
};

#endif
