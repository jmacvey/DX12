#ifndef BOX_H
#define BOX_H

#include "Vertex.h"
using Microsoft::WRL::ComPtr;

class Box {
public:
	static const char* GeometryName; 

	Box(ComPtr<ID3D12Device> pDevice, ComPtr<ID3D12GraphicsCommandList> pCmdList, UINT slotNumber = 0);

	std::array<VertexTypes::GenericVertex, 8> GetVertexList() const;

	std::array<std::uint16_t, 36> GetIndexList() const;

	std::vector<D3D12_INPUT_ELEMENT_DESC> GetInputElementLayout() const;

	std::unique_ptr<MeshGeometry> GetGeometry() const;

private:
	ComPtr<ID3D12Device> mpDevice;
	ComPtr<ID3D12GraphicsCommandList> mpCmdList;
	std::vector<D3D12_INPUT_ELEMENT_DESC> mLayout;
	std::array<VertexTypes::GenericVertex, 8> mVertexList;
	std::array<std::uint16_t, 36> mIndexList;
};

#endif