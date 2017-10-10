#include "stdafx.h"
#include "BoxApp.h"
#include "Pyramid.h"
#include "Box.h"
#include <array>


BoxApp::BoxApp(HINSTANCE hInstance) : D3DApp(hInstance) {}

BoxApp::~BoxApp() {}

bool BoxApp::Initialize() {
	if (!D3DApp::Initialize()) {
		return false;
	}

	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	BuildDescriptorHeaps();
	BuildConstantBuffers();
	BuildRootSignature();
	BuildShaders();
	// BuildBoxIndices();
	BuildBoxGeometry();
	// BuildBoxPositions();
	// BuildBoxColors();
	// BuildPyramidGeometry();
	BuildPSO();

	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	FlushCommandQueue();

	return true;
}

void BoxApp::OnMouseDown(WPARAM btnState, int x, int y) {
	mLastMousePos.x = x;
	mLastMousePos.y = y;

	SetCapture(mhMainWnd);
}

void BoxApp::OnMouseUp(WPARAM btnState, int x, int y) {
	ReleaseCapture();
}

void BoxApp::OnMouseMove(WPARAM btnState, int x, int y) {
	// left button click
	if ((btnState & MK_LBUTTON) != 0) {
		// make each pixel correspond to quarter of a degree
		float dx = XMConvertToRadians(0.25f*static_cast<float>(x - mLastMousePos.x));
		float dy = XMConvertToRadians(0.25f*static_cast<float>(y - mLastMousePos.y));

		// update angles based on input to orbit around box
		mTheta += dx; // <- 360 degrees freedom
		mPhi += dy;

		// restrict angle theta (180 degrees of freedom)
		mPhi = MathHelper::Clamp(mPhi, 0.1f, MathHelper::Pi - 0.1f);
	}
	else if ((btnState & MK_RBUTTON) != 0) {
		float dx = 0.005f*static_cast<float>(x - mLastMousePos.x);
		float dy = 0.005f*static_cast<float>(y - mLastMousePos.y);

		// update camera radius based on input
		mRadius += dx - dy;
		mRadius = MathHelper::Clamp(mRadius, 3.0f, 5.0f);
	}
	mLastMousePos.x = x;
	mLastMousePos.y = y;
}

void BoxApp::OnResize() {
	D3DApp::OnResize();

	// the window resized, so update the aspect ratio and recompute the projection matrix
	XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f*MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
	XMStoreFloat4x4(&mProj, P);
}

void BoxApp::Update(const GameTimer& gt) {
	// convert spherical to cartesian
	float x = mRadius*sinf(mPhi)*cosf(mTheta);
	float z = mRadius*sinf(mPhi)*sinf(mTheta);
	float y = mRadius*cosf(mPhi);

	// build view matrix
	XMVECTOR pos = XMVectorSet(x, y, z, 1.0f);
	XMVECTOR target = XMVectorZero(); // XMVectorSet(0.0f, (1.0f / 6.0f)*XM_PI, 0.0f, 0.0f);
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
	XMStoreFloat4x4(&mView, view);

	XMMATRIX world = XMLoadFloat4x4(&mWorld);
	XMMATRIX proj = XMLoadFloat4x4(&mProj);
	XMMATRIX worldViewProj = world*view*proj;

	// update constant buffer with latest matrix
	ObjectConstants objConstants;
	XMStoreFloat4x4(&objConstants.WorldViewProj, XMMatrixTranspose(worldViewProj));
	mObjectCB->CopyData(0, objConstants);
}

void BoxApp::Draw(const GameTimer& gt) {
	ThrowIfFailed(mDirectCmdListAlloc->Reset());

	// reset the command list
	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), mPSO.Get()));

	// set viewports and scissor rects
	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	// indicate state transition
	mCommandList->ResourceBarrier(1,
		&CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
			D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	// clear back buffer and depth buffer
	mCommandList->ClearRenderTargetView(CurrentBackBufferView(),
		Colors::LightSteelBlue, 0, nullptr);
	mCommandList->ClearDepthStencilView(DepthStencilView(),
		D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
		1.0f, 0, 0, nullptr);

	// set the render targets
	mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

	// set the descriptor heaps, root sig, vertex buffers, and descriptor table
	ID3D12DescriptorHeap* descriptorHeaps[] = { mCbvHeap.Get() };
	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

	// D3D12_VERTEX_BUFFER_VIEW vBufferViews[] = { mBoxPosData->VertexBufferView(), mBoxColorData->VertexBufferView() };
	mCommandList->IASetVertexBuffers(0, 1, &mBoxGeo->VertexBufferView());
	mCommandList->IASetIndexBuffer(&mBoxGeo->IndexBufferView());
	mCommandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	mCommandList->SetGraphicsRootDescriptorTable(0, mCbvHeap->GetGPUDescriptorHandleForHeapStart());
	mCommandList->DrawIndexedInstanced(
		mBoxGeo->DrawArgs[Box::GeometryName].IndexCount,
		1, 0, 0, 0
	);

	// indicate a state transition 
	mCommandList->ResourceBarrier(1,
		&CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	ThrowIfFailed(mCommandList->Close());

	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// swap back and front buffers
	ThrowIfFailed(mSwapChain->Present(0, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

	FlushCommandQueue();
}

void BoxApp::BuildDescriptorHeaps() {

	D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;
	cbvHeapDesc.NumDescriptors = 1;
	cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	cbvHeapDesc.NodeMask = 0;

	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&mCbvHeap)));
}

