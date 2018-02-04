#include "stdafx.h"
#include "CubeApp.h"

CubeApp::CubeApp(HINSTANCE hInstance) : D3DApp(hInstance)
{
	mCamera = std::make_unique<Camera>();
}

bool CubeApp::Initialize()
{
	if (!D3DApp::Initialize()) {
		return false;
	}

	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	BuildCubeMaps();
	BuildGeometry();
	BuildRenderItems();
	BuildFrameResources();
	BuildDescriptorHeaps();
	BuildInstanceDescriptors();
	BuildDescriptors();
	CompileShadersAndInputLayout();
	BuildRootSignature();
	BuildCommonRootSignature();
	BuildPSOs();

	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* commandLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

	FlushCommandQueue();
	return true;
}

void CubeApp::BuildGeometry()
{
	GeometryGenerator geoGen;
	GeometryGenerator::MeshData sphere = geoGen.CreateSphere(1.0f, 20, 30);

	std::vector<Vertex> vertices;
	vertices.resize(sphere.Vertices.size());
	for (UINT i = 0; i < (UINT)vertices.size(); ++i)
	{
		vertices[i] = { sphere.Vertices[i].Position, sphere.Vertices[i].Normal, sphere.Vertices[i].TexC };
	}

	std::vector<std::uint16_t> indices = sphere.GetIndices16();

	UINT vbByteSize = sizeof(Vertex) * (UINT)vertices.size();
	UINT ibByteSize = sizeof(std::uint16_t) * (UINT)indices.size();

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "skySphere";
	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));

	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(), mCommandList.Get(), vertices.data(),
		vbByteSize, geo->VertexBufferUploader);
	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(), mCommandList.Get(), indices.data(),
		ibByteSize, geo->IndexBufferUploader);

	SubmeshGeometry submesh = {};
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0u;
	submesh.BaseVertexLocation = 0u;
	geo->DrawArgs["skySphere"] = submesh;

	geo->VertexByteStride = sizeof(Vertex);
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexBufferByteSize = ibByteSize;

	mGeometries[geo->Name] = std::move(geo);
}

void CubeApp::BuildRenderItems()
{
	auto rItem = std::make_unique<RenderItem>();
	rItem->Geo = mGeometries["skySphere"].get();
	rItem->PrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	rItem->IndexCount = rItem->Geo->DrawArgs["skySphere"].IndexCount;
	rItem->StartIndexLocation = rItem->Geo->DrawArgs["skySphere"].StartIndexLocation;
	rItem->InstanceCount = 1;
	rItem->BaseVertexLocation = rItem->Geo->DrawArgs["skySphere"].BaseVertexLocation;
	InstanceData data;
	rItem->Instances.emplace_back(std::move(data));
	rItem->IgnoreBoundingBox = true;
	mRenderLayer[RenderLayers::Skies].emplace_back(rItem.get());
	mAllRenderItems.emplace_back(std::move(rItem));

	for (auto& rItem : mAllRenderItems)
		mMaxInstanceCount += (UINT)rItem->Instances.size();
}

void CubeApp::BuildDescriptorHeaps()
{
	auto numRenderItems = mAllRenderItems.size();
	mPassCbOffset = (int)(numRenderItems * mNumFrameResources);
	auto numDescriptors = mPassCbOffset + mNumFrameResources + mTextures.size();
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heapDesc.NumDescriptors = (UINT)numDescriptors;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heapDesc.NodeMask = 0u;

	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&mCbvSrvHeap)));
}

