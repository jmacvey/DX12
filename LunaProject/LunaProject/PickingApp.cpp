#include "stdafx.h"
#include "PickingApp.h"

PickingApp::PickingApp(HINSTANCE hInstance) : D3DApp(hInstance)
{
	mCamera = std::make_unique<Camera>();
}

bool PickingApp::Initialize()
{
	if (!D3DApp::Initialize()) {
		return false;
	}

	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	BuildGeometry();
	BuildRenderItems();
	BuildFrameResources();
	BuildDescriptorHeaps();
	BuildInstanceDescriptors();
	BuildDescriptors();
	CompileShadersAndInputLayout();
	BuildRootSignature();
	BuildPSOs();

	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* commandLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

	FlushCommandQueue();
	return true;
}

void PickingApp::BuildGeometry()
{

	std::vector<Vertex> vertices;
	auto carVertices = mCar.GetVertices();

	vertices.resize(carVertices.size());

	for (UINT i = 0; i < (UINT)carVertices.size(); ++i) {
		vertices[i] = { carVertices[i].Position };
	}

	auto indices = mCar.GetIndices();

	UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint32_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "Car";
	geo->IndexBufferByteSize = ibByteSize;
	geo->VertexBufferByteSize = vbByteSize;
	geo->VertexByteStride = sizeof(Vertex);
	geo->IndexFormat = DXGI_FORMAT_R32_UINT;

	// allocate memory
	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));

	// copy memory
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	// create buffers
	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(), mCommandList.Get(),
		vertices.data(), vbByteSize, geo->VertexBufferUploader);
	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(), mCommandList.Get(),
		indices.data(), ibByteSize, geo->IndexBufferUploader);

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.BaseVertexLocation = 0;
	submesh.StartIndexLocation = 0;
	submesh.Bounds = mCar.GetBoundingBox();

	geo->DrawArgs["Car"] = submesh;

	mGeometries[geo->Name] = std::move(geo);
}

void PickingApp::BuildRenderItems()
{
	auto rItem = std::make_unique<RenderItem>();
	rItem->Geo = mGeometries["Car"].get();
	rItem->PrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	rItem->IndexCount = rItem->Geo->DrawArgs["Car"].IndexCount;
	rItem->StartIndexLocation = rItem->Geo->DrawArgs["Car"].StartIndexLocation;
	rItem->BaseVertexLocation = rItem->Geo->DrawArgs["Car"].BaseVertexLocation;
	rItem->InstanceCount = 1;
	rItem->Bounds = rItem->Geo->DrawArgs["Car"].Bounds;
	BuildCarInstance(rItem.get());

	mRenderLayer[(UINT)RenderLayers::Cars].emplace_back(rItem.get());
	mAllRenderItems.emplace_back(std::move(rItem));

	rItem = std::make_unique<RenderItem>();
	rItem->Geo = mGeometries["Car"].get();
	rItem->PrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	rItem->IndexCount = 3;
	rItem->StartIndexLocation = 0;
	rItem->BaseVertexLocation = 0;
	rItem->InstanceCount = 1;
	rItem->Bounds = rItem->Geo->DrawArgs["Car"].Bounds;
	rItem->Visible = false;
	BuildCarInstance(rItem.get());

	mPickedTriangle = rItem.get();
	mAllRenderItems.emplace_back(std::move(rItem));
}

void PickingApp::BuildDescriptorHeaps()
{
	auto numRenderItems = mAllRenderItems.size();
	mPassCbOffset = (int)(numRenderItems * mNumFrameResources);
	auto numDescriptors = mPassCbOffset + mNumFrameResources;
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heapDesc.NumDescriptors = (UINT)numDescriptors;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heapDesc.NodeMask = 0u;

	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&mCbvSrvHeap)));
}

void PickingApp::BuildDescriptors()
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
}

void PickingApp::BuildCarInstance(RenderItem* rItem)
{
	rItem->Instances.resize(rItem->InstanceCount);
	rItem->Instances[0].World = MathHelper::Identity4x4();
}