void BoxApp::BuildConstantBuffers() {
	// wraps the upload buffer in a unique pointer and forwards constants to UploadBuffer constructor
	// d3dDevice pointer, # elements in constant buffer, isConstantBuffer = true
	mObjectCB = std::make_unique<UploadBuffer<ObjectConstants>>(md3dDevice.Get(), 1, true);

	UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));

	D3D12_GPU_VIRTUAL_ADDRESS cbAddress = mObjectCB->Resource()->GetGPUVirtualAddress();

	int boxCBufIndex = 0; // <- no offset
	cbAddress += boxCBufIndex*objCBByteSize;

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
	cbvDesc.BufferLocation = cbAddress;
	cbvDesc.SizeInBytes = objCBByteSize;

	md3dDevice->CreateConstantBufferView(&cbvDesc, mCbvHeap->GetCPUDescriptorHandleForHeapStart());
}

void BoxApp::BuildRootSignature() {
	// 1 root parameter
	CD3DX12_ROOT_PARAMETER slotRootParameter[1];

	// descriptor table describes contiguous range of descriptors in a descriptor heap
	CD3DX12_DESCRIPTOR_RANGE cbvTable;
	cbvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
	slotRootParameter[0].InitAsDescriptorTable(1, &cbvTable); // <- note all shaders can see this

	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(1, slotRootParameter, 0, nullptr,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;

	HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

	if (errorBlob != nullptr) {
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	ThrowIfFailed(hr);

	ThrowIfFailed(md3dDevice->CreateRootSignature(0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(&mRootSignature)));
}

void BoxApp::BuildShaders() {
	HRESULT hr = S_OK;

	mvsByteCode = d3dUtil::CompileShader(L"colors.hlsl", nullptr, "VS", "vs_5_0");
	mpsByteCode = d3dUtil::CompileShader(L"colors.hlsl", nullptr, "PS", "ps_5_0");
}

void BoxApp::BuildBoxIndices() {
	mBoxIndices = {
		// font face
		0, 1, 2,
		0, 2, 3,

		// back face
		4, 6, 5,
		4, 7, 6,

		// left face
		4, 5, 1,
		4, 1, 0,

		// right face
		3, 2, 6,
		3, 6, 7,

		// bottom face
		4, 0, 3,
		4, 3, 7,

		// top face
		1, 5, 6,
		1, 6, 2
	};

	mibByteSize = (UINT)mBoxIndices.size() * sizeof(std::uint16_t);
}

void BoxApp::BuildBoxGeometry() {
	Box box(md3dDevice, mCommandList);
	mBoxIndices = box.GetIndexList();
	mBoxGeo = box.GetGeometry();
	mInputLayout = box.GetInputElementLayout();
}

void BoxApp::BuildBoxPositions() {

	mBoxPosData = std::make_unique<MeshGeometry>();
	mBoxPosData->Name = "BoxPositions";

	std::array<VPosData, 8> posVertices = {
		VPosData({ XMFLOAT3(-1.0f, -1.0f, -1.0f) }),
		VPosData({ XMFLOAT3(-1.0f, +1.0f, -1.0f) }),
		VPosData({ XMFLOAT3(+1.0f, +1.0f, -1.0f) }),
		VPosData({ XMFLOAT3(+1.0f, -1.0f, -1.0f) }),
		VPosData({ XMFLOAT3(-1.0f, -1.0f, +1.0f) }),
		VPosData({ XMFLOAT3(-1.0f, +1.0f, +1.0f) }),
		VPosData({ XMFLOAT3(+1.0f, +1.0f, +1.0f) }),
		VPosData({ XMFLOAT3(+1.0f, -1.0f, +1.0f) }),
	};

	const UINT vbByteSize = (UINT)posVertices.size() * sizeof(VPosData);

	// allocate memory to vertex buffer CPU
	ThrowIfFailed(D3DCreateBlob(vbByteSize, &mBoxPosData->VertexBufferCPU));
	CopyMemory(mBoxPosData->VertexBufferCPU->GetBufferPointer(), posVertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(mibByteSize, &mBoxPosData->IndexBufferCPU));
	CopyMemory(mBoxPosData->IndexBufferCPU->GetBufferPointer(), mBoxIndices.data(), mibByteSize);


	// Load data from system-memory -> CPU_UPLOAD_HEAP -> GPU_UPLOAD_HEAP (the vertex and index data are constant and only need be loaded once)
	// the ID3D12Resource here is a default buffer
	mBoxPosData->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(), mCommandList.Get(), posVertices.data(), vbByteSize,
		mBoxPosData->VertexBufferUploader);
	mBoxPosData->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(), mCommandList.Get(), mBoxIndices.data(), mibByteSize,
		mBoxPosData->IndexBufferUploader);

	mBoxPosData->VertexByteStride = sizeof(VPosData);
	mBoxPosData->VertexBufferByteSize = vbByteSize;
	mBoxPosData->IndexFormat = DXGI_FORMAT_R16_UINT;
	mBoxPosData->IndexBufferByteSize = mibByteSize;

	SubmeshGeometry submesh;
	submesh.BaseVertexLocation = 0;
	submesh.StartIndexLocation = 0;
	submesh.IndexCount = (UINT)mBoxIndices.size();
	mBoxPosData->DrawArgs["box"] = submesh;
}