void CubeApp::BuildDescriptors()
{
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	UINT passCbByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(PassConstants));
	cbvDesc.SizeInBytes = passCbByteSize;
	for (UINT i = 0; i < (UINT)mNumFrameResources; ++i) {
		auto passResource = mFrameResources[i]->PassCB->Resource();
		cbvDesc.BufferLocation = passResource->GetGPUVirtualAddress();
		auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mCbvSrvHeap->GetCPUDescriptorHandleForHeapStart(), mPassCbOffset + i, mCbvSrvDescriptorSize);
		md3dDevice->CreateConstantBufferView(&cbvDesc, handle);
	}

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
	srvDesc.TextureCube.MostDetailedMip = 0;
	srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
	mCubeMapSrvOffset = mPassCbOffset + 3;
	auto dHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mCbvSrvHeap->GetCPUDescriptorHandleForHeapStart(), mCubeMapSrvOffset, mCbvSrvDescriptorSize);
	for (UINT i = 0; i < (UINT)mTextureNames.size(); ++i) {
		auto resource = mTextures[mTextureNames[i]]->Resource.Get();
		srvDesc.Format = resource->GetDesc().Format;
		srvDesc.TextureCube.MipLevels = resource->GetDesc().MipLevels;
		md3dDevice->CreateShaderResourceView(resource, &srvDesc, dHandle);
		dHandle = dHandle.Offset(1, mCbvSrvDescriptorSize);
	}
}

void CubeApp::BuildInstanceDescriptors()
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};

	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.NumElements = mMaxInstanceCount;
	srvDesc.Buffer.StructureByteStride = sizeof(InstanceData);
	srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

	for (UINT i = 0; i < mNumFrameResources; ++i) {
		auto res = mFrameResources[i]->InstanceCB->Resource();
		md3dDevice->CreateShaderResourceView(res, &srvDesc,
			CD3DX12_CPU_DESCRIPTOR_HANDLE(mCbvSrvHeap->GetCPUDescriptorHandleForHeapStart(), i, mCbvSrvDescriptorSize));
	}
}

void CubeApp::BuildMaterials()
{
	auto mat = std::make_unique<Material>(mNumFrameResources);
	mat->MatCBIndex = 0;
	mat->DiffuseAlbedo = { 0.2f, 0.8f, 0.2f, 1.0f };
	mat->Name = "grass";
	mMaterials[mat->Name] = std::move(mat);
}

