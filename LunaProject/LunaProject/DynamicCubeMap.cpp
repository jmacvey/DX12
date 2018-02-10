
#include "stdafx.h"
#include "DynamicCubeMap.h"

DynamicCubeMap::DynamicCubeMap(ID3D12Device * device, UINT width, UINT height, DXGI_FORMAT format) : mDevice(device),
	mWidth(width), mHeight(height), mFormat(format) {
	BuildViewportAndScissorRect();
}

ID3D12Resource* DynamicCubeMap::Resource()
{
	return mCubeMap.Get();
}

CD3DX12_GPU_DESCRIPTOR_HANDLE DynamicCubeMap::Srv()
{
	return mSrvGpuHandle;
}

CD3DX12_CPU_DESCRIPTOR_HANDLE DynamicCubeMap::Rtv(int & faceIndex)
{
	return mRtvCpuHandle[faceIndex];
}

D3D12_VIEWPORT DynamicCubeMap::Viewport() const
{
	return mViewport;
}

D3D12_RECT DynamicCubeMap::ScissorRect() const
{
	return mScissorRect;
}

void DynamicCubeMap::BuildDescriptors(CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv, CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv,
	CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuRtv[6])
{
	mSrvCpuHandle = hCpuSrv;
	mSrvGpuHandle = hGpuSrv;
	for (UINT i = 0; i < 6; ++i)
		mRtvCpuHandle[i] = hCpuRtv[i];

	BuildResource();
	BuildDescriptors();
}

void DynamicCubeMap::OnResize(UINT newWidth, UINT newHeight)
{
	if (newWidth != mWidth || newHeight != mHeight) {
		mWidth = newWidth;
		mHeight = newHeight;
		BuildViewportAndScissorRect();
		BuildResource();
		BuildDescriptors();
	}
}

void DynamicCubeMap::BuildDescriptors()
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
	ZeroMemory(&srvDesc, sizeof(D3D12_SHADER_RESOURCE_VIEW_DESC));

	srvDesc.Format = mFormat;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.TextureCube.MostDetailedMip = 0;
	srvDesc.TextureCube.MipLevels = 1;
	srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;

	mDevice->CreateShaderResourceView(mCubeMap.Get(), &srvDesc, mSrvCpuHandle);

	// RTV to each cube face
	for (UINT i = 0; i < 6; ++i) {
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc;
		rtvDesc.Format = mFormat;
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
		rtvDesc.Texture2DArray.MipSlice = 0;
		rtvDesc.Texture2DArray.FirstArraySlice = i;
		rtvDesc.Texture2DArray.ArraySize = 1;
		rtvDesc.Texture2DArray.PlaneSlice = 0;

		mDevice->CreateRenderTargetView(mCubeMap.Get(), &rtvDesc, mRtvCpuHandle[i]);
	}
}

void DynamicCubeMap::BuildResource()
{
	D3D12_RESOURCE_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Alignment = 0u;
	texDesc.Width = mWidth;
	texDesc.Height = mHeight;
	texDesc.DepthOrArraySize = 6;
	texDesc.MipLevels = 1;
	texDesc.Format = mFormat;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	D3D12_CLEAR_VALUE clearOpt;
	auto color = DirectX::Colors::LightSteelBlue;
	XMFLOAT4 colorf3 = {};
	XMStoreFloat4(&colorf3, color.v);
	clearOpt.Color[0] = colorf3.x;
	clearOpt.Color[1] = colorf3.y;
	clearOpt.Color[2] = colorf3.z;
	clearOpt.Color[3] = colorf3.w;
	clearOpt.Format = mFormat;

	ThrowIfFailed(mDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&texDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
		&clearOpt, IID_PPV_ARGS(&mCubeMap)));
}

void DynamicCubeMap::BuildViewportAndScissorRect()
{
	mViewport = { 0.0f, 0.0f, (float)mWidth, (float)mHeight, 0.0f, 1.0f };
	mScissorRect = { 0, 0, (LONG)mWidth, (LONG)mHeight };
}
