#include "stdafx.h"
#include "StencilApp.h"

StencilApp::StencilApp(HINSTANCE hInstance) : D3DApp(hInstance)
{
}

StencilApp::~StencilApp()
{
	if (md3dDevice != nullptr) {
		FlushCommandQueue();
	}
}

bool StencilApp::Initialize()
{
	if (!D3DApp::Initialize()) {
		return false;
	}

	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	CreateRootSignature();
	CreateShaderInputLayout();
	CreatePSOs();
	BuildGeometry();
	BuildRenderItems();
	BuildFrameResources();

	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* commandLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

	FlushCommandQueue();
	return true;
}

void StencilApp::LoadTextures()
{
	// STUB
}

void StencilApp::BuildFrameResources()
{
	for (int i = 0; i < mNumFrameResources; ++i) {
		mFrameResources.emplace_back(
			std::make_unique<FrameResource>(md3dDevice.Get(), 1, (UINT)mRenderItems.size())
		);
	}
}

void StencilApp::BuildGeometry()
{
	GeometryGenerator geoGen;
	auto grid = geoGen.CreateGrid(mGridWidth, 120.0f, 50, 60);
	GeometricObject geoObj("StencilApp");

	geoObj.AddObject(grid);
	geoObj.AddObject(mSkull.GetVertices(), mSkull.GetIndices());
	
	auto convertVertex = [&](const GeometryGenerator::Vertex& vertex) {
		Vertex v;
		v.Pos = vertex.Position;
		v.Normal = vertex.Normal;
		v.TexC = vertex.TexC;
		return v;
	};

	auto vertices = getVertices<Vertex>(&geoObj, convertVertex);
	UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);

	auto indices = geoObj.GetIndices();
	UINT ibByteSize = (UINT)indices.size() * sizeof(uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));

	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->Name = "StencilApp";
	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(), mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);
	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(), mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->DrawArgs["grid"] = geoObj.GetSubmesh(0);
	geo->DrawArgs["skull"] = geoObj.GetSubmesh(1);
	mGeometries[geo->Name] = std::move(geo);
}

void StencilApp::BuildTextures()
{
	// STUB
}

void StencilApp::BuildMaterials()
{
	// STUB
}

void StencilApp::BuildRenderItems()
{
	std::unique_ptr<RenderItem> rItem;
	auto buildRenderItem = [&](std::string&& submeshName, XMMATRIX&& world, UINT&& objCBIndex) {
		rItem = std::make_unique<RenderItem>();
		XMStoreFloat4x4(&rItem->World, world);
		rItem->TexTransform = MathHelper::Identity4x4();
		rItem->ObjCBIndex = objCBIndex;
		rItem->Geo = mGeometries["StencilApp"].get();
		rItem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		rItem->IndexCount = rItem->Geo->DrawArgs[submeshName].IndexCount;
		rItem->StartIndexLocation = rItem->Geo->DrawArgs[submeshName].StartIndexLocation;
		rItem->BaseVertexLocation = rItem->Geo->DrawArgs[submeshName].BaseVertexLocation;
		mRenderLayers[(UINT)RenderLayer::Opaque].emplace_back(rItem.get());
		mRenderItems.emplace_back(std::move(rItem));
	};

	XMMATRIX world = XMLoadFloat4x4(&MathHelper::Identity4x4());
	buildRenderItem("grid", std::move(world), 0);

	world = XMMatrixTranspose(XMMatrixRotationX(XM_PIDIV2)*XMMatrixTranslation(0.0f, +mGridDepth / 2.0f, -mGridDepth / 2.0f));
	buildRenderItem("grid", std::move(world), 1);

	world = XMMatrixTranspose(XMMatrixScaling(8.0f, 8.0f, 8.0f)*XMMatrixRotationY(XM_PIDIV2));
	buildRenderItem("skull", std::move(world), 2);
}

void StencilApp::DrawRenderItems(const std::vector<RenderItem*> rItems)
{
	auto cbByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
	auto baseAddr = mCurrentFrameResource->ObjectCB->Resource()->GetGPUVirtualAddress();
	for (auto& rItem : rItems) {
		mCommandList->IASetVertexBuffers(0, 1, &rItem->Geo->VertexBufferView());
		mCommandList->IASetIndexBuffer(&rItem->Geo->IndexBufferView());
		mCommandList->IASetPrimitiveTopology(rItem->PrimitiveType);

		mCommandList->SetGraphicsRootConstantBufferView(0, baseAddr + rItem->ObjCBIndex * cbByteSize);
		
		mCommandList->DrawIndexedInstanced(rItem->IndexCount, 1, rItem->StartIndexLocation, rItem->BaseVertexLocation, 0);
	}
}

