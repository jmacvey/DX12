#include "stdafx.h"
#include "CrateApp.h"

CrateApp::CrateApp(HINSTANCE hInstance) : D3DApp(hInstance)
{
}

CrateApp::~CrateApp()
{
	if (md3dDevice != nullptr) {
		FlushCommandQueue();
	}
}

bool CrateApp::Initialize()
{
	if (!D3DApp::Initialize()) {
		return false;
	}

	// reset the command list
	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));
	
	BuildTextures();
	BuildRootSignature();
	SetShadersAndInputLayout();
	BuildGeometry();
	BuildMaterials();
	BuildRenderItems();
	BuildFrameResources();
	BuildDescriptorHeaps();
	BuildConstantBuffers();
	BuildTextureDescriptors();
	BuildPSOs();

	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* cmdLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);

	FlushCommandQueue();
	return true;
}

void CrateApp::BuildRootSignature()
{
	auto cbvTable0 =  CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
		1, 0); // object constant b0

	auto cbvTable1 = CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
		1, 2); // pass constants b2

	auto srvTable0 = CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		1, 0); // textures in t0

	auto srvTable1 = CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		1, 1);

	CD3DX12_ROOT_PARAMETER params[5];
	
	params[0].InitAsDescriptorTable(1, &cbvTable0, D3D12_SHADER_VISIBILITY_ALL);
	params[1].InitAsConstantBufferView(1, 0, D3D12_SHADER_VISIBILITY_ALL); // mat constants b1
	params[2].InitAsDescriptorTable(1, &cbvTable1, D3D12_SHADER_VISIBILITY_ALL);
	params[3].InitAsDescriptorTable(1, &srvTable0, D3D12_SHADER_VISIBILITY_PIXEL);
	params[4].InitAsDescriptorTable(1, &srvTable1, D3D12_SHADER_VISIBILITY_PIXEL);

	auto staticSamplers = GetStaticSamplers();

	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(5, params, (UINT)staticSamplers.size(),
		staticSamplers.data(), D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> sigBlob = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	auto handle = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, sigBlob.GetAddressOf(),
		errorBlob.GetAddressOf());

	if (errorBlob != nullptr)
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	ThrowIfFailed(handle);


	ThrowIfFailed(md3dDevice->CreateRootSignature(
		0, sigBlob->GetBufferPointer(),
		sigBlob->GetBufferSize(),
		IID_PPV_ARGS(mRootSignature.GetAddressOf())
	));
}

void CrateApp::SetShadersAndInputLayout()
{
	mShaders["StandardVS"] = d3dUtil::CompileShader(L"Shaders\\default.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["OpaquePS"] = d3dUtil::CompileShader(L"Shaders\\default.hlsl", nullptr, "PS", "ps_5_1");

	mInputLayout = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
}

void CrateApp::BuildGeometry()
{
	GeometryGenerator geoGen;
	GeometricObject obj("CrateApp");
	obj.AddObject(geoGen.CreateBox(5.0f, 5.0f, 5.0f, 2));

	auto convertVertex = [](const GeometryGenerator::Vertex& v) -> Vertex {
		Vertex toReturn;
		toReturn.Pos = v.Position;
		toReturn.Normal = v.Normal;
		toReturn.TexC = v.TexC;
		return toReturn;
	};

	auto vertices = getVertices<Vertex>(&obj, convertVertex);
	auto indices = obj.GetIndices();
	
	auto vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	auto ibByteSize = (UINT)indices.size() * sizeof(uint16_t);

	std::unique_ptr<MeshGeometry> geo = std::make_unique<MeshGeometry>();
	geo->Name = "CrateApp";

	// allocate memory
	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));

	// copy memory
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(), mCommandList.Get(), vertices.data(), vbByteSize,
		geo->VertexBufferUploader);
	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(), mCommandList.Get(), indices.data(), ibByteSize,
		geo->IndexBufferUploader);
	geo->VertexByteStride = sizeof(Vertex);
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexBufferByteSize = ibByteSize;
	geo->DrawArgs["box"] = obj.GetSubmesh(0);
	
	mGeometries[geo->Name] = std::move(geo);
}

