#include "stdafx.h"
#include "InitDirect3D.h"

InitDirect3DApp::InitDirect3DApp(HINSTANCE hInstance) : D3DApp(hInstance) {}

InitDirect3DApp::~InitDirect3DApp() {}

bool InitDirect3DApp::Initialize() {
	if (!D3DApp::Initialize())
		return false;

	return true;
}

void InitDirect3DApp::OnResize() {
	D3DApp::OnResize();
}

void InitDirect3DApp::Update(const GameTimer& gt) {}

void InitDirect3DApp::Draw(const GameTimer& gt) {
	// reuse memory associated with command recording
	// we can only reset when the associated command lists have finished execution on the GPU
	ThrowIfFailed(mDirectCmdListAlloc->Reset());

	// Command list can be reset after it has been added to the command queue
	// via ExecuteCommandList.  Reusing the command list reuses memory
	ThrowIfFailed(mCommandList->Reset(
		mDirectCmdListAlloc.Get(), nullptr));

	// indicate a state trnasition
	mCommandList->ResourceBarrier(
		1, &CD3DX12_RESOURCE_BARRIER::Transition(
			CurrentBackBuffer(),
			D3D12_RESOURCE_STATE_PRESENT,
			D3D12_RESOURCE_STATE_RENDER_TARGET));

	// set the viewport and scissor rect.  This needs to be reset
	// whenever the command list is reset
	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	// clear the back buffer and depth buffer
	mCommandList->ClearRenderTargetView(
		CurrentBackBufferView(),
		DirectX::Colors::LightSteelBlue,
		0,
		nullptr // <- clear whole rectangle
	);
	mCommandList->ClearDepthStencilView(
		DepthStencilView(),
		D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
		1.0f,
		0,
		0,
		nullptr // <- clear whole rectangle
	);

	// specify buffers we are going to render to
	mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), // <- use 1 RTV
		true, // true if all RTVs in previous array are contiguous in descriptor heap, oetherwise false
		&DepthStencilView() // pointer to DSV that specifies the depth/stencil buffer we want to bind to the pipeline
	);

	// indicate a state transition on the resource usage
	mCommandList->ResourceBarrier(1,
		&CD3DX12_RESOURCE_BARRIER::Transition(
			CurrentBackBuffer(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PRESENT
		));

	// done recording commands
	ThrowIfFailed(mCommandList->Close());

	// add command list to queue for execution
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// swap back and front buffers
	ThrowIfFailed(mSwapChain->Present(0, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

	// wait until frame command are complete. 
	FlushCommandQueue();
}