void CubeApp::CompileShadersAndInputLayout()
{
	mShaders["BasicRenderVS"] = d3dUtil::CompileShader(L"Shaders\\BasicRender.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["BasicRenderPS"] = d3dUtil::CompileShader(L"Shaders\\BasicRender.hlsl", nullptr, "PS", "ps_5_1");
	mShaders["SkyCubeVS"] = d3dUtil::CompileShader(L"Shaders\\Sky.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["SkyCubePS"] = d3dUtil::CompileShader(L"Shaders\\Sky.hlsl", nullptr, "PS", "ps_5_1");
	mInputLayout[(UINT)InputLayoutType::Basic] = {
		{ "POSITION", 0u, DXGI_FORMAT_R32G32B32_FLOAT, 0u, 0u, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0u }
	};

	mInputLayout[(UINT)InputLayoutType::Sky] = {
		{ "POSITION", 0u, DXGI_FORMAT_R32G32B32_FLOAT, 0u, 0u, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0u },
		{ "NORMAL", 0u, DXGI_FORMAT_R32G32B32_FLOAT, 0u, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0u },
		{ "TEXCOORD", 0u, DXGI_FORMAT_R32G32_FLOAT, 0u, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0u }
	};
}

void CubeApp::BuildRootSignature()
{
	const int numRootParams = 2;
	auto srvTable0 = CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
	auto cbvTable1 = CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);

	CD3DX12_ROOT_PARAMETER rootParams[numRootParams];
	rootParams[0].InitAsDescriptorTable(1, &srvTable0);
	rootParams[1].InitAsDescriptorTable(1, &cbvTable1);

	auto rootSigDesc = CD3DX12_ROOT_SIGNATURE_DESC(numRootParams, rootParams,
		0u, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> sig;
	ComPtr<ID3DBlob> err;

	auto handle = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &sig, &err);

	if (err != nullptr) {
		::OutputDebugStringA((char*)err->GetBufferPointer());
	}

	ThrowIfFailed(handle);

	ThrowIfFailed(md3dDevice->CreateRootSignature(0u, sig->GetBufferPointer(),
		sig->GetBufferSize(), IID_PPV_ARGS(&mRootSignature)));
}

void CubeApp::BuildCommonRootSignature()
{
	const int numRootParams = 5;
	mPassCbvRootParamIndex = 0;
	CD3DX12_DESCRIPTOR_RANGE cbvTable0(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);		// Pass Constants
	mCubeMapSrvRootParamIndex = 1;
	CD3DX12_DESCRIPTOR_RANGE srvTable00(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);	// Cube Map
	CD3DX12_DESCRIPTOR_RANGE srvTable01(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 1);	// Material Data
	mInstanceDataRootParamIndex = 3;
	CD3DX12_DESCRIPTOR_RANGE srvTable1(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1);		// Instance Data
	CD3DX12_DESCRIPTOR_RANGE srvTable2(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2);		// Textures


	CD3DX12_ROOT_PARAMETER rootParams[numRootParams];
	rootParams[mPassCbvRootParamIndex].InitAsDescriptorTable(1, &cbvTable0);
	rootParams[mCubeMapSrvRootParamIndex].InitAsDescriptorTable(1, &srvTable00);
	rootParams[2].InitAsDescriptorTable(1, &srvTable01);
	rootParams[mInstanceDataRootParamIndex].InitAsDescriptorTable(1, &srvTable1);
	rootParams[4].InitAsDescriptorTable(1, &srvTable2);

	auto samplers = GetStaticSamplers();

	auto rootSigDesc = CD3DX12_ROOT_SIGNATURE_DESC(numRootParams, rootParams, (UINT)samplers.size(), samplers.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> rootSig;
	ComPtr<ID3DBlob> error;
	
	auto handle = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &rootSig, &error);

	if (error != nullptr)
		::OutputDebugStringA((char*)error->GetBufferPointer());

	ThrowIfFailed(handle);

	ThrowIfFailed(md3dDevice->CreateRootSignature(0u, rootSig->GetBufferPointer(), rootSig->GetBufferSize(),
		IID_PPV_ARGS(&mCommonRootSignature)));

}

void CubeApp::BuildPSOs()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC desc;
	ZeroMemory(&desc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));

	desc.pRootSignature = mRootSignature.Get();
	desc.VS = { reinterpret_cast<BYTE*>(mShaders["BasicRenderVS"]->GetBufferPointer()),
		(UINT)mShaders["BasicRenderVS"]->GetBufferSize() };
	desc.PS = { reinterpret_cast<BYTE*>(mShaders["BasicRenderPS"]->GetBufferPointer()),
		(UINT)mShaders["BasicRenderPS"]->GetBufferSize() };

	desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	desc.SampleMask = UINT_MAX;
	auto rState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	rState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	desc.RasterizerState = rState;
	desc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	desc.InputLayout = { mInputLayout[(UINT)InputLayoutType::Basic].data(),
		(UINT)mInputLayout[(UINT)InputLayoutType::Basic].size()
	};
	desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	desc.NumRenderTargets = 1;
	desc.RTVFormats[0] = mBackBufferFormat;
	desc.DSVFormat = mDepthStencilFormat;
	desc.SampleDesc.Quality = m4xMsaaState ? m4xMsaaQuality - 1 : 0;
	desc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	desc.NodeMask = 0u;
	desc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&mPSOs["wireframe"])));

	auto opaqueDesc = desc;
	opaqueDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	opaqueDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	D3D12_RENDER_TARGET_BLEND_DESC transparencyBlendDesc;
	transparencyBlendDesc.BlendEnable = true;
	transparencyBlendDesc.LogicOpEnable = false;
	transparencyBlendDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
	transparencyBlendDesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	transparencyBlendDesc.BlendOp = D3D12_BLEND_OP_ADD;
	transparencyBlendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
	transparencyBlendDesc.DestBlendAlpha = D3D12_BLEND_ZERO;
	transparencyBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
	transparencyBlendDesc.LogicOp = D3D12_LOGIC_OP_NOOP;
	transparencyBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	opaqueDesc.BlendState.RenderTarget[0] = transparencyBlendDesc;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&opaqueDesc, IID_PPV_ARGS(&mPSOs["opaque"])));

	auto skyDesc = desc;
	skyDesc.pRootSignature = mCommonRootSignature.Get();
	skyDesc.VS = { reinterpret_cast<BYTE*>(mShaders["SkyCubeVS"]->GetBufferPointer()),
		(UINT)mShaders["SkyCubeVS"]->GetBufferSize() };
	skyDesc.PS = { reinterpret_cast<BYTE*>(mShaders["SkyCubePS"]->GetBufferPointer()),
		(UINT)mShaders["SkyCubePS"]->GetBufferSize() };
	skyDesc.InputLayout = { mInputLayout[(UINT)InputLayoutType::Sky].data(),
		(UINT)mInputLayout[(UINT)InputLayoutType::Sky].size()
	};

	skyDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	skyDesc.RasterizerState.CullMode = D3D12_CULL_MODE_FRONT;
	skyDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;

	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&skyDesc, IID_PPV_ARGS(&mPSOs["sky"])));
}

