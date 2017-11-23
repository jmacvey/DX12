#include "stdafx.h"
#include "LitColumns.h"

LitColumns::LitColumns(HINSTANCE hInstance) : D3DApp(hInstance)
{
}

LitColumns::~LitColumns()
{
	if (md3dDevice != nullptr) {
		FlushCommandQueue();
	}
}

bool LitColumns::Initialize()
{
	if (!D3DApp::Initialize()) {
		return false;
	}
	// try to reset command list
	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	BuildRootSignature();
	SetShadersAndInputLayout();
	BuildGeometry();
	BuildMaterials();
	BuildRenderItems();
	BuildFrameResources();
	BuildDescriptorHeaps();
	BuildConstantBuffers();
	BuildPSOs();

	ThrowIfFailed(mCommandList->Close());

	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	FlushCommandQueue();
	return true;
}

void LitColumns::OnResize()
{
	D3DApp::OnResize();

	XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f*MathHelper::Pi, AspectRatio(), 1.0f, 2000.0f);
	XMStoreFloat4x4(&mProj, P);
}

void LitColumns::OnMouseDown(WPARAM btnState, int x, int y) {
	mLastMousePos.x = x;
	mLastMousePos.y = y;

	SetCapture(mhMainWnd);
}

void LitColumns::OnMouseUp(WPARAM btnState, int x, int y) {
	ReleaseCapture();
}

void LitColumns::OnMouseMove(WPARAM btnState, int x, int y) {
	if ((btnState & MK_LBUTTON) != 0) {
		float dx = XMConvertToRadians(0.25f*static_cast<float>(x - mLastMousePos.x));
		float dy = XMConvertToRadians(0.25f*static_cast<float>(y - mLastMousePos.y));

		mTheta += dx;
		mPhi += dy;

		mPhi = MathHelper::Clamp(mPhi, 0.1f, XM_PI - 0.1f);
	}
	else if ((btnState & MK_RBUTTON) != 0) {

		float dx = 0.05f*static_cast<float>(x - mLastMousePos.x);
		float dy = 0.05f*static_cast<float>(y - mLastMousePos.y);

		mRadius += dx - dy;

		mRadius = MathHelper::Clamp(mRadius, 5.0f, 150.0f);
	}

	mLastMousePos.x = x;
	mLastMousePos.y = y;
}

void LitColumns::OnKeyboardInput(const GameTimer & gt)
{
	auto dt = gt.DeltaTime();
	if (GetAsyncKeyState(VK_LEFT) & 0x8000) {
		mSunTheta -= 1.0f*dt;
	}
	if (GetAsyncKeyState(VK_RIGHT) & 0x8000) {
		mSunTheta += 1.0f*dt;
	}

	if (GetAsyncKeyState(VK_UP) & 0x8000) {
		mSunPhi -= 1.0f*dt;
	}

	if (GetAsyncKeyState(VK_DOWN) & 0x8000) {
		mSunPhi += 1.0f*dt;
	}

	mSunPhi = MathHelper::Clamp(mSunPhi, 0.1f, XM_PI);
}

void LitColumns::Update(const GameTimer & gt)
{
	UpdateCamera(gt);
	OnKeyboardInput(gt);

	mCurrFrameResourceIndex = (mCurrFrameResourceIndex + 1) % mNumFrameResources;
	mCurrFrameResource = mFrameResources[mCurrFrameResourceIndex].get();

	// block if frame hasn't returned
	if (mCurrFrameResource->Fence != 0 && mFence->GetCompletedValue() < mCurrFrameResource->Fence) {
		HANDLE eventHandle = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
		ThrowIfFailed(mFence->SetEventOnCompletion(mCurrFrameResource->Fence, eventHandle));
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}

	UpdatePassCB(gt);
	UpdateMaterialCBs(gt);
	UpdateObjectCBs(gt);
}

