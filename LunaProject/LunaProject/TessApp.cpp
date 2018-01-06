#include "stdafx.h"
#include "TessApp.h"

TessApp::TessApp(HINSTANCE hInstance) : D3DApp(hInstance) {
}

void TessApp::BuildRootSignature()
{
	const int numRootParams = 2;
	CD3DX12_ROOT_PARAMETER params[numRootParams];
	CD3DX12_DESCRIPTOR_RANGE cbvTable0(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0); // per object settings
	CD3DX12_DESCRIPTOR_RANGE cbvTable1(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1); // pass settings

	params[0].InitAsDescriptorTable(1, &cbvTable0);
	params[1].InitAsDescriptorTable(1, &cbvTable1);

	auto rootSigDesc = CD3DX12_ROOT_SIGNATURE_DESC(numRootParams, params, 0, nullptr,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> rootSig;
	ComPtr<ID3DBlob> error;

	auto hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &rootSig, &error);

	if (error != nullptr) {
		::OutputDebugStringA((char*)error->GetBufferPointer());
	}

	ThrowIfFailed(hr);

	ThrowIfFailed(md3dDevice->CreateRootSignature(0, rootSig->GetBufferPointer(), rootSig->GetBufferSize(),
		IID_PPV_ARGS(&mRootSignature)));
}

void TessApp::BuildShadersAndInputLayout()
{
	mShaders["PassThroughVS"] = d3dUtil::CompileShader(L"Shaders\\PassThrough.hlsl", nullptr, "VS", "vs_5_0");
	mShaders["PassThroughPS"] = d3dUtil::CompileShader(L"Shaders\\PassThrough.hlsl", nullptr, "PS", "ps_5_0");
	mShaders["PatchTessVS"] = d3dUtil::CompileShader(L"Shaders\\PatchTess.hlsl", nullptr, "VS", "vs_5_0");
	mShaders["PatchTessPS"] = d3dUtil::CompileShader(L"Shaders\\PatchTess.hlsl", nullptr, "PS", "ps_5_0");
	mShaders["PatchTessHS"] = d3dUtil::CompileShader(L"Shaders\\PatchTess.hlsl", nullptr, "HS", "hs_5_0");
	mShaders["PatchTessDS"] = d3dUtil::CompileShader(L"Shaders\\PatchTess.hlsl", nullptr, "DS", "ds_5_0");
	mShaders["BezierTessVS"] = d3dUtil::CompileShader(L"Shaders\\Bezier.hlsl", nullptr, "VS", "vs_5_0");
	mShaders["BezierTessHS"] = d3dUtil::CompileShader(L"Shaders\\Bezier.hlsl", nullptr, "HS", "hs_5_0");
	mShaders["BezierTessDS"] = d3dUtil::CompileShader(L"Shaders\\Bezier.hlsl", nullptr, "DS", "ds_5_0");
	mShaders["BezierTessPS"] = d3dUtil::CompileShader(L"Shaders\\Bezier.hlsl", nullptr, "PS", "ps_5_0");
	mInputLayout = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
}