void CrateApp::BuildMaterials()
{
	std::unique_ptr<Material> mat = std::make_unique<Material>(mNumFrameResources);
	mat->Name = "flare";
	mat->DiffuseSrvHeapIndex = 1;
	mat->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	mat->FresnelR0 = { 0.6f, 0.4f, 0.6f };
	mat->Roughness = .80f;
	mat->MatCBIndex = 0;

	mMaterials[mat->Name] = std::move(mat);
}

void CrateApp::BuildRenderItems()
{

	auto addGeometricParams = [&](std::unique_ptr<RenderItem>& rItem,
		const SubmeshGeometry& submesh) {
		rItem->IndexCount = submesh.IndexCount;
		rItem->StartIndexLocation = submesh.StartIndexLocation;
		rItem->BaseVertexLocation = submesh.BaseVertexLocation;
		rItem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	};

	std::unique_ptr<RenderItem> obj = std::make_unique<RenderItem>();
	obj->ObjCBIndex = 0;
	obj->Geo = mGeometries["CrateApp"].get();
	obj->World = MathHelper::Identity4x4();
	obj->Mat = mMaterials["flare"].get();
	addGeometricParams(obj, obj->Geo->DrawArgs["box"]);
	mCrates.emplace_back(obj.get());
	mRenderItems.emplace_back(std::move(obj));
}

void CrateApp::BuildDescriptorHeaps()
{
	// cbv heap for geometry
	mPassCbIndexOffset = (mRenderItems.size()) * mNumFrameResources;
	
	// texC is final location in descriptor heap
	mTexCIndexOffset = mPassCbIndexOffset + mNumFrameResources;

	D3D12_DESCRIPTOR_HEAP_DESC desc;
	desc.NumDescriptors = (mTexCIndexOffset + (UINT)mTextureNames.size());
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	desc.NodeMask = 0;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(mCbvHeap.GetAddressOf())));
}

void CrateApp::BuildConstantBuffers()
{
	UINT objectCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
	UINT passCbByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(PassConstants));

	int numRenderItems = (UINT)mRenderItems.size();
	// create the object constnats
	for (int i = 0; i < mNumFrameResources; ++i) {
		// frame resource object constant buffer = frocb
		auto frocb = mFrameResources[i]->ObjectCB->Resource();
		for (int j = 0; j < numRenderItems; ++j) {
			auto cbAddress = frocb->GetGPUVirtualAddress();
			cbAddress += j * objectCBByteSize;
			CD3DX12_CPU_DESCRIPTOR_HANDLE handle(mCbvHeap->GetCPUDescriptorHandleForHeapStart());
			handle.Offset(i * numRenderItems + j, mCbvSrvDescriptorSize);
			D3D12_CONSTANT_BUFFER_VIEW_DESC desc;
			desc.BufferLocation = cbAddress;
			desc.SizeInBytes = objectCBByteSize;
			md3dDevice->CreateConstantBufferView(&desc, handle);
		}
	}

	// pass constants
	for (int i = 0; i < mNumFrameResources; ++i) {
		auto frpcb = mFrameResources[i]->PassCB->Resource();
		auto cbAddress = frpcb->GetGPUVirtualAddress();
		CD3DX12_CPU_DESCRIPTOR_HANDLE handle(mCbvHeap->GetCPUDescriptorHandleForHeapStart());

		handle.Offset(mPassCbIndexOffset + i, mCbvSrvDescriptorSize);
		D3D12_CONSTANT_BUFFER_VIEW_DESC desc;
		desc.BufferLocation = cbAddress;
		desc.SizeInBytes = passCbByteSize;
		md3dDevice->CreateConstantBufferView(&desc, handle);
	}
}

void CrateApp::BuildFrameResources()
{
	for (int i = 0; i < mNumFrameResources; ++i) {
		mFrameResources.emplace_back(
			std::make_unique<FrameResource>(md3dDevice.Get(), 1, (UINT)mRenderItems.size(), (UINT)mMaterials.size())
		);
	}
}