void LitColumns::Draw(const GameTimer& gt)
{
	auto cmdListAlloc = mCurrFrameResource->CmdListAlloc;

	ThrowIfFailed(cmdListAlloc->Reset());

	ThrowIfFailed(mCommandList->Reset(cmdListAlloc.Get(), mPSOs["opaque"].Get()));


	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	// indicate transition
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		CurrentBackBuffer(), D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET
	));

	// clear rtv
	mCommandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::LightSteelBlue, 0, nullptr);
	mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_STENCIL | D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

	ID3D12DescriptorHeap* descriptorHeaps[1] = { mCbvHeap.Get() };
	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
	mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

	// add pass constants
	auto passDescriptorAddr = CD3DX12_GPU_DESCRIPTOR_HANDLE(mCbvHeap->GetGPUDescriptorHandleForHeapStart());
	passDescriptorAddr.Offset(mPassCbIndexOffset + mCurrFrameResourceIndex, mCbvSrvDescriptorSize);
	mCommandList->SetGraphicsRootDescriptorTable(2, passDescriptorAddr);

	DrawRenderItems(mAllRenderItems);

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		CurrentBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT
	));

	ThrowIfFailed(mCommandList->Close());

	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// present swap chain
	ThrowIfFailed(mSwapChain->Present(0, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

	// update fence
	mCurrFrameResource->Fence = ++mCurrentFence;
	mCommandQueue->Signal(mFence.Get(), mCurrentFence);
}

void LitColumns::UpdateCamera(const GameTimer& gt)
{
	mEyePos.x = mRadius*sinf(mPhi)*cosf(mTheta);
	mEyePos.y = mRadius*cosf(mPhi);
	mEyePos.z = mRadius*sinf(mPhi)*sinf(mTheta);

	XMVECTOR target = XMVectorZero();
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMVECTOR pos = XMVectorSet(mEyePos.x, mEyePos.y, mEyePos.z, 1.0f);
	XMMATRIX v = XMMatrixLookAtLH(pos, target, up);

	XMStoreFloat4x4(&mView, v);
}

void LitColumns::BuildRootSignature()
{
	D3D12_DESCRIPTOR_RANGE cbvTable0 = CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0); // object
	D3D12_DESCRIPTOR_RANGE cbvTable1 = CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1); // material
	D3D12_DESCRIPTOR_RANGE cbvTable2 = CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 2); // pass

	CD3DX12_ROOT_PARAMETER rootParams[3];
	rootParams[0].InitAsDescriptorTable(1, &cbvTable0);
	rootParams[1].InitAsDescriptorTable(1, &cbvTable1);
	rootParams[2].InitAsDescriptorTable(1, &cbvTable2);

	CD3DX12_ROOT_SIGNATURE_DESC desc(3, rootParams, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> rootSig;
	ComPtr<ID3DBlob> errorBlob;
	HRESULT hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1_0, rootSig.GetAddressOf(), errorBlob.GetAddressOf());

	ThrowIfFailed(hr);

	if (errorBlob != nullptr) {
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}

	ThrowIfFailed(md3dDevice->CreateRootSignature(0, rootSig->GetBufferPointer(), rootSig->GetBufferSize(),
		IID_PPV_ARGS(mRootSignature.GetAddressOf())));
}

void LitColumns::BuildDescriptorHeaps()
{
	mPassCbIndexOffset = ((UINT)mAllRenderItems.size() + (UINT)mMaterials.size())* mNumFrameResources;
	
	// materials start at numRenderItems
	// pass constants start at numRenderItems * frames + numMaterials
	mMatCbIndexOffset = (UINT)mAllRenderItems.size() * mNumFrameResources;
	D3D12_DESCRIPTOR_HEAP_DESC desc;
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	desc.NumDescriptors = ((UINT)mAllRenderItems.size() + (UINT)mMaterials.size() + 1)* mNumFrameResources;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	desc.NodeMask = 0;
	md3dDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(mCbvHeap.GetAddressOf()));
}