void PickingApp::BuildInstanceDescriptors()
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};

	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.NumElements = mRenderLayer[(UINT)RenderLayers::Cars][0]->Instances.size();
	srvDesc.Buffer.StructureByteStride = sizeof(InstanceData);
	srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

	for (UINT i = 0; i < mNumFrameResources; ++i) {
		auto res = mFrameResources[i]->InstanceCB->Resource();
		md3dDevice->CreateShaderResourceView(res, &srvDesc,
			CD3DX12_CPU_DESCRIPTOR_HANDLE(mCbvSrvHeap->GetCPUDescriptorHandleForHeapStart(), i, mCbvSrvDescriptorSize));
	}

}

void PickingApp::BuildMaterials()
{
	auto mat = std::make_unique<Material>(mNumFrameResources);
	mat->MatCBIndex = 0;
	mat->DiffuseAlbedo = { 0.2f, 0.8f, 0.2f, 1.0f };
	mat->Name = "grass";
	mMaterials[mat->Name] = std::move(mat);
}

void PickingApp::CompileShadersAndInputLayout()
{
	mShaders["BasicRenderVS"] = d3dUtil::CompileShader(L"Shaders\\BasicRender.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["BasicRenderPS"] = d3dUtil::CompileShader(L"Shaders\\BasicRender.hlsl", nullptr, "PS", "ps_5_1");

	mInputLayout = {
		{ "POSITION", 0u, DXGI_FORMAT_R32G32B32_FLOAT, 0u, 0u, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0u }
	};
}

void PickingApp::BuildRootSignature()
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

void PickingApp::BuildPSOs()
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
	desc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
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
}

void PickingApp::BuildFrameResources()
{
	for (UINT i = 0; i < (UINT)mNumFrameResources; ++i) {
		mFrameResources.emplace_back(std::make_unique<FrameResource>(
			md3dDevice.Get(), 1, (UINT)mRenderLayer[(UINT)RenderLayers::Cars][0]->Instances.size(), (UINT)mMaterials.size()));
	}
}

void PickingApp::Update(const GameTimer & gt)
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

void PickingApp::Draw(const GameTimer & gt)
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

	mCommandList->SetGraphicsRootSignature(mRootSignature.Get());
	ID3D12DescriptorHeap* descriptorHeaps[1] = { mCbvSrvHeap.Get() };

	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	mCommandList->SetGraphicsRootDescriptorTable(1,
		CD3DX12_GPU_DESCRIPTOR_HANDLE(mCbvSrvHeap->GetGPUDescriptorHandleForHeapStart(),
			mPassCbOffset + mCurrFrameIndex, mCbvSrvDescriptorSize));

	DrawRenderItems(mRenderLayer[(UINT)RenderLayers::Cars]);
	TryDrawPickedTriangle();
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