void TessApp::BuildPSOs()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaqueDesc;
	ZeroMemory(&opaqueDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));

	opaqueDesc.VS = {
		reinterpret_cast<BYTE*>(mShaders["PassThroughVS"]->GetBufferPointer()),
		(UINT)mShaders["PassThroughVS"]->GetBufferSize()
	};

	opaqueDesc.PS = {
		reinterpret_cast<BYTE*>(mShaders["PassThroughPS"]->GetBufferPointer()),
		(UINT)mShaders["PassThroughPS"]->GetBufferSize()
	};

	opaqueDesc.pRootSignature = mRootSignature.Get();
	opaqueDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	opaqueDesc.SampleMask = UINT_MAX;
	auto rState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	rState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	opaqueDesc.RasterizerState = rState;
	opaqueDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
	opaqueDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	opaqueDesc.NumRenderTargets = 1;
	opaqueDesc.RTVFormats[0] = mBackBufferFormat;
	opaqueDesc.DSVFormat = mDepthStencilFormat;
	opaqueDesc.SampleDesc.Quality = m4xMsaaState ? m4xMsaaQuality - 1 : 0;
	opaqueDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	opaqueDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&opaqueDesc, IID_PPV_ARGS(&mPSOs["PassThrough"])));

	auto patchDesc = opaqueDesc;
	patchDesc.VS = {
		reinterpret_cast<BYTE*>(mShaders["PatchTessVS"]->GetBufferPointer()),
		(UINT)mShaders["PatchTessVS"]->GetBufferSize()
	};
	patchDesc.HS = {
		reinterpret_cast<BYTE*>(mShaders["PatchTessHS"]->GetBufferPointer()),
		(UINT)mShaders["PatchTessHS"]->GetBufferSize()
	};
	patchDesc.DS = {
		reinterpret_cast<BYTE*>(mShaders["PatchTessDS"]->GetBufferPointer()),
		(UINT)mShaders["PatchTessDS"]->GetBufferSize()
	};
	patchDesc.PS = {
		reinterpret_cast<BYTE*>(mShaders["PatchTessPS"]->GetBufferPointer()),
		(UINT)mShaders["PatchTessPS"]->GetBufferSize()
	};

	patchDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&patchDesc, IID_PPV_ARGS(&mPSOs["PatchTess"])));

	auto bezierDesc = opaqueDesc;
	bezierDesc.VS = {
		reinterpret_cast<BYTE*>(mShaders["BezierTessVS"]->GetBufferPointer()),
		(UINT)mShaders["BezierTessVS"]->GetBufferSize()
	};
	bezierDesc.HS = {
		reinterpret_cast<BYTE*>(mShaders["BezierTessHS"]->GetBufferPointer()),
		(UINT)mShaders["BezierTessHS"]->GetBufferSize()
	};
	bezierDesc.DS = {
		reinterpret_cast<BYTE*>(mShaders["BezierTessDS"]->GetBufferPointer()),
		(UINT)mShaders["BezierTessDS"]->GetBufferSize()
	};
	bezierDesc.PS = {
		reinterpret_cast<BYTE*>(mShaders["BezierTessPS"]->GetBufferPointer()),
		(UINT)mShaders["BezierTessPS"]->GetBufferSize()
	};

	bezierDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&bezierDesc, IID_PPV_ARGS(&mPSOs["BezierTess"])));
}

void TessApp::BuildFrameResources()
{
	for (UINT i = 0; i < (UINT)mNumFrameResources; ++i) {
		mFrameResources.emplace_back(std::make_unique<TessFrameResource>(
			md3dDevice.Get(), 1, mRenderItems.size()));
	}
}

void TessApp::BuildDescriptorHeaps()
{
	auto numObjects = mNumFrameResources * (mRenderItems.size() + 1);
	mMainPassCbOffset = (int)mRenderItems.size() * mNumFrameResources;
	D3D12_DESCRIPTOR_HEAP_DESC dHeapDesc = {};
	dHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	dHeapDesc.NumDescriptors = (UINT)numObjects;
	dHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	dHeapDesc.NodeMask = 0u;

	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&dHeapDesc, IID_PPV_ARGS(&mCbvHeap)));
}

void TessApp::BuildDescriptors()
{
	auto objCbByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
	auto passCbByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(PassConstantsThrough));
	UINT numRenderItems = (UINT)mRenderItems.size();

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbDesc = {};
	for (UINT i = 0; i < (UINT)mNumFrameResources; ++i) {
		auto objectCB = mFrameResources[i]->ObjectCB->Resource();
		for (UINT j = 0; j < numRenderItems; ++j) {

			auto cbAddress = objectCB->GetGPUVirtualAddress();
			cbAddress += j * objCbByteSize;
			cbDesc.SizeInBytes = objCbByteSize;
			cbDesc.BufferLocation = cbAddress;
			auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mCbvHeap->GetCPUDescriptorHandleForHeapStart(),
				i*numRenderItems + j,
				mCbvSrvDescriptorSize);

			md3dDevice->CreateConstantBufferView(&cbDesc, handle);
		}
	}

	for (UINT i = 0; i < (UINT)mNumFrameResources; ++i) {
		auto passCB = mFrameResources[i]->PassCB->Resource();
		auto cbAddress = passCB->GetGPUVirtualAddress();
		cbDesc.SizeInBytes = passCbByteSize;
		cbDesc.BufferLocation = cbAddress;
		auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mCbvHeap->GetCPUDescriptorHandleForHeapStart(),
			mMainPassCbOffset + i, mCbvSrvDescriptorSize);

		md3dDevice->CreateConstantBufferView(&cbDesc, handle);
	}
}