void CubeApp::BuildFrameResources()
{
	for (UINT i = 0; i < (UINT)mNumFrameResources; ++i) {
		mFrameResources.emplace_back(std::make_unique<FrameResource>(
			md3dDevice.Get(), 1, mMaxInstanceCount, (UINT)mMaterials.size()));
	}
}

void CubeApp::BuildCubeMaps()
{
	struct ca {
		std::wstring fileName;
		std::string name;
	};
	std::vector<ca> cubeApps = {
		{ L"Textures/grasscube1024.dds", "grassSky" },
		{ L"Textures/desertcube1024.dds", "desertSky" },
		{ L"Textures/snowcube1024.dds", "snowSky" },
		{ L"Textures/sunsetcube1024.dds", "sunsetSky"}
	};

	for (auto& cApp : cubeApps)
	{
		auto tex = std::make_unique<Texture>();
		tex->Name = cApp.name;
		tex->Filename = cApp.fileName;
		ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(), mCommandList.Get(),
			tex->Filename.c_str(), tex->Resource, tex->UploadHeap));
		mTextureNames.emplace_back(std::move(cApp.name));
		mTextures[tex->Name] = std::move(tex);
		++mSkyCount;
	}
}

std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> CubeApp::GetStaticSamplers() const
{
	const CD3DX12_STATIC_SAMPLER_DESC pointWrap(
		0,
		D3D12_FILTER_MIN_MAG_MIP_POINT,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP
	);

	const CD3DX12_STATIC_SAMPLER_DESC pointClamp(
		1,
		D3D12_FILTER_MIN_MAG_MIP_POINT,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP
	);

	const CD3DX12_STATIC_SAMPLER_DESC linearWrap(
		2,
		D3D12_FILTER_MIN_MAG_MIP_LINEAR,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP
	);

	const CD3DX12_STATIC_SAMPLER_DESC linearClamp(
		3,
		D3D12_FILTER_MIN_MAG_MIP_LINEAR,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP
	);

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicWrap(
		4,
		D3D12_FILTER_ANISOTROPIC,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		0.0f,
		8
	);



	const CD3DX12_STATIC_SAMPLER_DESC anisotropicClamp(
		5,
		D3D12_FILTER_ANISOTROPIC,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		0.0f,
		8
	);

	return { pointWrap, pointClamp, linearWrap, linearClamp, anisotropicWrap, anisotropicClamp };
}


void CubeApp::Update(const GameTimer& gt)
{
	OnKeyboardInput(gt);
	mCurrFrameIndex = (mCurrFrameIndex + 1) % mNumFrameResources;
	mCurrentFrameResource = mFrameResources[mCurrFrameIndex].get();

	if (mCurrentFrameResource->Fence != 0 && mFence->GetCompletedValue() < mCurrentFrameResource->Fence) {
		auto eventHandle = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
		mFence->SetEventOnCompletion(mCurrentFrameResource->Fence, eventHandle);
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}

	if (mCamera->UpdateViewMatrix()) {
		MarkRenderItemsDirty();
	};

	UpdateInstances(gt);
	UpdateMainPassCB(gt);
}