void LitColumns::BuildGeometry()
{
	GeometryGenerator geoGen;
	auto cylinder = geoGen.CreateCylinder(3.0f, 1.0f, 10.0f, 20, 20);
	auto grid = geoGen.CreateGrid(160.0f, 120.0f, 40, 50);
	GeometricObject geoObj("columns");
	geoObj.AddObject(cylinder);
	geoObj.AddObject(grid);

	auto convertVertex = [](const GeometryGenerator::Vertex& v) -> Vertex {
		Vertex vTo;
		vTo.Pos = v.Position;
		vTo.Normal = v.Normal;
		return vTo;
	};

	auto vertices = getVertices<Vertex>(&geoObj, convertVertex);
	auto meshGeometry = std::make_unique<MeshGeometry>();
	UINT ibByteSize = (UINT)geoObj.GetIndices().size() * sizeof(uint16_t);
	auto indices = geoObj.GetIndices();

	UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);

	meshGeometry->IndexBufferByteSize = ibByteSize;
	meshGeometry->VertexBufferByteSize = vbByteSize;
	meshGeometry->VertexByteStride = sizeof(Vertex);
	meshGeometry->Name = "columns";
	meshGeometry->IndexFormat = DXGI_FORMAT_R16_UINT;

	D3DCreateBlob(vbByteSize, &meshGeometry->VertexBufferCPU);
	D3DCreateBlob(ibByteSize, &meshGeometry->IndexBufferCPU);

	CopyMemory(meshGeometry->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);
	CopyMemory(meshGeometry->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);
	meshGeometry->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), vertices.data(), vbByteSize, meshGeometry->VertexBufferUploader);
	meshGeometry->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, meshGeometry->IndexBufferUploader);

	//auto sphere = geoGen.CreateGeosphere(2.0f, 2);
	meshGeometry->DrawArgs["cylinder"] = geoObj.GetSubmesh(0);
	meshGeometry->DrawArgs["grid"] = geoObj.GetSubmesh(1);
	mGeometries[meshGeometry->Name] = std::move(meshGeometry);
}

void LitColumns::BuildRenderItems()
{
	std::unique_ptr<RenderItem> obj;

	auto addSubmesh = [](RenderItem* rItem, const SubmeshGeometry& submesh) {
		rItem->IndexCount = submesh.IndexCount;
		rItem->BaseVertexLocation = submesh.BaseVertexLocation;
		rItem->StartIndexLocation = submesh.StartIndexLocation;
		rItem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;;
	};

	for (uint16_t i = 0; i < 4; ++i) {
		obj = std::make_unique<RenderItem>();
		obj->ObjCBIndex = i;
		addSubmesh(obj.get(), mGeometries["columns"]->DrawArgs["cylinder"]);
		obj->Geo = mGeometries["columns"].get();
		obj->Mat = mMaterials["gold"].get();
		XMStoreFloat4x4(&obj->World, XMMatrixTranslation(5.0f + i % 2 == 0 ? i * 6.0f : i * -6.0f,
			5.0f, 
			i % 2 == 0 ? -5.0f : + 5.0f));
		mAllRenderItems.emplace_back(std::move(obj));
	}

	obj = std::make_unique<RenderItem>();
	obj->ObjCBIndex = 4;
	addSubmesh(obj.get(), mGeometries["columns"]->DrawArgs["grid"]);
	obj->Geo = mGeometries["columns"].get();
	obj->Mat = mMaterials["grass"].get();
	obj->World = MathHelper::Identity4x4();
	mAllRenderItems.emplace_back(std::move(obj));
}

