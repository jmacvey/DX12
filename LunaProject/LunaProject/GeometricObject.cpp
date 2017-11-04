#include "stdafx.h"
#include "GeometricObject.h"

const std::string GeometricObject::SubmeshName = "submesh";

GeometricObject::GeometricObject(const std::string& name) : mName(name)
{
}

void GeometricObject::BuildGeometry(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, GeometryGenerator::MeshData& meshData)
{
	auto vbByteSize = (UINT)meshData.Vertices.size() * sizeof(GeometryGenerator::Vertex);
	auto ibByteSize = (UINT)meshData.GetIndices16().size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = mName;

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));

	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), meshData.Vertices.data(), vbByteSize);
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), meshData.GetIndices16().data(), ibByteSize);

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(pd3dDevice, pd3dCommandList,
		meshData.Vertices.data(), vbByteSize,
		geo->VertexBufferUploader);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(pd3dDevice, pd3dCommandList,
		meshData.GetIndices16().data(), ibByteSize,
		geo->IndexBufferUploader);

	geo->IndexBufferByteSize = ibByteSize;
	geo->VertexBufferByteSize = vbByteSize;
	geo->VertexByteStride = sizeof(GeometryGenerator::Vertex);
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)meshData.GetIndices16().size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	geo->DrawArgs["submesh"] = submesh;

	geo.swap(mGeo);
}

std::vector<D3D12_INPUT_ELEMENT_DESC> GeometricObject::GetInputLayout() const {
	return mInputLayout;
}

void GeometricObject::SetInputLayout(const std::vector<D3D12_INPUT_ELEMENT_DESC>& inputLayout) {
	mInputLayout = inputLayout;
}

MeshGeometry* GeometricObject::GetGeometry() {
	return mGeo.get();
}