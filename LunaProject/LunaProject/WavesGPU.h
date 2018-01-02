#pragma once
#ifndef WAVES_GPU_H
#define WAVES_GPU_H

#include "GameTimer.h"
using Microsoft::WRL::ComPtr;

class WavesGPU {
public:
	WavesGPU() = delete;
	WavesGPU(const WavesGPU& rhs) = delete;
	WavesGPU& operator=(const WavesGPU& rhs) = delete;

	WavesGPU(int m, int n, float dx, float dt, float speed, float damping,
		ComPtr<ID3D12GraphicsCommandList> cmdList, ComPtr<ID3D12Device> device);

	void Update(const GameTimer& gt);
	~WavesGPU();

public:
	CD3DX12_GPU_DESCRIPTOR_HANDLE DisplacementMap() const;

	int RowCount() const;
	int ColumnCount() const;
	int VertexCount() const;
	float SpatialStep() const;

	void BuildDescriptors(CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle,
		CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle,
		UINT descriptorSize);

	void Disturb(UINT i, UINT j, float magnitude);

	ComPtr<ID3D12Resource> CurrentSolution();

private:
	void CreateResources();
	void BuildRootSignature();
	void BuildPSO();

private:
	ComPtr<ID3D12RootSignature> mRootSignature;
private:
	int mNumRows = 0;
	int mNumCols = 0;

	int mVertexCount = 0;
	int mTriangleCount = 0;

	float mK[3];

	float mTimeStep = 0.0f;
	float mSpatialStep = 0.0f;

	bool mDisturbed = false;

	ComPtr<ID3D12GraphicsCommandList> mCmdList = nullptr;
	ComPtr<ID3D12Device> mDevice = nullptr;
	ComPtr<ID3D12Resource> mCurrSolution = nullptr;
	ComPtr<ID3D12Resource> mPrevSolution = nullptr;
	ComPtr<ID3D12Resource> mOutput = nullptr;

	ComPtr<ID3D12Resource> mPrevUploadBuffer = nullptr;
	ComPtr<ID3D12Resource> mCurrUploadBuffer = nullptr;

	std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> mPSOs;
	std::unordered_map<std::string, ComPtr<ID3DBlob>> mShaders;


	CD3DX12_GPU_DESCRIPTOR_HANDLE mPrevSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE mCurrSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE mOutputSrv;

	CD3DX12_GPU_DESCRIPTOR_HANDLE mPrevUav;
	CD3DX12_GPU_DESCRIPTOR_HANDLE mCurrUav;
	CD3DX12_GPU_DESCRIPTOR_HANDLE mOutputUav;
};

#endif