void CrateApp::BuildTextures()
{

	auto createTexture = [&](std::string&& texName, std::wstring&& fileName) {
		auto tex = std::make_unique<Texture>();
		tex->Name = texName;
		tex->Filename = fileName;
		ThrowIfFailed(CreateDDSTextureFromFile12(md3dDevice.Get(), mCommandList.Get(),
			tex->Filename.c_str(), tex->Resource, tex->UploadHeap));
		mTextureNames.emplace_back(tex->Name);
		mTextures[tex->Name] = std::move(tex);
	};

	createTexture("crate", L"Textures//WoodCrate01.dds");
	createTexture("flare", L"Textures//flare.dds");
	createTexture("flareAlpha", L"Textures/flarealpha.dds");
}

void CrateApp::BuildTextureDescriptors()
{
	auto crateTex = mTextures["crate"]->Resource;
	auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mCbvHeap->GetCPUDescriptorHandleForHeapStart());
	handle.Offset(mTexCIndexOffset, mCbvSrvDescriptorSize);
	D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
	desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	desc.Texture2D.ResourceMinLODClamp = 0.0f;
	desc.Texture2D.MostDetailedMip = 0;

	UINT numTextures = (UINT) mTextureNames.size();
	for (UINT i = 0; i < numTextures; ++i) {
		auto resource = mTextures[mTextureNames[i]]->Resource;
		desc.Format = resource->GetDesc().Format;
		desc.Texture2D.MipLevels = resource->GetDesc().MipLevels;
		md3dDevice->CreateShaderResourceView(resource.Get(), &desc, handle);
		handle.Offset(1, mCbvSrvDescriptorSize);
	}
}

std::array<const CD3DX12_STATIC_SAMPLER_DESC, 1> CrateApp::GetStaticSamplers()
{
	const CD3DX12_STATIC_SAMPLER_DESC anisotropicWrap(
		0, // shader register
		D3D12_FILTER_ANISOTROPIC,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP, // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP, // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP, // address W
		0.0f, // mip LODBias
		8);

	return { anisotropicWrap };
}

void CrateApp::BuildPSOs()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC desc;
	ZeroMemory(&desc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));

	desc.pRootSignature = mRootSignature.Get();
	desc.VS = { reinterpret_cast<BYTE*>(mShaders["StandardVS"]->GetBufferPointer()),
				mShaders["StandardVS"]->GetBufferSize() };
	desc.PS = { reinterpret_cast<BYTE*>(mShaders["OpaquePS"]->GetBufferPointer()),
		mShaders["OpaquePS"]->GetBufferSize()
	};
	desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	desc.SampleMask = UINT_MAX;
	auto rasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	desc.RasterizerState = rasterizerState;
	desc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	desc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
	desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	desc.NumRenderTargets = 1;
	desc.RTVFormats[0] = mBackBufferFormat;
	desc.DSVFormat = mDepthStencilFormat;
	desc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	desc.SampleDesc.Quality = m4xMsaaState ? m4xMsaaQuality - 1 : 0;
	
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&mPSOs["opaque"])));
}

void CrateApp::OnResize()
{
	D3DApp::OnResize();
	XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f*MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
	XMStoreFloat4x4(&mProj, P);
}

void CrateApp::Update(const GameTimer & gt)
{
	UpdateCamera(gt);
	OnKeyboardInput(gt);
	AnimateFlares(gt);

	mCurrentFrameIndex = (mCurrentFrameIndex + 1) % mNumFrameResources;
	mCurrentFrameResource = mFrameResources[mCurrentFrameIndex].get();

	// block if frame hasn't returned
	if (mCurrentFrameResource->Fence != 0 && mFence->GetCompletedValue() < mCurrentFrameResource->Fence) {
		HANDLE eventHandle = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
		ThrowIfFailed(mFence->SetEventOnCompletion(mCurrentFrameResource->Fence, eventHandle));
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}

	UpdateObjectCBs(gt);
	UpdatePassCB(gt);
}

