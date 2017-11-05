#include "stdafx.h"
#include "GrowingPyramid.h"

GrowingPyramid::GrowingPyramid(HINSTANCE hInstance) : D3DApp(hInstance)
{
}

GrowingPyramid::~GrowingPyramid()
{
	if (md3dDevice != nullptr) {
		FlushCommandQueue();
	}
}

bool GrowingPyramid::Initialize()
{
	if (!D3DApp::Initialize()) {
		return false;
	}

	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	BuildRootSignature();
	BuildShadersAndInputLayout();
	BuildGeometry();
	BuildRenderItems();
	BuildFrameResources();
	BuildDescriptorHeaps();
	BuildCBVs();
	CreatePipelineStateObjects();

	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* cmdLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);

	FlushCommandQueue();
	return true;
}

void GrowingPyramid::BuildRenderItems()
{
	std::unique_ptr<RenderItem> pyramid;
	auto addGeometricParams = [](const std::unique_ptr<ShapesDemo::RenderItem>& rItem,
		const SubmeshGeometry& geo) {
		rItem->IndexCount = geo.IndexCount;
		rItem->BaseVertexLocation = geo.BaseVertexLocation;
		rItem->StartIndexLocation = geo.StartIndexLocation;
		rItem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	};

	for (uint16_t i = 0; i < mNumPyramidFrames; ++i) {
		pyramid = std::make_unique<RenderItem>(mNumFrameResources, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		pyramid->ObjCBIndex = i;

		// height offset equation: 
		// -(hmax - h) = -(height / 2.0f - height*p / 2.0f) = -0.5f*10.0f*(1-p)
		// p is given as 0.1f*(imax - i) = 0.1*(10u-i), where i is the index of the render item

		// width offset:
		// base * index = 5.0f*i
		XMStoreFloat4x4(&pyramid->World, XMMatrixTranslation(0.0f, -0.5f*(1.0f - (1.0f/mNumPyramidFrames)*(mNumPyramidFrames-i))*(10.0f), 0.0f));
		// pyramid->World = MathHelper::Identity4x4();
		pyramid->Geo = mObject->GetGeometry();
		addGeometricParams(pyramid, mObject->GetSubmesh(pyramid->ObjCBIndex));
		mAllRenderItems.emplace_back(std::move(pyramid));
	}

	auto skull = std::make_unique<RenderItem>(mNumFrameResources, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	skull->Geo = mObject->GetGeometry();
	skull->ObjCBIndex = mNumPyramidFrames;
	XMStoreFloat4x4(&skull->World, XMMatrixScaling(0.5f, 0.5f, 0.5f)*XMMatrixTranslation(10.0f, 0.0f, 0.0f));
	addGeometricParams(skull, mObject->GetSubmesh(skull->ObjCBIndex));
	
	mAllRenderItems.emplace_back(std::move(skull));
	for (auto& rItem : mAllRenderItems) {
		mOpaqueRenderItems.emplace_back(rItem.get());
	}
}

void GrowingPyramid::BuildGeometry()
{
	std::unique_ptr<GeometricObject> geo = std::make_unique<GeometricObject>("PyramidGeometry");
	GeometryGenerator geoGen;
	for (uint16_t i = 10; i > 0; --i) {
		auto pyramid = geoGen.CreatePyramid(10.0f, 10.0f, 0.1f*i, 4);
		geo->AddObject(pyramid);
	}
	geo->AddObject(mSkull.GetVertices(), mSkull.GetIndices());
	geo->BuildGeometry(md3dDevice.Get(), mCommandList.Get());
	geo.swap(mObject);
}

void GrowingPyramid::BuildFrameResources()
{
	for (uint16_t i = 0; i < mNumFrameResources; ++i) {
		mFrameResources.emplace_back(std::make_unique<FrameResource>(md3dDevice.Get(), 1, (UINT)mAllRenderItems.size()));
	}
}

void GrowingPyramid::BuildRootSignature()
{
	// b0 (constant buffers)
	CD3DX12_DESCRIPTOR_RANGE cbvTable0;
	cbvTable0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);

	// b1 (pass buffers)
	CD3DX12_DESCRIPTOR_RANGE cbvTable1;
	cbvTable1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1);

	CD3DX12_ROOT_PARAMETER rootParams[2];
	rootParams[0].InitAsDescriptorTable(1, &cbvTable0);
	rootParams[1].InitAsDescriptorTable(1, &cbvTable1);

	CD3DX12_ROOT_SIGNATURE_DESC desc(2, rootParams, 0, nullptr,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

	ThrowIfFailed(hr);

	if (errorBlob != nullptr)
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());

	ThrowIfFailed(md3dDevice->CreateRootSignature(
		0, serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(mRootSignature.GetAddressOf())
	));
}

