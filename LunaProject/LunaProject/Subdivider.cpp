#include "stdafx.h"
#include "Subdivider.h"

Subdivider::Subdivider(ComPtr<ID3D12GraphicsCommandList> cmdList, ComPtr<ID3D12Device> device) : mCmdList(cmdList), mDevice(device)
{
	mInputLayout = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
}

Subdivider::~Subdivider()
{
}

D3D12_INPUT_LAYOUT_DESC Subdivider::GetInputLayout() const
{
	return {
		mInputLayout.data(),
		(UINT)mInputLayout.size()
	};
}

void Subdivider::BuildGeometry()
{
	GeometryGenerator geoGen;
	auto ico = geoGen.CreateGeosphere(1.0f, 0);

	UINT numVertices = (UINT)ico.Vertices.size();
	UINT vbByteSize = numVertices * sizeof(LocalVertex);

	std::vector<LocalVertex> vertices(numVertices);


	for (UINT i = 0; i < numVertices; ++i) {
		vertices[i].Pos = ico.Vertices[i].Position;
		vertices[i].Normal = ico.Vertices[i].Normal;
	};

	UINT numIndices = (UINT)ico.GetIndices16().size();
	UINT ibByteSize = numIndices * sizeof(std::uint16_t);
	auto indices = ico.GetIndices16();

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "subdivider";
	
	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));

	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(mDevice.Get(), mCmdList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);
	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(mDevice.Get(), mCmdList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->VertexByteStride = sizeof(LocalVertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry icoSubmesh;
	icoSubmesh.IndexCount = numIndices;
	geo->DrawArgs["icosahedron"] = icoSubmesh;
	mGeo = std::move(geo);
}