void CrateApp::OnKeyboardInput(const GameTimer & gt)
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
}

void CrateApp::UpdateCamera(const GameTimer & gt)
{
	mEyePos.x = mRadius*sinf(mPhi)*cosf(mTheta);
	mEyePos.y = mRadius*cosf(mPhi);
	mEyePos.z = mRadius*sinf(mPhi)*sinf(mTheta);

	XMVECTOR pos = XMVectorSet(mEyePos.x, mEyePos.y, mEyePos.z, 1.0f);
	XMVECTOR target = XMVectorZero();
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMMATRIX viewMatrix = XMMatrixLookAtLH(pos, target, up);
	XMStoreFloat4x4(&mView, viewMatrix);
}

void CrateApp::UpdateObjectCBs(const GameTimer & gt)
{
	auto currObjCb = mCurrentFrameResource->ObjectCB.get();
	for (auto& rItem : mRenderItems) {
		if (rItem->NumFramesDirty > 0) {
			XMMATRIX w = XMLoadFloat4x4(&rItem->World);
			XMMATRIX tt = XMLoadFloat4x4(&rItem->TexTransform);
			ObjectConstants oc;
			XMStoreFloat4x4(&oc.World, w);
			XMStoreFloat4x4(&oc.TexTransform, tt);
			currObjCb->CopyData(rItem->ObjCBIndex, oc);
			rItem->NumFramesDirty--;
		}
	}

	// materials
	auto currMatCb = mCurrentFrameResource->MaterialCB.get();
	for (auto& materialTuple : mMaterials) {
		auto mat = materialTuple.second.get();
		if (mat->NumFramesDirty > 0) {
			XMMATRIX matTransform = XMLoadFloat4x4(&mat->MatTransform);
			MaterialConstants mc;
			XMStoreFloat4x4(&mc.MatTransform, matTransform);
			mc.DiffuseAlbedo = mat->DiffuseAlbedo;
			mc.FresnelR0 = mat->FresnelR0;
			mc.Roughness = mat->Roughness;
			currMatCb->CopyData(mat->MatCBIndex, mc);
			mat->NumFramesDirty--;
		}
	}
}

void CrateApp::UpdatePassCB(const GameTimer & gt)
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
	mMainPassCB.RenderTargetSize = XMFLOAT2((float) mClientWidth, (float) mClientHeight);
	mMainPassCB.InvRenderTargetSize = XMFLOAT2(1.0f / mClientWidth, 1.0f / mClientHeight);
	mMainPassCB.NearZ = 1.0f;
	mMainPassCB.FarZ = 1000.0f;
	mMainPassCB.TotalTime = gt.TotalTime();
	mMainPassCB.DeltaTime = gt.DeltaTime();

	XMVECTOR lightDir = -MathHelper::SphericalToCartesian(1.0f, mSunTheta, mSunPhi);
	XMStoreFloat3(&mMainPassCB.Lights[0].Direction, lightDir);
	lightDir = -MathHelper::SphericalToCartesian(1.0f, XM_PIDIV4, XM_PIDIV4);
	XMStoreFloat3(&mMainPassCB.Lights[1].Direction, lightDir);

	mMainPassCB.Lights[0].Strength = XMFLOAT3(1.0f, 1.0f, 0.9f);
	mMainPassCB.Lights[1].Strength = XMFLOAT3(1.0f, 0.2f, 0.2f);
	mCurrentFrameResource->PassCB->CopyData(0, mMainPassCB);
}

void CrateApp::AnimateFlares(const GameTimer & gt)
{
	auto currentCrate = mCrates[0];

	XMMATRIX mat = XMLoadFloat4x4(&currentCrate->TexTransform);
	XMMATRIX rotationZ = XMMatrixRotationZ(XMConvertToRadians(45.0f*gt.DeltaTime()));
	XMMATRIX next =  mat*rotationZ;
	XMStoreFloat4x4(&currentCrate->TexTransform, next);
	currentCrate->NumFramesDirty = mNumFrameResources;
}