void TessApp::BuildGridGeometry() {

	std::vector<Vertex> vertices = {
		{ { -10.0f, 0.0f, +10.0f } },
		{ { +10.0f, 0.0f, +10.0f } },
		{ { -10.0f, 0.0f, -10.0f } },
		{ { +10.0f, 0.0f, -10.0f } }
	};

	std::vector<uint16_t> indices = { 0, 1, 2, 3 };

	std::vector<Vertex> bezierVertices =
	{
		// Row 0
		{ { -10.0f, -10.0f, +15.0f } },
		{ { -5.0f,  0.0f, +15.0f } },
		{ { +5.0f,  0.0f, +15.0f } },
		{ { +10.0f, 0.0f, +15.0f } },

		// Row 1
		{ { -15.0f, 0.0f, +5.0f } },
		{ { -5.0f,  0.0f, +5.0f } },
		{ { +5.0f,  20.0f, +5.0f } },
		{ { +15.0f, 0.0f, +5.0f } } ,

		// Row 2
		{ { -15.0f, 0.0f, -5.0f } },
		{ { -5.0f,  0.0f, -5.0f } },
		{ { +5.0f,  0.0f, -5.0f } },
		{ { +15.0f, 0.0f, -5.0f } },

		// Row 3
		{ { -10.0f, 10.0f, -15.0f } },
		{ { -5.0f,  0.0f, -15.0f } },
		{ { +5.0f,  0.0f, -15.0f } },
		{ { +25.0f, 10.0f, -15.0f } }
	};

	std::vector<std::uint16_t> bezierIndices =
	{
		0, 1, 2, 3,
		4, 5, 6, 7,
		8, 9, 10, 11,
		12, 13, 14, 15
	};

	UINT vertexSz = (UINT)vertices.size() + (UINT)bezierVertices.size();
	auto vbByteSize = vertexSz * sizeof(Vertex);

	std::vector<Vertex> allVertices;
	allVertices.resize(vertexSz);

	auto copyVertices = [&](const std::vector<Vertex>& toCopy, UINT startIndex) {
		for (UINT i = startIndex; (i - startIndex) < (UINT)toCopy.size(); ++i) {
			allVertices[i] = toCopy[i - startIndex];
		}
	};
	copyVertices(vertices, 0u);
	copyVertices(bezierVertices, (UINT)vertices.size());


	UINT szIndices = (UINT)indices.size() + (UINT)bezierIndices.size();
	auto ibByteSize = szIndices * sizeof(std::uint16_t);

	std::vector<std::int16_t> allIndices;
	allIndices.resize(szIndices);

	auto copyIndices = [&](const std::vector<std::uint16_t>& toCopy, UINT startIndex) {
		for (UINT i = startIndex; (i - startIndex) < (UINT)toCopy.size(); ++i) {
			allIndices[i] = toCopy[i - startIndex];
		}
	};
	copyIndices(indices, 0u);
	copyIndices(bezierIndices, (UINT)indices.size());

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "grid";
	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = (UINT)vbByteSize;
	geo->IndexBufferByteSize = (UINT)ibByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));

	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), allVertices.data(), vbByteSize);
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), allIndices.data(), ibByteSize);

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), allVertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), allIndices.data(), ibByteSize, geo->IndexBufferUploader);

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	geo->DrawArgs["grid"] = submesh;

	submesh.IndexCount = (UINT)bezierIndices.size();
	submesh.StartIndexLocation = (UINT)indices.size();
	submesh.BaseVertexLocation = (UINT)vertices.size();

	geo->DrawArgs["bezier"] = submesh;

	mGeometries[geo->Name] = std::move(geo);
}

