#include "stdafx.h"
#include "Pyramid.h"

const char* const Pyramid::GeometryName = "PYRAMID";

Pyramid::Pyramid(ComPtr<ID3D12Device> pDevice, ComPtr<ID3D12GraphicsCommandList> pCmdList, UINT inputSlot) : mpDevice(pDevice), mpCmdList(pCmdList) {
	mInputLayout = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, inputSlot, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, inputSlot, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
}

std::array<std::uint16_t, Pyramid::PyramidIndexSize> Pyramid::GetIndexList() const {
	return {
		// bottom left
		0, 2, 1,
		0, 3, 2,

		// sides
		0, 1, 4,
		1, 2, 4,
		2, 3, 4,
		3, 0, 4
	};
}

std::array<VertexTypes::GenericVertex, 5> Pyramid::GetVertexList() const {
	using VertexTypes::GenericVertex;
	return {
		GenericVertex({ XMFLOAT3(+1.0, +0.0, +1.0), XMFLOAT4(Colors::Green) }),
		GenericVertex({ XMFLOAT3(+1.0, +0.0, -1.0), XMFLOAT4(Colors::Green) }),
		GenericVertex({ XMFLOAT3(-1.0, +0.0, -1.0), XMFLOAT4(Colors::Green) }),
		GenericVertex({ XMFLOAT3(-1.0, +0.0, +1.0), XMFLOAT4(Colors::Green) }),
		GenericVertex({ XMFLOAT3(+0.0, (1.0/3.0)*XM_PI, +0.0), XMFLOAT4(Colors::Red) })
	};
}

std::unique_ptr<MeshGeometry> Pyramid::GetGeometry() const {
	auto vertices = GetVertexList();
	auto indices = GetIndexList();

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(VertexTypes::GenericVertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geometry = std::make_unique<MeshGeometry>();
	geometry->Name = "Pyramid";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geometry->VertexBufferCPU));
	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geometry->IndexBufferCPU));

	CopyMemory(geometry->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);
	CopyMemory(geometry->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geometry->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(mpDevice.Get(), mpCmdList.Get(), vertices.data(), vbByteSize, geometry->VertexBufferUploader);
	geometry->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(mpDevice.Get(), mpCmdList.Get(), indices.data(), ibByteSize, geometry->IndexBufferUploader);

	geometry->VertexBufferByteSize = vbByteSize;
	geometry->VertexByteStride = sizeof(VertexTypes::GenericVertex);
	geometry->IndexFormat = DXGI_FORMAT_R16_UINT;
	geometry->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry submesh;
	submesh.BaseVertexLocation = 0;
	submesh.StartIndexLocation = 0;
	submesh.IndexCount = (UINT)indices.size();

	geometry->DrawArgs[Pyramid::GeometryName] = submesh;
	return geometry;
}

std::vector<D3D12_INPUT_ELEMENT_DESC> Pyramid::GetInputLayoutDescription() const {
	return mInputLayout;
}