void GrowingPyramid::BuildShadersAndInputLayout()
{
	mShaders["standardVS"] = d3dUtil::CompileShader(L"Shaders//color.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["opaquePS"] = d3dUtil::CompileShader(L"Shaders//color.hlsl", nullptr, "PS", "ps_5_1");

	mInputLayout = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
}

void GrowingPyramid::CreatePipelineStateObjects()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
	ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));

	psoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
	psoDesc.pRootSignature = mRootSignature.Get();

	psoDesc.VS = {
		reinterpret_cast<BYTE*>(mShaders["standardVS"]->GetBufferPointer()),
		mShaders["standardVS"]->GetBufferSize()
	};
	psoDesc.PS = {
		reinterpret_cast<BYTE*>(mShaders["opaquePS"]->GetBufferPointer()),
		mShaders["opaquePS"]->GetBufferSize()
	};
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	// psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = mBackBufferFormat;
	psoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	psoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	psoDesc.DSVFormat = mDepthStencilFormat;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPSOs["opaque"])));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC wfDesc = psoDesc;
	wfDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&wfDesc, IID_PPV_ARGS(&mPSOs["opaque_wireframe"])));
}

void GrowingPyramid::BuildDescriptorHeaps()
{
	auto numObjects = (UINT)mAllRenderItems.size();
	mMainPassCbvOffset = numObjects * mNumFrameResources;
	D3D12_DESCRIPTOR_HEAP_DESC desc;
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	// descriptor for every object + per pass per num frame resources
	desc.NumDescriptors = (numObjects + 1) * mNumFrameResources;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	desc.NodeMask = 0;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&mCbvHeap)));
}

void GrowingPyramid::BuildCBVs()
{
	UINT objCbByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
	UINT objCount = (UINT)mAllRenderItems.size();

	for (uint16_t i = 0; i < mNumFrameResources; ++i) {
		auto objectCB = mFrameResources[i]->ObjectCB->Resource();
		for (uint32_t j = 0; j < objCount; ++j) {
			auto cbAddress = objectCB->GetGPUVirtualAddress();
			cbAddress += j*objCbByteSize;

			auto heapIndex = i*objCount + j;
			auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mCbvHeap->GetCPUDescriptorHandleForHeapStart());
			handle.Offset(heapIndex, mCbvSrvDescriptorSize);
			D3D12_CONSTANT_BUFFER_VIEW_DESC desc;
			desc.BufferLocation = cbAddress;
			desc.SizeInBytes = objCbByteSize;
			md3dDevice->CreateConstantBufferView(&desc, handle);
		}
	}

	// add pass CBs on the end
	UINT passCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(PassConstants));
	for (uint16_t i = 0; i < mNumFrameResources; ++i) {
		auto passCB = mFrameResources[i]->PassCB->Resource();
		auto heapIndex = mMainPassCbvOffset + i;
		auto cbAddress = passCB->GetGPUVirtualAddress();

		auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mCbvHeap->GetCPUDescriptorHandleForHeapStart());
		handle.Offset(heapIndex, mCbvSrvDescriptorSize);
		D3D12_CONSTANT_BUFFER_VIEW_DESC desc;
		desc.BufferLocation = cbAddress;
		desc.SizeInBytes = passCBByteSize;
		md3dDevice->CreateConstantBufferView(&desc, handle);
	}
}

void GrowingPyramid::UpdateMainPassCB(const GameTimer& gt) {
	XMMATRIX view = XMLoadFloat4x4(&mView);
	XMMATRIX proj = XMLoadFloat4x4(&mProj);

	XMMATRIX viewProj = XMMatrixMultiply(view, proj);
	XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
	XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
	XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);

	XMStoreFloat4x4(&mMainPassCB.View, XMMatrixTranspose(view));
	XMStoreFloat4x4(&mMainPassCB.InvView, XMMatrixTranspose(invView));
	XMStoreFloat4x4(&mMainPassCB.Proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&mMainPassCB.InvProj, XMMatrixTranspose(invProj));
	XMStoreFloat4x4(&mMainPassCB.ViewProj, XMMatrixTranspose(viewProj));
	XMStoreFloat4x4(&mMainPassCB.InvViewProj, XMMatrixTranspose(viewProj));

	mMainPassCB.EyePosW = mEyePos;
	mMainPassCB.RenderTargetSize = XMFLOAT2((float)mClientWidth, (float)mClientHeight);
	mMainPassCB.InvRenderTargetSize = XMFLOAT2(1.0f / mClientWidth, 1.0f / mClientHeight);
	mMainPassCB.NearZ = 1.0f;
	mMainPassCB.FarZ = 1000.0f;
	mMainPassCB.TotalTime = gt.TotalTime();
	mMainPassCB.DeltaTime = gt.DeltaTime();

	auto currPassCB = mCurrFrameResource->PassCB.get();
	currPassCB->CopyData(0, mMainPassCB);
}