void TessApp::BuildRenderItems()
{
	auto renderItem = std::make_unique<RenderItem>();
	renderItem->ObjCBIndex = 0;
	renderItem->World = MathHelper::Identity4x4();
	renderItem->Geo = mGeometries["grid"].get();
	renderItem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST;
	renderItem->IndexCount = renderItem->Geo->DrawArgs["grid"].IndexCount;
	renderItem->StartIndexLocation = renderItem->Geo->DrawArgs["grid"].StartIndexLocation;
	renderItem->BaseVertexLocation = renderItem->Geo->DrawArgs["grid"].BaseVertexLocation;

	mRenderLayers[(UINT)RenderLayer::Opaque].emplace_back(renderItem.get());
	mRenderItems.emplace_back(std::move(renderItem));

	renderItem = std::make_unique<RenderItem>();
	renderItem->ObjCBIndex = 1;
	renderItem->World = MathHelper::Identity4x4();
	renderItem->Geo = mGeometries["grid"].get();
	renderItem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_16_CONTROL_POINT_PATCHLIST;
	renderItem->IndexCount = renderItem->Geo->DrawArgs["bezier"].IndexCount;
	renderItem->StartIndexLocation = renderItem->Geo->DrawArgs["bezier"].StartIndexLocation;
	renderItem->BaseVertexLocation = renderItem->Geo->DrawArgs["bezier"].BaseVertexLocation;

	mRenderLayers[(UINT)RenderLayer::Bezier].emplace_back(renderItem.get());
	mRenderItems.emplace_back(std::move(renderItem));
}

void TessApp::UpdateCB(const GameTimer & gt)
{
	for (auto& rItem : mRenderItems) {
		if (rItem->NumFramesDirty != 0) {
			ObjectConstants ocb;
			XMMATRIX world = XMLoadFloat4x4(&rItem->World);
			XMStoreFloat4x4(&ocb.World, world);
			mCurrentFrameResource->ObjectCB->CopyData(rItem->ObjCBIndex, ocb);
			rItem->NumFramesDirty--;
		}
	}
}

void TessApp::UpdatePassCB(const GameTimer & gt)
{
	XMMATRIX view = XMLoadFloat4x4(&mView);
	XMMATRIX proj = XMLoadFloat4x4(&mProj);
	XMStoreFloat4x4(&mPassConstants.View, XMMatrixTranspose(view));
	XMStoreFloat4x4(&mPassConstants.Proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&mPassConstants.ViewProj, XMMatrixTranspose(XMMatrixMultiply(view, proj)));
	mPassConstants.EyePos = mEyePos;
	mCurrentFrameResource->PassCB->CopyData(0, mPassConstants);
}

bool TessApp::Initialize()
{
	if (!D3DApp::Initialize()) {
		return false;
	}

	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	BuildRootSignature();
	BuildShadersAndInputLayout();
	BuildGridGeometry();
	BuildRenderItems();
	BuildFrameResources();
	BuildDescriptorHeaps();
	BuildDescriptors();
	BuildPSOs();


	ThrowIfFailed(mCommandList->Close());

	ID3D12CommandList* cmdLists[1] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);

	FlushCommandQueue();

	return true;
}

void TessApp::Update(const GameTimer & gt)
{

	UpdateCamera(gt);
	HandleKeyboardInput(gt);

	mCurrentFrameResourceIndex = (mCurrentFrameResourceIndex + 1) % mNumFrameResources;
	mCurrentFrameResource = mFrameResources[mCurrentFrameResourceIndex].get();

	if (mCurrentFrameResource->Fence != 0 && mFence->GetCompletedValue() < mCurrentFrameResource->Fence) {
		auto continueExec = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
		ThrowIfFailed(mFence->SetEventOnCompletion(mCurrentFrameResource->Fence, continueExec));
		WaitForSingleObject(continueExec, INFINITE);
		CloseHandle(continueExec);
	}

	UpdatePassCB(gt);
	UpdateCB(gt);
}

