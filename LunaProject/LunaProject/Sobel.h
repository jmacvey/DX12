#pragma once
#ifndef SOBEL_H
#define SOBEL_H

using Microsoft::WRL::ComPtr;

class Sobel {
public:
	Sobel() = delete;
	Sobel(const Sobel& rhs) = delete;
	Sobel& operator=(const Sobel& rhs) = delete;
	Sobel(ComPtr<ID3D12Device> device, ComPtr<ID3D12GraphicsCommandList> cmdList, UINT width, UINT height, DXGI_FORMAT format);

	void BuildDescriptors(CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle, CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle,
		UINT descriptorSize);

	void OnResize(UINT width, UINT height);

	void Execute(ID3D12Resource* input, D3D12_RESOURCE_STATES mPrevState);

private:
	void CreateResources();
	void BuildDescriptors();
	void BuildRootSignature();
	void BuildPSOs();
	
private:
	ComPtr<ID3D12Device> mDevice = nullptr;
	ComPtr<ID3D12GraphicsCommandList> mCmdList = nullptr;
	ComPtr<ID3D12RootSignature> mRootSignature = nullptr;
	ComPtr<ID3D12Resource> mOutput = nullptr;
	ComPtr<ID3D12Resource> mInput = nullptr;

	std::unordered_map<std::string, ComPtr<ID3DBlob>> mShaders;
	std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> mPSOs;


	CD3DX12_GPU_DESCRIPTOR_HANDLE mOutputGpuUav;
	CD3DX12_GPU_DESCRIPTOR_HANDLE mInputGpuSrv;
	CD3DX12_CPU_DESCRIPTOR_HANDLE mOutputCpuUav;
	CD3DX12_CPU_DESCRIPTOR_HANDLE mInputCpuSrv;

	UINT mWidth;
	UINT mHeight;
	DXGI_FORMAT mFormat;
};

#endif // SOBEL_H