void CubeApp::Draw(const GameTimer & gt)
{
	auto cmdAlloc = mCurrentFrameResource->CmdListAlloc;
	ThrowIfFailed(cmdAlloc->Reset());
	ThrowIfFailed(mCommandList->Reset(cmdAlloc.Get(), mPSOs["wireframe"].Get()));

	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(), D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET));

	mCommandList->ClearRenderTargetView(CurrentBackBufferView(), DirectX::Colors::LightSteelBlue, 0, nullptr);
	mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
	mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

	mCommandList->SetGraphicsRootSignature(mCommonRootSignature.Get());
	ID3D12DescriptorHeap* descriptorHeaps[1] = { mCbvSrvHeap.Get() };

	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	mCommandList->SetGraphicsRootDescriptorTable(mPassCbvRootParamIndex,
		CD3DX12_GPU_DESCRIPTOR_HANDLE(mCbvSrvHeap->GetGPUDescriptorHandleForHeapStart(),
			mPassCbOffset + mCurrFrameIndex, mCbvSrvDescriptorSize));

	mCommandList->SetGraphicsRootDescriptorTable(mCubeMapSrvRootParamIndex,
		CD3DX12_GPU_DESCRIPTOR_HANDLE(mCbvSrvHeap->GetGPUDescriptorHandleForHeapStart(),
			mCubeMapSrvOffset + (UINT)mActiveSky, mCbvSrvDescriptorSize));

	mCommandList->SetPipelineState(mPSOs["sky"].Get());
	DrawRenderItems(mRenderLayer[(UINT)RenderLayers::Skies]);

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT));

	ThrowIfFailed(mCommandList->Close());

	ID3D12CommandList* commandLists[1] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

	ThrowIfFailed(mSwapChain->Present(0, 0));

	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

	mCurrentFrameResource->Fence = ++mCurrentFence;
	mCommandQueue->Signal(mFence.Get(), mCurrentFence);
}

void CubeApp::DrawRenderItems(const std::vector<RenderItem*>& renderItems)
{
	mCommandList->SetGraphicsRootDescriptorTable(mInstanceDataRootParamIndex,
		CD3DX12_GPU_DESCRIPTOR_HANDLE(mCbvSrvHeap->GetGPUDescriptorHandleForHeapStart(),
			mCurrFrameIndex, mCbvSrvDescriptorSize));

	for (UINT i = 0; i < (UINT)renderItems.size(); ++i) {
		auto rItem = renderItems[i];
		mCommandList->IASetVertexBuffers(0, 1, &rItem->Geo->VertexBufferView());
		mCommandList->IASetIndexBuffer(&rItem->Geo->IndexBufferView());
		mCommandList->IASetPrimitiveTopology(rItem->PrimitiveTopology);
		mCommandList->DrawIndexedInstanced(rItem->IndexCount, rItem->InstanceCount, rItem->StartIndexLocation, rItem->BaseVertexLocation, 0);
	}
}

void CubeApp::UpdateInstances(const GameTimer & gt)
{
	XMMATRIX view = mCamera->View();
	XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
	BoundingFrustum& cameraFrustum = mCamera->GetBoundingFrustum();
	BoundingFrustum localFrustum;
	for (auto& renderItem : mAllRenderItems) {

		if (renderItem->NumFramesDirty > 0) {

			int visibleInstanceCount = 0;
			for (UINT i = 0; i < (UINT)renderItem->Instances.size(); ++i) {

				XMMATRIX world = XMLoadFloat4x4(&renderItem->Instances[i].World);
				XMMATRIX invWorld = XMMatrixInverse(&XMMatrixDeterminant(world), world);

				XMMATRIX invViewWorld = XMMatrixMultiply(invView, invWorld);
				cameraFrustum.Transform(localFrustum, invViewWorld);

				if (renderItem->IgnoreBoundingBox || localFrustum.Contains(renderItem->Bounds) != DirectX::DISJOINT) {
					InstanceData data = {};
					XMStoreFloat4x4(&data.World, XMMatrixTranspose(world));

					mCurrentFrameResource->InstanceCB->CopyData(visibleInstanceCount++, data);
				}
			}
			renderItem->InstanceCount = visibleInstanceCount;
			renderItem->NumFramesDirty--;

			std::wostringstream outs;
			outs << L"Instancing and Culling Demo: " << renderItem->InstanceCount <<
				L" objects visible out of " << renderItem->Instances.size();
			mMainWndCaption = outs.str();
		}
	}
}

