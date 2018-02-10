#pragma once
#ifndef DYNAMIC_CUBE_MAP_H
#define DYNAMIC_CUBE_MAP_H

using namespace DirectX;

class DynamicCubeMap {
public:
	DynamicCubeMap(ID3D12Device* device, UINT width, UINT height, DXGI_FORMAT format);
	DynamicCubeMap(const DynamicCubeMap& rhs) = delete;
	DynamicCubeMap& operator=(const DynamicCubeMap& rhs) = delete;
	~DynamicCubeMap() = default;

	ID3D12Resource* Resource();
	CD3DX12_GPU_DESCRIPTOR_HANDLE Srv();
	CD3DX12_CPU_DESCRIPTOR_HANDLE Rtv(int& faceIndex);

	D3D12_VIEWPORT Viewport() const;
	D3D12_RECT ScissorRect() const;

	void BuildDescriptors(CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv,
		CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv,
		CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuRtv[6]);

	void OnResize(UINT newWidth, UINT newHeight);

private:
	void BuildDescriptors();
	void BuildResource();
	void BuildViewportAndScissorRect();
private:
	ID3D12Device* mDevice;
	D3D12_VIEWPORT mViewport;
	D3D12_RECT mScissorRect;

	// Rtv handle for each face
	CD3DX12_CPU_DESCRIPTOR_HANDLE mRtvCpuHandle[6];

	// writes to the srv
	CD3DX12_CPU_DESCRIPTOR_HANDLE mSrvCpuHandle;
	CD3DX12_GPU_DESCRIPTOR_HANDLE mSrvGpuHandle;

	DXGI_FORMAT mFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	
	UINT mWidth = 0u;
	UINT mHeight = 0u;
	Microsoft::WRL::ComPtr<ID3D12Resource> mCubeMap = nullptr;
	std::unique_ptr<DynamicCubeMap> mDynamicCubeMap = nullptr;
};

#endif