#include "stdafx.h"
#include "Sobel.h"

Sobel::Sobel(ComPtr<ID3D12Device> device, ComPtr<ID3D12GraphicsCommandList> cmdList,
	UINT width, UINT height, DXGI_FORMAT format) : mDevice(device), mCmdList(cmdList),
		mWidth(width), mHeight(height), mFormat(format) {
	CreateResources();
	BuildRootSignature();
	BuildPSOs();
}

void Sobel::BuildDescriptors() {
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = mFormat;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.PlaneSlice = 0;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = mFormat;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	uavDesc.Texture2D.MipSlice = 0;
	uavDesc.Texture2D.PlaneSlice = 0;

	mDevice->CreateShaderResourceView(mInput.Get(), &srvDesc, mInputCpuSrv);

	mDevice->CreateUnorderedAccessView(mOutput.Get(), nullptr, &uavDesc, mOutputCpuUav);
}

void Sobel::BuildDescriptors(CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle, CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle,
	UINT descriptorSize) {
	mInputCpuSrv = cpuHandle;
	mOutputCpuUav = cpuHandle.Offset(1, descriptorSize);

	mInputGpuSrv = gpuHandle;
	mOutputGpuUav = gpuHandle.Offset(1, descriptorSize);

	BuildDescriptors();
}

void Sobel::OnResize(UINT width, UINT height) {
	if (width != mWidth || height != mHeight) {
		mWidth = width;
		mHeight = height;
		CreateResources();
		BuildDescriptors();
	}
}

void Sobel::Execute(ID3D12Resource* input, D3D12_RESOURCE_STATES prevState)
{
	mCmdList->SetComputeRootSignature(mRootSignature.Get());
	mCmdList->SetPipelineState(mPSOs["sobel"].Get());

	mCmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(input,
		prevState, D3D12_RESOURCE_STATE_COPY_SOURCE));

	mCmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mInput.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));

	mCmdList->CopyResource(mInput.Get(), input);

	mCmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mInput.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));

	mCmdList->SetComputeRootDescriptorTable(0, mInputGpuSrv);
	mCmdList->SetComputeRootDescriptorTable(1, mOutputGpuUav);

	auto numGroupsX = ceil(mWidth / 16);
	auto numGroupsY = ceil(mHeight / 16);
	mCmdList->Dispatch(numGroupsX, numGroupsY, 1);

	mCmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(input, 
		D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COPY_DEST));
	mCmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mOutput.Get(),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE));

	mCmdList->CopyResource(input, mOutput.Get());

	// restore resource states

	mCmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(input,
		D3D12_RESOURCE_STATE_COPY_DEST, prevState));

	mCmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mOutput.Get(),
		D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

	mCmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mInput.Get(),
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COMMON));
}

void Sobel::CreateResources() {
	auto resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		mFormat,
		mWidth,
		mHeight,
		1u,
		1u,
		1u,
		0u,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
	);
	
	ThrowIfFailed(mDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		nullptr,
		IID_PPV_ARGS(mOutput.GetAddressOf())
	));

	resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	ThrowIfFailed(mDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(mInput.GetAddressOf())
	));
}

void Sobel::BuildRootSignature() {
	const std::uint16_t numParams = 2;
	CD3DX12_ROOT_PARAMETER params[numParams];
	
	auto srvTable0 = CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

	auto uavTable0 = CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);
	params[0].InitAsDescriptorTable(1, &srvTable0); // Srv in param 0
	params[1].InitAsDescriptorTable(1, &uavTable0); // uav in param 1

	auto rootSigDesc = CD3DX12_ROOT_SIGNATURE_DESC(numParams, params, 0u, nullptr,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> rootSig;
	ComPtr<ID3DBlob> error;

	auto hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, rootSig.GetAddressOf(), error.GetAddressOf());

	if (error != nullptr) {
		::OutputDebugStringA((char*)error->GetBufferPointer());
	}

	ThrowIfFailed(hr);

	ThrowIfFailed(mDevice->CreateRootSignature(0, rootSig->GetBufferPointer(),
		rootSig->GetBufferSize(), IID_PPV_ARGS(&mRootSignature)));
}

void Sobel::BuildPSOs() {

	mShaders["sobel"] = d3dUtil::CompileShader(L"Shaders\\Sobel.hlsl", nullptr, "SobelCS", "cs_5_0");

	D3D12_COMPUTE_PIPELINE_STATE_DESC computeDesc;
	ZeroMemory(&computeDesc, sizeof(D3D12_COMPUTE_PIPELINE_STATE_DESC));

	computeDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	computeDesc.NodeMask = 0;
	computeDesc.pRootSignature = mRootSignature.Get();
	computeDesc.CS = {
		reinterpret_cast<BYTE*>(mShaders["sobel"]->GetBufferPointer()),
		(UINT)mShaders["sobel"]->GetBufferSize()
	};

	ThrowIfFailed(mDevice->CreateComputePipelineState(&computeDesc,
		IID_PPV_ARGS(&mPSOs["sobel"])));
}
