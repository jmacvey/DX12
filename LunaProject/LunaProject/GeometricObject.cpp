#include "stdafx.h"
#include "GeometricObject.h"

const std::string GeometricObject::SubmeshName = "submesh";

GeometricObject::GeometricObject(const std::string& name) : mName(name)
{
}

void GeometricObject::AddObject(GeometryGenerator::MeshData& meshData)
{
	TryInitializeGeometry();
	AddObject(meshData.Vertices, meshData.GetIndices16());
}

void GeometricObject::AddObject(const std::vector<GeometryGenerator::Vertex>& vertexList, const std::vector<uint16_t>& indexList)
{
	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indexList.size();
	submesh.StartIndexLocation = (UINT)mIndices.size();
	submesh.BaseVertexLocation = (UINT)mVertices.size();
	mGeo->DrawArgs[GeometricObject::SubmeshName + std::to_string(mSize++)] = submesh;

	mIndices.insert(mIndices.end(), indexList.begin(), indexList.end());
	mVertices.insert(mVertices.end(), vertexList.begin(), vertexList.end());
	ResetBufferSizes();
}

void GeometricObject::BuildGeometry(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	if (mSize == 0) {
		return;
	}

	ThrowIfFailed(D3DCreateBlob(mGeo->VertexBufferByteSize, &mGeo->VertexBufferCPU));
	ThrowIfFailed(D3DCreateBlob(mGeo->IndexBufferByteSize, &mGeo->IndexBufferCPU));

	CopyMemory(mGeo->VertexBufferCPU->GetBufferPointer(), mVertices.data(), mGeo->VertexBufferByteSize);
	CopyMemory(mGeo->IndexBufferCPU->GetBufferPointer(), mIndices.data(), mGeo->IndexBufferByteSize);

	mGeo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(pd3dDevice, pd3dCommandList,
		mVertices.data(), mGeo->VertexBufferByteSize,
		mGeo->VertexBufferUploader);

	mGeo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(pd3dDevice, pd3dCommandList,
		mIndices.data(), mGeo->IndexBufferByteSize,
		mGeo->IndexBufferUploader);
}
SubmeshGeometry GeometricObject::GetSubmesh(uint32_t submeshIndex) const {
	return mGeo->DrawArgs[GeometricObject::SubmeshName + std::to_string(submeshIndex)];
}

void GeometricObject::ResetBufferSizes()
{
	auto vbByteSize = (UINT)mVertices.size() * sizeof(GeometryGenerator::Vertex);
	auto ibByteSize = (UINT)mIndices.size() * sizeof(uint16_t);

	mGeo->IndexBufferByteSize = ibByteSize;
	mGeo->VertexBufferByteSize = vbByteSize;
}

void GeometricObject::TryInitializeGeometry()
{
	if (mGeo == nullptr) {
		mGeo = std::make_unique<MeshGeometry>();
		mGeo->Name = mName;
		mGeo->IndexFormat = DXGI_FORMAT_R16_UINT;
		mGeo->VertexByteStride = sizeof(GeometryGenerator::Vertex);
	}
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