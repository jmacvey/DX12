#include "stdafx.h"
#include "Box.h"
#include "Vertex.h"


const char* Box::GeometryName = "Box";

Box::Box(ComPtr<ID3D12Device> pDevice, ComPtr<ID3D12GraphicsCommandList> pCommandList, UINT slotNumber) : mpDevice(pDevice),
	mpCmdList(pCommandList) { 
	using VertexTypes::GenericVertex;
	mLayout = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, slotNumber, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, slotNumber, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	mVertexList = {
		GenericVertex({ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::White) }),
		GenericVertex({ XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Black) }),
		GenericVertex({ XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Red) }),
		GenericVertex({ XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::Green) }),
		GenericVertex({ XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Blue) }),
		GenericVertex({ XMFLOAT3(-1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Yellow) }),
		GenericVertex({ XMFLOAT3(+1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Cyan) }),
		GenericVertex({ XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Magenta) })
	};

	mIndexList = {
		// font face
		0, 1, 2,
		0, 2, 3,

		// back face
		4, 6, 5,
		4, 7, 6,

		// left face
		4, 5, 1,
		4, 1, 0,

		// right face
		3, 2, 6,
		3, 6, 7,

		// bottom face
		4, 0, 3,
		4, 3, 7,

		// top face
		1, 5, 6,
		1, 6, 2
	};
}

std::array<VertexTypes::GenericVertex, 8> Box::GetVertexList() const {
	return mVertexList;
}

std::array<std::uint16_t, 36> Box::GetIndexList() const {
	return mIndexList;
}

std::vector<D3D12_INPUT_ELEMENT_DESC> Box::GetInputElementLayout() const { return mLayout; }

std::unique_ptr<MeshGeometry> Box::GetGeometry() const {
	auto geometry = std::make_unique<MeshGeometry>();
	geometry->Name = Box::GeometryName;


	const UINT vbByteSize = (UINT)mVertexList.size() * sizeof(VertexTypes::GenericVertex);
	const UINT ibByteSize = (UINT)mIndexList.size() * sizeof(std::uint16_t);
	
	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geometry->VertexBufferCPU));
	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geometry->IndexBufferCPU));

	CopyMemory(geometry->VertexBufferCPU->GetBufferPointer(), mVertexList.data(), vbByteSize);
	CopyMemory(geometry->IndexBufferCPU->GetBufferPointer(), mIndexList.data(), ibByteSize);

	d3dUtil::CreateDefaultBuffer(mpDevice.Get(), mpCmdList.Get(), mVertexList.data(), vbByteSize, geometry->VertexBufferGPU);
	d3dUtil::CreateDefaultBuffer(mpDevice.Get(), mpCmdList.Get(), mIndexList.data(), ibByteSize, geometry->IndexBufferGPU);

	geometry->VertexBufferByteSize = vbByteSize;
	geometry->IndexBufferByteSize = ibByteSize;
	geometry->VertexByteStride = sizeof(VertexTypes::GenericVertex);
	geometry->IndexFormat = DXGI_FORMAT_R16_UINT;

	SubmeshGeometry submesh;
	submesh.BaseVertexLocation = 0;
	submesh.IndexCount = (UINT)mIndexList.size();
	submesh.StartIndexLocation = 0;

	geometry->DrawArgs[Box::GeometryName] = submesh;
	return geometry;
}