#include "stdafx.h"
#include "WavesGPU.h"

WavesGPU::WavesGPU(int m, int n, float dx, float dt, float speed, float damping,
	ComPtr<ID3D12GraphicsCommandList> cmdList, ComPtr<ID3D12Device> device) : mCmdList(cmdList), mDevice(device)
{

	mNumRows = m;
	mNumCols = n;
	mVertexCount = m*n;
	mTriangleCount = (m - 1)*(n - 1)*mVertexCount;

	mTimeStep = dt;
	mSpatialStep = dx;
	mTimeStep = dt;
	mSpatialStep = dx;

	float d = damping*dt + 2.0f;
	float e = (speed*speed)*(dt*dt) / (dx*dx);
	mK[0] = (damping*dt - 2.0f) / d;
	mK[1] = (4.0f - 8.0f*e) / d;
	mK[2] = (2.0f*e) / d;

	CreateResources();
	BuildRootSignature();
	BuildPSO();
}

void WavesGPU::Update(const GameTimer & gt)
{
	static float t = 0.0f;

	// Accumulate time.
	t += gt.DeltaTime();

	mCmdList->SetPipelineState(mPSOs["update"].Get());
	mCmdList->SetComputeRootSignature(mRootSignature.Get());

	// Only update the simulation at the specified time step.
	if (t >= mTimeStep)
	{
		// Set the update constants.
		mCmdList->SetComputeRoot32BitConstants(0, 3, mK, 0);

		mCmdList->SetComputeRootDescriptorTable(1, mPrevUav);
		mCmdList->SetComputeRootDescriptorTable(2, mCurrUav);
		mCmdList->SetComputeRootDescriptorTable(3, mOutputUav);

		// How many groups do we need to dispatch to cover the wave grid.  
		// Note that mNumRows and mNumCols should be divisible by 16
		// so there is no remainder.
		UINT numGroupsX = mNumCols / 16;
		UINT numGroupsY = mNumRows / 16;
		mCmdList->Dispatch(numGroupsX, numGroupsY, 1);

		//
		// Ping-pong buffers in preparation for the next update.
		// The previous solution is no longer needed and becomes the target of the next solution in the next update.
		// The current solution becomes the previous solution.
		// The next solution becomes the current solution.
		//

		auto resTemp = mPrevSolution;
		mPrevSolution = mCurrSolution;
		mCurrSolution = mOutput;
		mOutput = resTemp;

		auto srvTemp = mPrevSrv;
		mPrevSrv = mCurrSrv;
		mCurrSrv = mOutputSrv;
		mOutputSrv = srvTemp;

		auto uavTemp = mPrevUav;
		mPrevUav = mCurrUav;
		mCurrUav = mOutputUav;
		mOutputUav = uavTemp;

		t = 0.0f; // reset time
	}
}

WavesGPU::~WavesGPU()
{
}

CD3DX12_GPU_DESCRIPTOR_HANDLE WavesGPU::DisplacementMap() const
{
	return mCurrSrv;
}

int WavesGPU::RowCount() const
{
	return mNumRows;
}

int WavesGPU::ColumnCount() const
{
	return mNumCols;
}

int WavesGPU::VertexCount() const
{
	return mVertexCount;
}

float WavesGPU::SpatialStep() const
{
	return mSpatialStep;
}

void WavesGPU::CreateResources()
{
	auto byteSize = mVertexCount * sizeof(float);

	std::vector<ID3D12Resource**> resourceAddresses = { &mCurrSolution, &mPrevSolution, &mOutput,
		&mPrevUploadBuffer, &mCurrUploadBuffer };

	UINT uploadOffset = 3;

	auto texDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R32_FLOAT, mNumCols, mNumRows,
		1u, 1u, 1u, 0u, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

	for (UINT i = 0; i < uploadOffset; ++i) {
		ThrowIfFailed(mDevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&texDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			IID_PPV_ARGS(resourceAddresses[i])
		));
	}

	auto numSubresources = texDesc.DepthOrArraySize * texDesc.MipLevels;
	auto bufferSize = GetRequiredIntermediateSize(mCurrSolution.Get(), 0, numSubresources);

	for (UINT i = uploadOffset; i < (UINT)resourceAddresses.size(); ++i) {
		ThrowIfFailed(mDevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(bufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(resourceAddresses[i])
		));
	}

	std::vector<float> initialData;
	initialData.resize(mNumRows*mNumCols);
	for (UINT i = 0; i < mNumRows*mNumCols; ++i) {
		initialData[i] = 0.0f;
	}

	D3D12_SUBRESOURCE_DATA subResources = {};
	subResources.pData = initialData.data();
	subResources.RowPitch = mNumCols * sizeof(float);
	subResources.SlicePitch = mNumRows * subResources.RowPitch;

	mCmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mCurrSolution.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));
	UpdateSubresources(mCmdList.Get(), mCurrSolution.Get(), mCurrUploadBuffer.Get(), 0, 0, numSubresources, &subResources);
	mCmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mCurrSolution.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

	mCmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mPrevSolution.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));
	UpdateSubresources(mCmdList.Get(), mPrevSolution.Get(), mPrevUploadBuffer.Get(), 0, 0, numSubresources, &subResources);
	mCmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mPrevSolution.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

	mCmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mOutput.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
}