void CrateApp::Draw(const GameTimer & gt)
{
	auto cmdAlloc = mCurrentFrameResource->CmdListAlloc;

	ThrowIfFailed(cmdAlloc->Reset());
	ThrowIfFailed(mCommandList->Reset(cmdAlloc.Get(), mPSOs["opaque"].Get()));
	// try resetting the command list


	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	// transition for RTV
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		CurrentBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	mCommandList->ClearRenderTargetView(CurrentBackBufferView(), DirectX::Colors::LightSteelBlue, 0, nullptr);
	mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
	mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

	ID3D12DescriptorHeap* heaps[1] = { mCbvHeap.Get() };
	mCommandList->SetDescriptorHeaps(_countof(heaps), heaps);

	mCommandList->SetGraphicsRootSignature(mRootSignature.Get());
	auto pcbh = CD3DX12_GPU_DESCRIPTOR_HANDLE(mCbvHeap->GetGPUDescriptorHandleForHeapStart());
	pcbh.Offset(mPassCbIndexOffset + mCurrentFrameIndex, mCbvSrvDescriptorSize);

	mCommandList->SetGraphicsRootDescriptorTable(2, pcbh);

	DrawRenderItems(mRenderItems);

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		CurrentBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT
	));

	ThrowIfFailed(mCommandList->Close());

	ID3D12CommandList* cmdLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);

	ThrowIfFailed(mSwapChain->Present(0, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;
	
	mCurrentFrameResource->Fence = ++mCurrentFence;
	mCommandQueue->Signal(mFence.Get(), mCurrentFence);
}

void CrateApp::DrawRenderItems(const std::vector<std::unique_ptr<RenderItem>>& rItems)
{
	auto sz = (UINT)rItems.size();
	auto matCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(MaterialConstants));
	
	auto mat = mCurrentFrameResource->MaterialCB->Resource();
	auto getOffsetedHandle = [&](UINT&& offset) {
		auto handle = CD3DX12_GPU_DESCRIPTOR_HANDLE(mCbvHeap->GetGPUDescriptorHandleForHeapStart());
		handle.Offset(offset, mCbvSrvDescriptorSize);
		return handle;
	};

	for (uint16_t i = 0; i < sz; ++i) {
		auto rItem = rItems[i].get();
		mCommandList->IASetVertexBuffers(0, 1, &rItem->Geo->VertexBufferView());
		mCommandList->IASetIndexBuffer(&rItem->Geo->IndexBufferView());
		mCommandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		mCommandList->SetGraphicsRootDescriptorTable(0, getOffsetedHandle(mCurrentFrameIndex * sz + i));

		mCommandList->SetGraphicsRootDescriptorTable(3, 
			getOffsetedHandle(mTexCIndexOffset + rItem->Mat->DiffuseSrvHeapIndex)); // <- diffuse map in root param index 3
		
		mCommandList->SetGraphicsRootDescriptorTable(4,
			getOffsetedHandle(mTexCIndexOffset + rItem->Mat->DiffuseSrvHeapIndex + 1));

		auto matAddress = mat->GetGPUVirtualAddress() + rItem->Mat->MatCBIndex * matCBByteSize;
		mCommandList->SetGraphicsRootConstantBufferView(1, matAddress);

		mCommandList->DrawIndexedInstanced(rItem->IndexCount, 1, rItem->StartIndexLocation, rItem->BaseVertexLocation, 0);
	}
}

void CrateApp::OnMouseDown(WPARAM btnState, int x, int y)
{
	mLastMousePos.x = x;
	mLastMousePos.y = y;
	SetCapture(mhMainWnd);
}

void CrateApp::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void CrateApp::OnMouseMove(WPARAM btnState, int x, int y)
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
		mRadius = MathHelper::Clamp(mRadius, 1.0f, 1000.0f);
	}

	mLastMousePos.x = x;
	mLastMousePos.y = y;
}