void BoxApp::BuildBoxColors() {

	mBoxColorData = std::make_unique<MeshGeometry>();
	mBoxColorData->Name = "BoxColor";

	std::array<VColorData, 8> colorVertices = {
		VColorData({ XMFLOAT4(Colors::White) }),
		VColorData({ XMFLOAT4(Colors::Black) }),
		VColorData({ XMFLOAT4(Colors::Red) }),
		VColorData({ XMFLOAT4(Colors::Green) }),
		VColorData({ XMFLOAT4(Colors::Blue) }),
		VColorData({ XMFLOAT4(Colors::Yellow) }),
		VColorData({ XMFLOAT4(Colors::Cyan) }),
		VColorData({ XMFLOAT4(Colors::Magenta) })
	};

	const UINT vbByteSize = (UINT)colorVertices.size() * sizeof(VColorData);
	
	D3DCreateBlob(vbByteSize, &mBoxColorData->VertexBufferCPU);
	CopyMemory(mBoxColorData->VertexBufferCPU->GetBufferPointer(), colorVertices.data(), vbByteSize);

	D3DCreateBlob(mibByteSize, &mBoxColorData->IndexBufferCPU);
	CopyMemory(mBoxColorData->IndexBufferCPU->GetBufferPointer(), mBoxIndices.data(), mibByteSize);

	// Load data from system-memory -> CPU_UPLOAD_HEAP -> GPU_UPLOAD_HEAP (the vertex and index data are constant and only need be loaded once)
	// the ID3D12Resource here is a default buffer
	mBoxColorData->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(), mCommandList.Get(), colorVertices.data(), vbByteSize,
		mBoxPosData->VertexBufferUploader);
	mBoxColorData->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(), mCommandList.Get(), mBoxIndices.data(), mibByteSize,
		mBoxColorData->IndexBufferUploader);

	mBoxColorData->IndexBufferByteSize = mibByteSize;
	mBoxColorData->VertexBufferByteSize = vbByteSize;
	mBoxColorData->VertexByteStride = sizeof(VColorData);

	SubmeshGeometry submesh;
	submesh.BaseVertexLocation = 0;
	submesh.IndexCount = colorVertices.size();
	submesh.StartIndexLocation = 0;
	
	mBoxColorData->DrawArgs["BoxColors"] = submesh;
}

void BoxApp::BuildPyramidGeometry() {
	Pyramid pyramid(md3dDevice, mCommandList);
	mInputLayout = pyramid.GetInputLayoutDescription();
	mBoxGeo = pyramid.GetGeometry();
}

void BoxApp::BuildPSO() {
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
	ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	psoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
	psoDesc.pRootSignature = mRootSignature.Get();
	psoDesc.VS = {
		reinterpret_cast<BYTE*>(mvsByteCode->GetBufferPointer()),
		mvsByteCode->GetBufferSize()
	};

	psoDesc.PS = {
		reinterpret_cast<BYTE*>(mpsByteCode->GetBufferPointer()),
		mpsByteCode->GetBufferSize()
	};

	auto rasterizer = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	rasterizer.FillMode = D3D12_FILL_MODE_WIREFRAME;
	// rasterizer.CullMode = D3D12_CULL_MODE_FRONT;
	psoDesc.RasterizerState = rasterizer;
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = mBackBufferFormat;
	psoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	psoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	psoDesc.DSVFormat = mDepthStencilFormat;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPSO)));
}