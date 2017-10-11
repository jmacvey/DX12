#ifndef PYRAMID_H
#define PYRAMID_H

#include "Vertex.h"
#include <string>

using Microsoft::WRL::ComPtr;

class Pyramid {
public:
	static const UINT PyramidIndexSize = 18;

	static const char* const GeometryName;

	Pyramid(ComPtr<ID3D12Device> pDevice, ComPtr<ID3D12GraphicsCommandList> pCmdList, UINT inputSlot = 0);

	std::array<std::uint16_t, Pyramid::PyramidIndexSize> GetIndexList() const;

	std::array<VertexTypes::EfficientColorVertex, 5> GetVertexList() const;

	std::unique_ptr<MeshGeometry> GetGeometry() const;

	std::vector<D3D12_INPUT_ELEMENT_DESC> GetInputLayoutDescription() const;

private:
	ComPtr<ID3D12Device> mpDevice;
	ComPtr<ID3D12GraphicsCommandList> mpCmdList;
	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;
};

#endif