void PickingApp::DrawRenderItems(const std::vector<RenderItem*>& renderItems)
{
	mCommandList->SetGraphicsRootDescriptorTable(0,
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

void PickingApp::TryDrawPickedTriangle()
{
	if (mPickedTriangle->Visible) {
		mCommandList->SetPipelineState(mPSOs["opaque"].Get());
		mCommandList->IASetVertexBuffers(0, 1, &mPickedTriangle->Geo->VertexBufferView());
		mCommandList->IASetIndexBuffer(&mPickedTriangle->Geo->IndexBufferView());
		mCommandList->IASetPrimitiveTopology(mPickedTriangle->PrimitiveTopology);
		mCommandList->DrawIndexedInstanced(mPickedTriangle->IndexCount, mPickedTriangle->InstanceCount,
			mPickedTriangle->StartIndexLocation, mPickedTriangle->BaseVertexLocation, 0);
	}
}

void PickingApp::UpdateInstances(const GameTimer & gt)
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

				if (localFrustum.Contains(renderItem->Bounds) != DirectX::DISJOINT) {
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

void PickingApp::UpdateMainPassCB(const GameTimer & gt)
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

void PickingApp::MarkRenderItemsDirty()
{
	for (auto& rItem : mAllRenderItems) {
		rItem->NumFramesDirty++;
	}
}

void PickingApp::OnMouseDown(WPARAM btnState, int x, int y)
{
	SetCapture(mhMainWnd);
	mLastMousePos.x = x;
	mLastMousePos.y = y;
}

void PickingApp::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
	Pick(x, y);
}

void PickingApp::OnMouseMove(WPARAM btnState, int x, int y)
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

void PickingApp::OnResize()
{
	D3DApp::OnResize();
	mCamera->SetLens(0.25f*MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
}

void PickingApp::OnKeyboardInput(const GameTimer & gt)
{
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
}

void PickingApp::Pick(int sx, int sy)
{
	XMFLOAT4X4 P = mCamera->GetProj4x4f();
	float vx = (+2.0f*sx / mClientWidth - 1.0f) / P(0, 0);
	float vy = (-2.0f*sy / mClientHeight + 1.0f) / P(1, 1);

	// ray props

	mPickingRayDir.x = vx;
	mPickingRayDir.y = vy;

	// transform ray to local space
	XMMATRIX V = mCamera->View();
	XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(V), V);

	mPickedTriangle->Visible = false;
	for (auto& rItem : mRenderLayer[(UINT)RenderLayers::Cars]) {
		for (auto& instance : rItem->Instances) {
			
			if (!rItem->Visible) {
				continue;
			}
			XMVECTOR rayOrigin = XMLoadFloat4(&mPickingRayOrigin);
			XMVECTOR rayDir = XMLoadFloat4(&mPickingRayDir);
			XMMATRIX W = XMLoadFloat4x4(&instance.World);
			XMMATRIX invWorld = XMMatrixInverse(&XMMatrixDeterminant(W), W);
			XMMATRIX toLocal = XMMatrixMultiply(invView, invWorld);
			rayOrigin = XMVector3TransformCoord(rayOrigin, toLocal);
			rayDir = XMVector3TransformNormal(rayDir, toLocal);
			
			// normalize the ray direction
			rayDir = XMVector3Normalize(rayDir);
		
			auto tmin = 0.0f;
			if (rItem->Bounds.Intersects(rayOrigin, rayDir, tmin)) {
				::OutputDebugStringW(L"Intersected bounding box\n");

				auto vertices = (Vertex*)rItem->Geo->VertexBufferCPU->GetBufferPointer();
				auto indices = (std::uint32_t*)rItem->Geo->IndexBufferCPU->GetBufferPointer();
				
				UINT numTriangles = rItem->IndexCount / 3;
				tmin = MathHelper::Infinity;
				bool foundTriangle = false;
				for (UINT i = 0; i < numTriangles; ++i) {
					UINT i0 = indices[i * 3 + 0];
					UINT i1 = indices[i * 3 + 1];
					UINT i2 = indices[i * 3 + 2];

					XMVECTOR v0 = XMLoadFloat3(&vertices[i0].Position);
					XMVECTOR v1 = XMLoadFloat3(&vertices[i1].Position);
					XMVECTOR v2 = XMLoadFloat3(&vertices[i2].Position);
					auto t = 0.0f;
					if (TriangleTests::Intersects(rayOrigin, rayDir, v0, v1, v2, t)) {
						if (t < tmin) {
							tmin = t;
							UINT pickedTriangle = i;

							mPickedTriangle->IndexCount = 3;
							mPickedTriangle->StartIndexLocation = pickedTriangle * 3;
							mPickedTriangle->Visible = true;
							mPickedTriangle->NumFramesDirty = mNumFrameResources;
							mPickedTriangle->BaseVertexLocation = 0;
						}
					}
				}
				if (foundTriangle) {
					::OutputDebugStringW(L"Found Triangle!\n");
				}
			}
		}
	}

}