void WavesGPU::BuildRootSignature()
{
	const UINT numParams = 4;
	CD3DX12_ROOT_PARAMETER params[numParams];
	params[0].InitAsConstants(6, 0); // 0 -> coefficients

	auto uavTable0 = CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);
	auto uavTable1 = CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 1);
	auto uavTable2 = CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 2);

	params[1].InitAsDescriptorTable(1, &uavTable0); // 1 -> prevSolution
	params[2].InitAsDescriptorTable(1, &uavTable1); // 2 -> currSolution
	params[3].InitAsDescriptorTable(1, &uavTable2); // 3 -> Output

	auto rootSigDesc = CD3DX12_ROOT_SIGNATURE_DESC(numParams, params, 0, nullptr,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> rootSig;
	ComPtr<ID3DBlob> errorBlob;

	auto hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		rootSig.GetAddressOf(), errorBlob.GetAddressOf());

	if (errorBlob != nullptr) {
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}

	ThrowIfFailed(hr);

	ThrowIfFailed(mDevice->CreateRootSignature(0, rootSig->GetBufferPointer(),
		rootSig->GetBufferSize(),
		IID_PPV_ARGS(&mRootSignature)));
}

void WavesGPU::BuildDescriptors(CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle,
	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle,
	UINT descriptorSize)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.PlaneSlice = 0;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = DXGI_FORMAT_R32_FLOAT;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	uavDesc.Texture2D.MipSlice = 0;

	mDevice->CreateShaderResourceView(mPrevSolution.Get(), &srvDesc, cpuHandle);
	mDevice->CreateShaderResourceView(mCurrSolution.Get(), &srvDesc, cpuHandle.Offset(1, descriptorSize));
	mDevice->CreateShaderResourceView(mOutput.Get(), &srvDesc, cpuHandle.Offset(1, descriptorSize));

	mDevice->CreateUnorderedAccessView(mPrevSolution.Get(), nullptr, &uavDesc, cpuHandle.Offset(1, descriptorSize));
	mDevice->CreateUnorderedAccessView(mCurrSolution.Get(), nullptr, &uavDesc, cpuHandle.Offset(1, descriptorSize));
	mDevice->CreateUnorderedAccessView(mOutput.Get(), nullptr, &uavDesc, cpuHandle.Offset(1, descriptorSize));

	mPrevSrv = gpuHandle;
	mCurrSrv = gpuHandle.Offset(1, descriptorSize);
	mOutputSrv = gpuHandle.Offset(1, descriptorSize);
	mPrevUav = gpuHandle.Offset(1, descriptorSize);
	mCurrUav = gpuHandle.Offset(1, descriptorSize);
	mOutputUav = gpuHandle.Offset(1, descriptorSize);
}

void WavesGPU::Disturb(UINT i, UINT j, float magnitude)
{
	UINT disturbIndex[2] = { j, i };
	mCmdList->SetComputeRootSignature(mRootSignature.Get());
	mCmdList->SetPipelineState(mPSOs["disturb"].Get());
	mCmdList->SetComputeRoot32BitConstants(0, 1, &magnitude, 3);
	mCmdList->SetComputeRoot32BitConstants(0, 2, disturbIndex, 4);
	mCmdList->SetComputeRootDescriptorTable(3, mCurrUav);

	mCmdList->Dispatch(1, 1, 1);
}

ComPtr<ID3D12Resource> WavesGPU::CurrentSolution()
{
	return mCurrSolution;
}

void WavesGPU::BuildPSO()
{

	mShaders["update"] = d3dUtil::CompileShader(L"Shaders\\Waves.hlsl", nullptr, "WavesCS", "cs_5_0");
	mShaders["disturb"] = d3dUtil::CompileShader(L"Shaders\\Waves.hlsl", nullptr, "DisturbCS", "cs_5_0");
	D3D12_COMPUTE_PIPELINE_STATE_DESC desc;
	ZeroMemory(&desc, sizeof(D3D12_COMPUTE_PIPELINE_STATE_DESC));

	desc.pRootSignature = mRootSignature.Get();
	desc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	desc.CS = {
		reinterpret_cast<BYTE*>(mShaders["update"]->GetBufferPointer()),
		(UINT)mShaders["update"]->GetBufferSize()
	};

	ThrowIfFailed(mDevice->CreateComputePipelineState(&desc, IID_PPV_ARGS(&mPSOs["update"])));

	desc.CS = {
		reinterpret_cast<BYTE*>(mShaders["disturb"]->GetBufferPointer()),
		(UINT)mShaders["disturb"]->GetBufferSize()
	};

	ThrowIfFailed(mDevice->CreateComputePipelineState(&desc, IID_PPV_ARGS(&mPSOs["disturb"])));
}