void TessApp::Draw(const GameTimer & gt)
{
	auto cmdAlloc = mCurrentFrameResource->CmdListAlloc.Get();

	ThrowIfFailed(cmdAlloc->Reset());

	ThrowIfFailed(mCommandList->Reset(cmdAlloc, mBezierDemoActive ?
		mPSOs["BezierTess"].Get() : mPSOs["PatchTess"].Get()));

	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	mCommandList->ClearRenderTargetView(CurrentBackBufferView(), DirectX::Colors::LightSteelBlue, 0, nullptr);
	mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

	mCommandList->SetGraphicsRootSignature(mRootSignature.Get());
	ID3D12DescriptorHeap* heaps[1] = { mCbvHeap.Get() };
	mCommandList->SetDescriptorHeaps(_countof(heaps), heaps);

	auto passCbOffset = mCurrentFrameResourceIndex + mMainPassCbOffset;
	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle(mCbvHeap->GetGPUDescriptorHandleForHeapStart(),
		passCbOffset, mCbvSrvDescriptorSize);
	mCommandList->SetGraphicsRootDescriptorTable(1, gpuHandle);

	auto renderLayer = mBezierDemoActive ? RenderLayer::Bezier : RenderLayer::Opaque;

	for (auto i = 0; i < mRenderLayers[(int)renderLayer].size(); ++i) {
		auto rItem = mRenderLayers[(int)renderLayer][i];
		mCommandList->IASetPrimitiveTopology(rItem->PrimitiveType);
		mCommandList->IASetVertexBuffers(0, 1, &rItem->Geo->VertexBufferView());
		mCommandList->IASetIndexBuffer(&rItem->Geo->IndexBufferView());


		UINT cbOffset = (UINT)(mCurrentFrameResourceIndex * mRenderItems.size() + rItem->ObjCBIndex);

		gpuHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(mCbvHeap->GetGPUDescriptorHandleForHeapStart(),
			cbOffset, mCbvSrvDescriptorSize);
		mCommandList->SetGraphicsRootDescriptorTable(0, gpuHandle);
		mCommandList->DrawIndexedInstanced(rItem->IndexCount, 1, rItem->StartIndexLocation, rItem->BaseVertexLocation, 0);
	}


	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	ThrowIfFailed(mCommandList->Close());

	ID3D12CommandList* commandLists[1] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

	// swap the buffer
	ThrowIfFailed(mSwapChain->Present(0, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

	mCurrentFrameResource->Fence = ++mCurrentFence;
	mCommandQueue->Signal(mFence.Get(), mCurrentFence);
}

void TessApp::OnResize()
{
	D3DApp::OnResize();
	auto P = XMMatrixPerspectiveFovLH(XM_PIDIV4, AspectRatio(), mNearZ, mFarZ);
	XMStoreFloat4x4(&mProj, P);
}

void TessApp::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void TessApp::OnMouseDown(WPARAM btnState, int x, int y)
{
	mLastMousePos.x = x;
	mLastMousePos.y = y;
	SetCapture(mhMainWnd);
}

void TessApp::OnMouseMove(WPARAM btnState, int x, int y)
{
	if ((btnState & MK_LBUTTON) != 0) {
		float dx = XMConvertToRadians(0.25f*static_cast<float>(x - mLastMousePos.x));
		float dy = XMConvertToRadians(0.25f*static_cast<float>(y - mLastMousePos.y));

		mTheta += dx;
		mPhi += dy;

		mPhi = MathHelper::Clamp(mPhi, 1.0f, XM_PI - 1.0f);
	}
	else if ((btnState & MK_RBUTTON) != 0) {
		float dx = 5.0f*static_cast<float>(x - mLastMousePos.x);
		float dy = 5.0f*static_cast<float>(y - mLastMousePos.y);

		mRadius += dx - dy;
		mRadius = MathHelper::Clamp(mRadius, mNearZ + 1.0f, mFarZ - 1.0f);
	}

	mLastMousePos.x = x;
	mLastMousePos.y = y;
}

void TessApp::HandleKeyboardInput(const GameTimer & gt)
{
	const static float debounceTime = 0.25f;
	static float t = 0.2f;
	t += gt.DeltaTime();

	if (t > debounceTime) {
		if (GetAsyncKeyState('B') & 0x8000) {
			mBezierDemoActive = !mBezierDemoActive;
			t = 0.0f;
		}
	}
}

void TessApp::UpdateCamera(const GameTimer& gt)
{
	auto x = mRadius*sinf(mPhi)*cosf(mTheta);
	auto z = mRadius*sinf(mPhi)*sinf(mTheta);
	auto y = mRadius*cosf(mPhi);

	mEyePos.x = x;
	mEyePos.y = y;
	mEyePos.z = z;

	XMVECTOR pos = XMVectorSet(x, y, z, 1.0f);
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMMATRIX V = XMMatrixLookAtLH(pos, XMVectorZero(), up);
	XMStoreFloat4x4(&mView, V);
}