void GrowingPyramid::UpdateCamera(const GameTimer & gt)
{
	mEyePos.x = mRadius*sinf(mPhi)*cosf(mTheta);
	mEyePos.y = mRadius*cosf(mPhi);
	mEyePos.z = mRadius*sinf(mPhi)*sinf(mTheta);
	XMVECTOR eyePos = XMVectorSet(mEyePos.x, mEyePos.y, mEyePos.z, 1.0f);
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMVECTOR target = XMVectorZero();
	XMMATRIX v = XMMatrixLookAtLH(eyePos, target, up);
	XMStoreFloat4x4(&mView, v);
}

void GrowingPyramid::UpdateObjectCBs(const GameTimer & gt)
{
	auto currObjectCB = mCurrFrameResource->ObjectCB.get();
	ObjectConstants objConstants;
	XMMATRIX world;
	for (uint16_t i = 0; i < mNumPyramidFrames; ++i) {
		if (mAllRenderItems[i]->NumFramesDirty > 0) {
			world = XMLoadFloat4x4(&mAllRenderItems[i]->World);
			XMStoreFloat4x4(&objConstants.World, XMMatrixTranspose(world));
			currObjectCB->CopyData(mAllRenderItems[i]->ObjCBIndex, objConstants);
			mAllRenderItems[i]->NumFramesDirty--;
		}
	}

	// update the skull position
	auto baseTranslation = -0.5f*(1.0f - 0.1f*(mNumPyramidFrames - mPyramidIndex))*(10.0f);
	world = XMMatrixScaling(0.5f, 0.5f, 0.5f)*XMMatrixTranslation(0.0f, 5.0f*(1.0f - mPyramidIndex*0.1f) + baseTranslation, 0.0f);
	XMStoreFloat4x4(&objConstants.World, XMMatrixTranspose(world));
	currObjectCB->CopyData(mAllRenderItems[mNumPyramidFrames]->ObjCBIndex, objConstants);
}

void GrowingPyramid::Update(const GameTimer& gt)
{
	OnKeyboardInput(gt);
	UpdateCamera(gt);

	// go to next frame resource
	mCurrFrameResourceIndex = (mCurrFrameResourceIndex + 1) % mNumFrameResources;
	mCurrFrameResource = mFrameResources[mCurrFrameResourceIndex].get();

	// wait if the resource is not ready
	if (mCurrFrameResource->Fence != 0 && mFence->GetCompletedValue() < mCurrFrameResource->Fence) {
		HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
		ThrowIfFailed(mFence->SetEventOnCompletion(mCurrFrameResource->Fence, eventHandle));
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}

	// update the constant buffers
	UpdateMainPassCB(gt);
	UpdateObjectCBs(gt);

}