void LitColumns::SetShadersAndInputLayout()
{

	mShaders["standardVS"] = d3dUtil::CompileShader(L"Shaders\\Default.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["opaquePS"] = d3dUtil::CompileShader(L"Shaders\\Default.hlsl", nullptr, "PS", "ps_5_1");

	mInputLayout = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
}

void LitColumns::BuildFrameResources()
{
	for (int i = 0; i < mNumFrameResources; ++i) {
		mFrameResources.emplace_back(std::make_unique<FrameResource>(md3dDevice.Get(), 1, (UINT)mAllRenderItems.size(), (UINT)mMaterials.size()));
	}
}

// Object constants | Material Constants | Pass Constants
void LitColumns::BuildConstantBuffers()
{
	// object constants
	auto objCbByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
	auto numRenderItems = (UINT) mAllRenderItems.size();
	for (auto i = 0; i < mNumFrameResources; ++i) {
		auto frameResource = mFrameResources[i]->ObjectCB->Resource();
		for (uint32_t j = 0; j < numRenderItems; ++j) {

			// offset to the object cb address
			auto cbAddress = frameResource->GetGPUVirtualAddress();
			cbAddress += j * objCbByteSize;

			D3D12_CONSTANT_BUFFER_VIEW_DESC desc;
			desc.BufferLocation = cbAddress;
			desc.SizeInBytes = objCbByteSize;

			CD3DX12_CPU_DESCRIPTOR_HANDLE handle(mCbvHeap->GetCPUDescriptorHandleForHeapStart());
			handle.Offset(i * numRenderItems + j, mCbvSrvDescriptorSize);

			md3dDevice->CreateConstantBufferView(&desc, handle);
		}
	}

	// material constants
	auto matCbByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(MaterialConstants));
	auto numMaterials = (UINT)mMaterials.size();
	for (auto i = 0; i < mNumFrameResources; ++i) {
		auto frameResource = mFrameResources[i]->MaterialCB->Resource();
		for (uint16_t j = 0; j < numMaterials; ++j) {
			auto cbAddress = frameResource->GetGPUVirtualAddress();
			cbAddress += j * matCbByteSize;

			D3D12_CONSTANT_BUFFER_VIEW_DESC desc;
			desc.BufferLocation = cbAddress;
			desc.SizeInBytes = matCbByteSize;

			auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mCbvHeap->GetCPUDescriptorHandleForHeapStart());
			handle.Offset(mMatCbIndexOffset + i * numMaterials + j, mCbvSrvDescriptorSize);
			md3dDevice->CreateConstantBufferView(&desc, handle);
		}
	}

	// pass constants
	auto passCbByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(PassConstants));
	for (auto i = 0; i < mNumFrameResources; ++i) {
		auto frameResource = mFrameResources[i]->PassCB->Resource();
		auto cbAddress = frameResource->GetGPUVirtualAddress();
		D3D12_CONSTANT_BUFFER_VIEW_DESC desc;
		desc.BufferLocation = cbAddress;
		desc.SizeInBytes = passCbByteSize;

		CD3DX12_CPU_DESCRIPTOR_HANDLE handle(mCbvHeap->GetCPUDescriptorHandleForHeapStart());
		handle.Offset(mPassCbIndexOffset + i, mCbvSrvDescriptorSize);
		md3dDevice->CreateConstantBufferView(&desc, handle);
	}
}

void LitColumns::BuildMaterials()
{
	std::unique_ptr<Material> grass = std::make_unique<Material>(mNumFrameResources);
	grass->MatCBIndex = 0;
	grass->Name = "grass";
	grass->DiffuseAlbedo = XMFLOAT4(0.2f, 0.6f, 0.2f, 1.0f);
	grass->FresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);
	grass->Roughness = 0.125f;
	mMaterials["grass"] = std::move(grass);

	std::unique_ptr<Material> gold = std::make_unique<Material>(mNumFrameResources);
	gold->MatCBIndex = 1;
	gold->Name = "gold";
	gold->DiffuseAlbedo = XMFLOAT4(1.0f, 0.5f, 0.2f, 1.0f);
	gold->FresnelR0 = XMFLOAT3(0.55f, 0.46f, 0.00f);
	gold->Roughness = 0.01f;
	mMaterials["gold"] = std::move(gold);
}

void LitColumns::BuildPSOs()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC desc;
	ZeroMemory(&desc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	desc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
	desc.pRootSignature = mRootSignature.Get();
	desc.VS = { reinterpret_cast<BYTE*>(mShaders["standardVS"]->GetBufferPointer()), mShaders["standardVS"]->GetBufferSize() };
	desc.PS = { reinterpret_cast<BYTE*>(mShaders["opaquePS"]->GetBufferPointer()), mShaders["opaquePS"]->GetBufferSize() };
	desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	desc.SampleMask = UINT_MAX;

	CD3DX12_RASTERIZER_DESC rasterizerDesc(D3D12_DEFAULT);
	// rasterizerDesc.FillMode = D3D12_FILL_MODE_WIREFRAME;
	// rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
	desc.RasterizerState = rasterizerDesc;
	desc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	desc.NumRenderTargets = 1;
	desc.RTVFormats[0] = mBackBufferFormat;
	desc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	desc.SampleDesc.Quality = m4xMsaaState ? m4xMsaaQuality - 1 : 0;
	desc.DSVFormat = mDepthStencilFormat;

	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&mPSOs["opaque"])));
}