void CubeApp::UpdateMainPassCB(const GameTimer & gt)
{
	XMMATRIX view = mCamera->View();
	XMMATRIX proj = mCamera->Proj();
	XMMATRIX viewProj = XMMatrixMultiply(view, proj);
	XMStoreFloat4x4(&mMainPassCB.View, XMMatrixTranspose(view));
	XMStoreFloat4x4(&mMainPassCB.Proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&mMainPassCB.InvView, XMMatrixTranspose(XMMatrixInverse(&XMMatrixDeterminant(view), view)));
	XMStoreFloat4x4(&mMainPassCB.InvProj, XMMatrixTranspose(XMMatrixInverse(&XMMatrixDeterminant(proj), proj)));
	XMStoreFloat4x4(&mMainPassCB.ViewProj, XMMatrixTranspose(viewProj));
	XMStoreFloat4x4(&mMainPassCB.InvViewProj, XMMatrixTranspose(XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj)));
	XMStoreFloat3(&mMainPassCB.EyePos, mCamera->GetPosition());
	mMainPassCB.RenderTargetSize = { (float)mClientWidth, (float)mClientHeight };
	mMainPassCB.InvRenderTargetSize = { 1.0f / mClientWidth, 1.0f / mClientHeight };
	mMainPassCB.NearZ = 1.0f;
	mMainPassCB.FarZ = 1000.0f;
	mMainPassCB.TotalTime = gt.TotalTime();
	mMainPassCB.DeltaTime = gt.DeltaTime();
	mMainPassCB.AmbientLight = { 1.0f, 1.0f, 1.0f, 1.0f };

	mCurrentFrameResource->PassCB->CopyData(0, mMainPassCB);
}

void CubeApp::MarkRenderItemsDirty()
{
	for (auto& rItem : mAllRenderItems) {
		rItem->NumFramesDirty++;
	}
}

void CubeApp::OnMouseDown(WPARAM btnState, int x, int y)
{
	SetCapture(mhMainWnd);
	mLastMousePos.x = x;
	mLastMousePos.y = y;
}

void CubeApp::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void CubeApp::OnMouseMove(WPARAM btnState, int x, int y)
{
	if ((btnState & MK_LBUTTON) != 0) {
		float dx = XMConvertToRadians(0.25f*static_cast<float>(x - mLastMousePos.x));
		float dy = XMConvertToRadians(0.25f*static_cast<float>(y - mLastMousePos.y));

		mCamera->Rotate(dx);
		mCamera->Pitch(dy);
	}

	mLastMousePos.x = x;
	mLastMousePos.y = y;
}

void CubeApp::OnResize()
{
	D3DApp::OnResize();
	mCamera->SetLens(0.25f*MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
}

void CubeApp::OnKeyboardInput(const GameTimer & gt)
{
	static float debounceTime = 0.1f;
	static float debounce = 0.0f;
	debounce += gt.DeltaTime();
	if (GetAsyncKeyState(VK_UP) & 0x8000) {
		mCamera->Walk(10.0f*gt.DeltaTime());
	}

	if (GetAsyncKeyState(VK_DOWN) & 0x8000) {
		mCamera->Walk(-10.0f*gt.DeltaTime());
	}

	if (GetAsyncKeyState(VK_LEFT) & 0x8000) {
		mCamera->Rotate(-1.0f*gt.DeltaTime());
	}

	if (GetAsyncKeyState(VK_RIGHT) & 0x8000) {
		mCamera->Rotate(1.0f*gt.DeltaTime());
	}

	if (debounce > debounceTime) {
		if (GetAsyncKeyState('G') & 0x8000) {
			mActiveSky = SkyType::Grass;
			debounce = 0.0f;
		}
		else if (GetAsyncKeyState('D') & 0x8000) {
			mActiveSky = SkyType::Desert;
			debounce = 0.0f;
		}
		else if (GetAsyncKeyState('S') & 0x8000) {
			mActiveSky = SkyType::Snow;
			debounce = 0.0f;
		}
		else if (GetAsyncKeyState('U') & 0x8000) {
			mActiveSky = SkyType::Sunset;
			debounce = 0.0f;
		}
	}
}