void GrowingPyramid::Draw(const GameTimer& gt)
{
	auto commandListAlloc = mCurrFrameResource->CmdListAlloc;

	ThrowIfFailed(commandListAlloc->Reset());
	if (mIsWireframe) {
		mCommandList->Reset(commandListAlloc.Get(), mPSOs["opaque_wireframe"].Get());
	}
	else {
		mCommandList->Reset(commandListAlloc.Get(), mPSOs["opaque"].Get());
	}

	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET
	));

	mCommandList->ClearRenderTargetView(CurrentBackBufferView(),
		mIsWireframe ? Colors::White : Colors::LightSteelBlue, 0, nullptr);
	mCommandList->ClearDepthStencilView(DepthStencilView(),
		D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
		1.0f,
		0, 0, nullptr);

	mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());
	ID3D12DescriptorHeap* descriptorHeaps[] = { mCbvHeap.Get() };
	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
	mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

	auto passCbvIndex = mMainPassCbvOffset + mCurrFrameResourceIndex;
	auto handle = CD3DX12_GPU_DESCRIPTOR_HANDLE(mCbvHeap->GetGPUDescriptorHandleForHeapStart());
	handle.Offset(passCbvIndex, mCbvSrvDescriptorSize);
	mCommandList->SetGraphicsRootDescriptorTable(1, handle);

	DrawRenderItems(mCommandList.Get(), mOpaqueRenderItems);

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT
	));

	ThrowIfFailed(mCommandList->Close());

	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	ThrowIfFailed(mSwapChain->Present(0, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;
	mCurrFrameResource->Fence = ++mCurrentFence;
	mCommandQueue->Signal(mFence.Get(), mCurrentFence);
}

void GrowingPyramid::DrawRenderItems(ID3D12GraphicsCommandList * cmdList, const std::vector<RenderItem*>& rItems)
{
	UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
	auto objectCB = mCurrFrameResource->ObjectCB->Resource();
	auto objCount = (UINT)rItems.size();
	auto rItem = rItems[mPyramidIndex];
	cmdList->IASetVertexBuffers(0, 1, &rItem->Geo->VertexBufferView());
	cmdList->IASetIndexBuffer(&rItem->Geo->IndexBufferView());
	cmdList->IASetPrimitiveTopology(rItem->PrimitiveType);

	UINT cbvIndex = mCurrFrameResourceIndex * objCount + rItem->ObjCBIndex;
	auto cbvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(mCbvHeap->GetGPUDescriptorHandleForHeapStart());
	cbvHandle.Offset(cbvIndex, mCbvSrvDescriptorSize);
	cmdList->SetGraphicsRootDescriptorTable(0, cbvHandle);
	cmdList->DrawIndexedInstanced(rItem->IndexCount, 1, rItem->StartIndexLocation, rItem->BaseVertexLocation, 0);

	// draw the skull
	rItem = rItems[mNumPyramidFrames];
	cbvIndex = mCurrFrameResourceIndex * objCount + rItem->ObjCBIndex;
	cbvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(mCbvHeap->GetGPUDescriptorHandleForHeapStart());
	cbvHandle.Offset(cbvIndex, mCbvSrvDescriptorSize);
	cmdList->SetGraphicsRootDescriptorTable(0, cbvHandle);
	cmdList->DrawIndexedInstanced(rItem->IndexCount, 1, rItem->StartIndexLocation, rItem->BaseVertexLocation, 0);

}

void GrowingPyramid::OnResize()
{
	D3DApp::OnResize();

	XMMATRIX p = XMMatrixPerspectiveFovLH(0.25f*MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
	XMStoreFloat4x4(&mProj, p);
}

void GrowingPyramid::OnMouseDown(WPARAM btnState, int x, int y)
{
	mLastMousePos.x = x;
	mLastMousePos.y = y;
	SetCapture(mhMainWnd);
}

void GrowingPyramid::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void GrowingPyramid::OnMouseMove(WPARAM btnState, int x, int y)
{
	if ((btnState & MK_LBUTTON) != 0) {
		float dx = 0.25f*XMConvertToRadians(static_cast<float>(x - mLastMousePos.x));
		float dy = 0.25f*XMConvertToRadians(static_cast<float>(y - mLastMousePos.y));

		mTheta += dx;
		mPhi += dy;

		mPhi = MathHelper::Clamp(mPhi, 0.1f, XM_PI - 0.1f);
	}
	else if ((btnState & MK_RBUTTON) != 0) {
		float dx = 0.10f*(static_cast<float>(x - mLastMousePos.x));
		float dy = 0.10f*(static_cast<float>(y - mLastMousePos.y));
		mRadius += (dx - dy);
		
		mRadius = MathHelper::Clamp(mRadius, 1.0f, 500.0f);
	}
	mLastMousePos.x = x;
	mLastMousePos.y = y;
}

void GrowingPyramid::OnKeyboardInput(const GameTimer & gt)
{
	if (GetAsyncKeyState('1') & 0x8000) {
		mIsWireframe = true;
	}
	else {
		mIsWireframe = false;
	}

	if (GetAsyncKeyState(VK_UP) & 0x8000) {
		if (gt.TotalTime() - lastTime > 0.2f) {
			mPyramidIndex--;
			mPyramidIndex = MathHelper::Clamp<uint16_t>(mPyramidIndex, 0u, 9u);
			lastTime = gt.TotalTime();
		}

	}
	else if (GetAsyncKeyState(VK_DOWN) & 0x8000) {
		if (gt.TotalTime() - lastTime > 0.2f) {
			mPyramidIndex++;
			mPyramidIndex = MathHelper::Clamp<uint16_t>(mPyramidIndex, 0u, 9u);
			lastTime = gt.TotalTime();
		}
	}
}