void LitColumns::UpdateObjectCBs(const GameTimer & gt)
{
	auto currObjectCB = mCurrFrameResource->ObjectCB.get();
	auto currMaterialCB = mCurrFrameResource->MaterialCB.get();
	for (auto& renderItem : mAllRenderItems) {
		if (renderItem->NumFramesDirty > 0) {
			ObjectConstants oc;
			XMMATRIX world = XMLoadFloat4x4(&renderItem->World);
			XMStoreFloat4x4(&oc.World, XMMatrixTranspose(world));
			currObjectCB->CopyData(renderItem->ObjCBIndex, oc);
			renderItem->NumFramesDirty--;
		}
	}
}

void LitColumns::UpdateMaterialCBs(const GameTimer & gt)
{
	auto currMaterialCB = mCurrFrameResource->MaterialCB.get();
	for (auto& material : mMaterials) {
		Material* mat = material.second.get();
		if (mat->NumFramesDirty > 0) {
			MaterialConstants mc;
			mc.DiffuseAlbedo = mat->DiffuseAlbedo;
			mc.FresnelR0 = mat->FresnelR0;
			mc.Roughness = mat->Roughness;

			currMaterialCB->CopyData(mat->MatCBIndex, mc);
			mat->NumFramesDirty--;
		}
	}
}

void LitColumns::UpdatePassCB(const GameTimer & gt)
{
	XMMATRIX view = XMLoadFloat4x4(&mView);
	XMMATRIX proj = XMLoadFloat4x4(&mProj);
	XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
	XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
	XMMATRIX viewProj = view*proj;
	XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);

	XMStoreFloat4x4(&mMainPassCB.View, XMMatrixTranspose(view));
	XMStoreFloat4x4(&mMainPassCB.InvProj, XMMatrixTranspose(invView));
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

	XMVECTOR lightDir = -MathHelper::SphericalToCartesian(1.0f, mSunTheta, mSunPhi);

	XMStoreFloat3(&mMainPassCB.Lights[0].Direction, lightDir);
	mMainPassCB.Lights[0].Strength = { 1.0f, 1.0f, 0.9f };

	auto currPassCB = mCurrFrameResource->PassCB.get();
	currPassCB->CopyData(0, mMainPassCB);
}

void LitColumns::DrawRenderItems(const std::vector<std::unique_ptr<RenderItem>>& renderItems)
{
	auto sz = renderItems.size();
	auto matSz = (UINT)mMaterials.size();
	for (uint32_t i = 0; i < sz; ++i) {
		auto rItem = renderItems[i].get();

		// bind vertex and index buffers
		mCommandList->IASetVertexBuffers(0, 1, &rItem->Geo->VertexBufferView());
		mCommandList->IASetIndexBuffer(&rItem->Geo->IndexBufferView());
		mCommandList->IASetPrimitiveTopology(rItem->PrimitiveType);

		// bind object cosntants
		auto cbvIndex = rItem->ObjCBIndex + mCurrFrameResourceIndex * sz;
		auto descriptorAddr = CD3DX12_GPU_DESCRIPTOR_HANDLE(mCbvHeap->GetGPUDescriptorHandleForHeapStart());
		descriptorAddr.Offset(cbvIndex, mCbvSrvDescriptorSize);
		mCommandList->SetGraphicsRootDescriptorTable(0, descriptorAddr);

		// bind material constants
		auto matCbvIndex = mMatCbIndexOffset + rItem->Mat->MatCBIndex + matSz * mCurrFrameResourceIndex;
		auto matDescriptorHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(mCbvHeap->GetGPUDescriptorHandleForHeapStart());
		matDescriptorHandle.Offset(matCbvIndex, mCbvSrvDescriptorSize);
		mCommandList->SetGraphicsRootDescriptorTable(1, matDescriptorHandle);

		mCommandList->DrawIndexedInstanced(rItem->IndexCount, 1, rItem->StartIndexLocation, rItem->BaseVertexLocation, 0);
	}
}