void StencilApp::CreateRootSignature()
{
	CD3DX12_ROOT_PARAMETER params[2];

	params[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_ALL); // object constants
	params[1].InitAsConstantBufferView(1, 0, D3D12_SHADER_VISIBILITY_ALL); // pass constants

	auto rootSigDesc = CD3DX12_ROOT_SIGNATURE_DESC(2, params, 0,
		nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> rootSig;
	ComPtr<ID3DBlob> errorBlob;

	auto handle = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &rootSig, &errorBlob);

	ThrowIfFailed(handle);

	if (errorBlob != nullptr) {
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	
	ThrowIfFailed(md3dDevice->CreateRootSignature(0, rootSig->GetBufferPointer(),
		rootSig->GetBufferSize(), IID_PPV_ARGS(mRootSignature.GetAddressOf())));
}

void StencilApp::CreateShaderInputLayout()
{
	mShaders["standardVS"] = d3dUtil::CompileShader(L"Shaders//geometry.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["opaquePS"] = d3dUtil::CompileShader(L"Shaders//geometry.hlsl", nullptr, "PS", "ps_5_1");

	mInputLayout = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
}

void StencilApp::BuildOpaquePSO(D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc)
{
	desc.pRootSignature = mRootSignature.Get();
	desc.VS = {
		reinterpret_cast<BYTE*>(mShaders["standardVS"]->GetBufferPointer()),
		mShaders["standardVS"]->GetBufferSize()
	};
	desc.PS = {
		reinterpret_cast<BYTE*>(mShaders["opaquePS"]->GetBufferPointer()),
		mShaders["opaquePS"]->GetBufferSize()
	};

	desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	desc.SampleMask = UINT_MAX;
	D3D12_RASTERIZER_DESC rState = {};
	rState.FillMode = D3D12_FILL_MODE_SOLID;
	rState.CullMode = D3D12_CULL_MODE_BACK;
	
	desc.RasterizerState = rState;
	desc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	desc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
	desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	desc.NumRenderTargets = 1;
	desc.RTVFormats[0] = mBackBufferFormat;
	desc.DSVFormat = mDepthStencilFormat;

	desc.SampleDesc.Quality = m4xMsaaState ? m4xMsaaQuality - 1 : 0;
	desc.SampleDesc.Count = m4xMsaaState ? 4 : 1;

	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&mPSOs["opaque"])));

	auto wireframeDesc = desc;
	wireframeDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&wireframeDesc, IID_PPV_ARGS(&mPSOs["wireframe"])));
}

void StencilApp::CreatePSOs()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC desc;
	ZeroMemory(&desc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	BuildOpaquePSO(desc);
}

void StencilApp::OnResize()
{
	D3DApp::OnResize();
	XMMATRIX P = XMMatrixPerspectiveFovLH(XM_PIDIV4, AspectRatio(), mNearPlane, mFarPlane);
	XMStoreFloat4x4(&mProj, P);
}

void StencilApp::Update(const GameTimer & gt)
{
	UpdateCamera(gt);

	mCurrentFrameResourceIndex = (mCurrentFrameResourceIndex + 1) % mNumFrameResources;
	mCurrentFrameResource = mFrameResources[mCurrentFrameResourceIndex].get();

	if (mCurrentFrameResource->Fence != 0 && mCurrentFrameResource->Fence < mFence->GetCompletedValue()) {
		HANDLE handle = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
		ThrowIfFailed(mFence->SetEventOnCompletion(mCurrentFrameResource->Fence, handle));
		WaitForSingleObject(handle, INFINITE);
		CloseHandle(handle);
	}

	UpdateObjectCBs(gt);
	UpdateMainPassCB(gt);
}

void StencilApp::UpdateCamera(const GameTimer & gt)
{
	mEyePos.x = mRadius*sinf(mPhi)*cosf(mTheta);
	mEyePos.y = mRadius*cosf(mPhi);
	mEyePos.z = mRadius*sinf(mPhi)*sinf(mTheta);

	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMVECTOR target = XMVectorZero();
	XMVECTOR eyePos = XMVectorSet(mEyePos.x, mEyePos.y, mEyePos.z, 1.0f);

	XMMATRIX view = XMMatrixLookAtLH(eyePos, target, up);
	XMStoreFloat4x4(&mView, view);
}

void StencilApp::UpdateObjectCBs(const GameTimer & gt)
{
	auto currFrame = mCurrentFrameResource->ObjectCB.get();
	for (auto& rItem : mRenderItems) {
		if (rItem->NumFramesDirty > 0) {
			ObjectConstants oc;
			XMMATRIX w = XMLoadFloat4x4(&rItem->World);
			XMMATRIX tt = XMLoadFloat4x4(&rItem->TexTransform);
			XMStoreFloat4x4(&oc.World, w);
			XMStoreFloat4x4(&oc.TexTransform, tt);
			currFrame->CopyData(rItem->ObjCBIndex, oc);
		}
	}
}

void StencilApp::UpdateMainPassCB(const GameTimer & gt)
{
	XMMATRIX proj = XMLoadFloat4x4(&mProj);
	XMMATRIX view = XMLoadFloat4x4(&mView);
	XMMATRIX viewProj = XMMatrixMultiply(view, proj);

	XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
	XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
	XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);

	XMStoreFloat4x4(&mMainPassCB.View, XMMatrixTranspose(view));
	XMStoreFloat4x4(&mMainPassCB.InvView, XMMatrixTranspose(invView));
	XMStoreFloat4x4(&mMainPassCB.Proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&mMainPassCB.InvProj, XMMatrixTranspose(invProj));
	XMStoreFloat4x4(&mMainPassCB.ViewProj, XMMatrixTranspose(viewProj));
	XMStoreFloat4x4(&mMainPassCB.InvViewProj, XMMatrixTranspose(invViewProj));
	mMainPassCB.EyePosW = mEyePos;
	mMainPassCB.RenderTargetSize = XMFLOAT2((float)mClientWidth, (float)mClientHeight);
	mMainPassCB.InvRenderTargetSize = XMFLOAT2(1.0f / mClientWidth, 1.0f / mClientHeight);
	mMainPassCB.NearZ = 1.0f;
	mMainPassCB.FarZ = 1000.0f;
	mMainPassCB.TotalTime = gt.TotalTime();
	mMainPassCB.DeltaTime = gt.DeltaTime();

	//XMVECTOR lightDir = -MathHelper::SphericalToCartesian(1.0f, mSunTheta, mSunPhi);
	//XMStoreFloat3(&mMainPassCB.Lights[0].Direction, lightDir);
	//lightDir = -MathHelper::SphericalToCartesian(1.0f, XM_PIDIV4, XM_PIDIV4);
	//XMStoreFloat3(&mMainPassCB.Lights[1].Direction, lightDir);

	//mMainPassCB.Lights[0].Strength = XMFLOAT3(1.0f, 1.0f, 0.9f);
	//mMainPassCB.Lights[1].Strength = XMFLOAT3(1.0f, 0.2f, 0.2f);
	mCurrentFrameResource->PassCB->CopyData(0, mMainPassCB);
}

void StencilApp::Draw(const GameTimer & gt)
{
	auto commandAlloc = mCurrentFrameResource->CmdListAlloc.Get();

	ThrowIfFailed(mCommandList->Reset(commandAlloc, mPSOs["wireframe"].Get()));

	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);
	mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		CurrentBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	mCommandList->ClearRenderTargetView(CurrentBackBufferView(), DirectX::Colors::LightSteelBlue, 0, nullptr);
	mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_STENCIL | D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

	auto baseAddr = mCurrentFrameResource->PassCB->Resource()->GetGPUVirtualAddress();
	mCommandList->SetGraphicsRootConstantBufferView(1, baseAddr);

	DrawRenderItems(mRenderLayers[(UINT)RenderLayer::Opaque]);

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		CurrentBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT
	));

	ThrowIfFailed(mCommandList->Close());

	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// oresent and swap
	mSwapChain->Present(0, 0);
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

	mCurrentFrameResource->Fence = ++mCurrentFence;
	mCommandQueue->Signal(mFence.Get(), mCurrentFence);
}

void StencilApp::OnMouseDown(WPARAM btnState, int x, int y)
{
	mLastMousePos.x = x;
	mLastMousePos.y = y;
	SetCapture(mhMainWnd);
}

void StencilApp::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void StencilApp::OnMouseMove(WPARAM btnState, int x, int y)
{
	if ((btnState & MK_LBUTTON) != 0) {
		float dx = XMConvertToRadians(0.25f*static_cast<float>(x - mLastMousePos.x));
		float dy = XMConvertToRadians(0.25f*static_cast<float>(y - mLastMousePos.y));

		mTheta += dx;
		mPhi += dy;
		mPhi = MathHelper::Clamp(mPhi, 0.1f, XM_PI - 0.1f);
	}
	else if ((btnState & MK_RBUTTON) != 0) {
		float dx = 0.5f*static_cast<float>(x - mLastMousePos.x);
		float dy = 0.5f*static_cast<float>(y - mLastMousePos.y);

		mRadius += dx - dy;

		mRadius = MathHelper::Clamp(mRadius, mNearPlane, mFarPlane);
	}

	mLastMousePos.x = x;
	mLastMousePos.y = y;
}
