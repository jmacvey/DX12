#include "stdafx.h"
#include "LitWaves.h"


LitWavesApp::LitWavesApp(HINSTANCE hInstance) : D3DApp(hInstance)
{
}

LitWavesApp::~LitWavesApp()
{
	if (md3dDevice != nullptr) {
		FlushCommandQueue();
	}
}

bool LitWavesApp::Initialize()
{
	if (!D3DApp::Initialize()) {
		return false;
	}

	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	mCbvSrvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	mWaves = std::make_unique<Waves>(128, 128, 1.0f, 0.03f, 4.0f, 0.2f);

	// handles building textures resources, descriptor heap, and descriptors
	BuildTextures();
	BuildDescriptorHeaps();
	BuildTextureDescriptors();

	// handles everything else
	BuildRootSignature();
	BuildShadersAndInputLayout();
	BuildDepthVisualizationQuads();
	BuildLandGeometry();
	BuildTreeSpriteGeometry();
	BuildWavesGeometryBuffers();

	mSubdivider = std::make_unique<Subdivider>(mCommandList, md3dDevice);
	mSubdivider->BuildGeometry();

	BuildMaterials();
	BuildRenderItems();
	BuildFrameResources();
	BuildPSOs();

	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	FlushCommandQueue();
	return true;
}

void LitWavesApp::OnResize()
{
	D3DApp::OnResize();
	XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f*MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
	XMStoreFloat4x4(&mProj, P);
}

void LitWavesApp::Update(const GameTimer & gt)
{
	OnKeyboardInput(gt);
	UpdateCamera(gt);
	AnimateMaterials(gt);

	// Cycle through the circular frame resource array.
	mCurrFrameResourceIndex = (mCurrFrameResourceIndex + 1) % mNumFrameResources;
	mCurrFrameResource = mFrameResources[mCurrFrameResourceIndex].get();

	// Has the GPU finished processing the commands of the current frame resource?
	// If not, wait until the GPU has completed commands up to this fence point.
	if (mCurrFrameResource->Fence != 0 && mFence->GetCompletedValue() < mCurrFrameResource->Fence)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
		ThrowIfFailed(mFence->SetEventOnCompletion(mCurrFrameResource->Fence, eventHandle));
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}

	UpdateWaves(gt);
	UpdateLightning(gt);
	UpdateObjectCBs(gt);
	UpdateMaterialCBs(gt);
	UpdateMainPassCB(gt);
	UpdateDepthVisualizers(gt);
}

void LitWavesApp::Draw(const GameTimer & gt)
{
	auto cmdListAlloc = mCurrFrameResource->CmdListAlloc;

	// Reuse the memory associated with command recording.
	// We can only reset when the associated command lists have finished execution on the GPU.
	ThrowIfFailed(cmdListAlloc->Reset());

	// A command list can be reset after it has been added to the command queue via ExecuteCommandList.
	// Reusing the command list reuses memory.

	if (mBlendVisualizerEnabled) {
		ThrowIfFailed(mCommandList->Reset(cmdListAlloc.Get(), mPSOs["blendDepthVisualizer"].Get()));
	}
	else {
		ThrowIfFailed(mCommandList->Reset(cmdListAlloc.Get(), mPSOs["opaque"].Get()));
	}


	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	// Indicate a state transition on the resource usage.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	// Clear the back buffer and depth buffer.
	mCommandList->ClearRenderTargetView(CurrentBackBufferView(), (float*)&mMainPassCB.FogColor, 0, nullptr);
	mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	// Specify the buffers we are going to render to.
	mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

	ID3D12DescriptorHeap* descriptorHeaps[1] = { mSrvHeap.Get() };
	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

	auto passCB = mCurrFrameResource->PassCB->Resource();
	mCommandList->SetGraphicsRootConstantBufferView(2, passCB->GetGPUVirtualAddress());
	
	auto srvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(mSrvHeap->GetGPUDescriptorHandleForHeapStart());
	srvHandle.Offset(64, mCbvSrvDescriptorSize);

	mCommandList->SetGraphicsRootDescriptorTable(4, srvHandle);

	DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::Opaque]);

	if (mBlendVisualizerEnabled) {
		DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::AlphaTested]);
		DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::Blended]);
		DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::Additive]);
	}
	else {
		mCommandList->SetPipelineState(mPSOs["explosion"].Get());
		DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::OptimizedSphere]);

		mCommandList->SetPipelineState(mPSOs["alphaTested"].Get());
		DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::AlphaTested]);

		mCommandList->SetPipelineState(mPSOs["transparent"].Get());
		DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::Blended]);

		mCommandList->SetPipelineState(mPSOs["additive"].Get());
		DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::Additive]);

		mCommandList->SetPipelineState(mPSOs["treeArray"].Get());
		DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::AlphaTestedTreeSprites]);
	}

	if (mNormalVisualizerEnabled) {
		mCommandList->SetPipelineState(mPSOs["normalVisualizer"].Get());
		DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::Opaque]);
		DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::AlphaTested]);
		//DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::Blended]);
		DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::Additive]);
		DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::OptimizedSphere]);
	}

	if (mDepthVisualizerEnabled) {
		mCommandList->SetPipelineState(mPSOs["depthVisualizer"].Get());
		DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::DepthVisualizer], true);
	}

	// Indicate a state transition on the resource usage.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	// Done recording commands.
	ThrowIfFailed(mCommandList->Close());

	// Add the command list to the queue for execution.
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// Swap the back and front buffers
	ThrowIfFailed(mSwapChain->Present(0, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

	// Advance the fence value to mark commands up to this fence point.
	mCurrFrameResource->Fence = ++mCurrentFence;

	// Add an instruction to the command queue to set a new fence point. 
	// Because we are on the GPU timeline, the new fence point won't be 
	// set until the GPU finishes processing all the commands prior to this Signal().
	mCommandQueue->Signal(mFence.Get(), mCurrentFence);

}

void LitWavesApp::OnMouseDown(WPARAM btnState, int x, int y)
{
	mLastMousePos.x = x;
	mLastMousePos.y = y;
	SetCapture(mhMainWnd);
}

void LitWavesApp::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void LitWavesApp::OnMouseMove(WPARAM btnState, int x, int y)
{
	if ((btnState & MK_LBUTTON) != 0)
	{
		// Make each pixel correspond to a quarter of a degree.
		float dx = XMConvertToRadians(0.25f*static_cast<float>(x - mLastMousePos.x));
		float dy = XMConvertToRadians(0.25f*static_cast<float>(y - mLastMousePos.y));

		// Update angles based on input to orbit camera around box.
		mTheta += dx;
		mPhi += dy;

		// Restrict the angle mPhi.
		mPhi = MathHelper::Clamp(mPhi, 0.1f, MathHelper::Pi - 0.1f);
	}
	else if ((btnState & MK_RBUTTON) != 0)
	{
		// Make each pixel correspond to 0.2 unit in the scene.
		float dx = 0.2f*static_cast<float>(x - mLastMousePos.x);
		float dy = 0.2f*static_cast<float>(y - mLastMousePos.y);

		// Update the camera radius based on input.
		mRadius += dx - dy;

		// Restrict the radius.
		mRadius = MathHelper::Clamp(mRadius, 5.0f, 1000.0f);
	}

	mLastMousePos.x = x;
	mLastMousePos.y = y;
}

void LitWavesApp::OnKeyboardInput(const GameTimer & gt)
{
	const float dt = gt.DeltaTime();
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
		mSunPhi += 1.0f * dt;
	}

	if (GetAsyncKeyState('V') & 0x8000) {
		mDepthVisualizerEnabled = true;
		mBlendVisualizerEnabled = false;
	}

	if (GetAsyncKeyState('O') & 0x8000) {
		mDepthVisualizerEnabled = false;
		mBlendVisualizerEnabled = false;
		mNormalVisualizerEnabled = false;
	}

	if (GetAsyncKeyState('B') & 0x8000) {
		mDepthVisualizerEnabled = false;
		mBlendVisualizerEnabled = true;
	}

	if (GetAsyncKeyState('N') & 0x8000) {
		mNormalVisualizerEnabled = true;
	}

	mSunPhi = MathHelper::Clamp(mSunPhi, 0.1f, XM_PIDIV2);
}

void LitWavesApp::UpdateCamera(const GameTimer & gt)
{
	// Convert Spherical to Cartesian coordinates.
	mEyePos.x = mRadius*sinf(mPhi)*cosf(mTheta);
	mEyePos.z = mRadius*sinf(mPhi)*sinf(mTheta);
	mEyePos.y = mRadius*cosf(mPhi);

	// Build the view matrix.
	XMVECTOR pos = XMVectorSet(mEyePos.x, mEyePos.y, mEyePos.z, 1.0f);
	XMVECTOR target = XMVectorZero();
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
	XMStoreFloat4x4(&mView, view);
}

void LitWavesApp::UpdateDepthVisualizers(const GameTimer& gt)
{
	for (auto& quad : mRitemLayer[(UINT)RenderLayer::DepthVisualizer]) {

		// rotate the quad to always be aligned with camera
		auto translationVector = 0.95f*mRadius*XMVector3Normalize(XMLoadFloat3(&mEyePos));
		XMFLOAT3 T = {};
		XMStoreFloat3(&T, translationVector);
		auto quadPosition = XMMatrixScaling(mClientWidth / 2.0f, mClientHeight / 2.0f, 1.0f)*
			XMMatrixRotationX(XM_PIDIV2 - mPhi)*
			XMMatrixRotationY(-XM_PIDIV2 - mTheta)*XMMatrixTranslation(T.x, T.y, T.z);
		XMStoreFloat4x4(&quad->World, quadPosition);
		quad->NumFramesDirty = mNumFrameResources;
	}
}

void LitWavesApp::UpdateObjectCBs(const GameTimer& gt)
{
	auto currObjectCB = mCurrFrameResource->ObjectCB.get();
	for (auto& rItem : mAllRItems) {

		if (rItem->NumFramesDirty > 0) {
			XMMATRIX world = XMLoadFloat4x4(&rItem->World);
			XMMATRIX texTransform = XMLoadFloat4x4(&rItem->TexTransform);

			ObjectConstants objConstants;
			XMStoreFloat4x4(&objConstants.World, XMMatrixTranspose(world));
			XMStoreFloat4x4(&objConstants.TexTransform, XMMatrixTranspose(texTransform));
			auto inverseWorld = XMMatrixInverse(&XMMatrixDeterminant(world), world);
			XMStoreFloat4x4(&objConstants.WorldInvTranspose, XMMatrixTranspose(inverseWorld));
			objConstants.Color = rItem->Color;
			currObjectCB->CopyData(rItem->ObjCBIndex, objConstants);
			rItem->NumFramesDirty--;
		}
	}
}

void LitWavesApp::UpdateMaterialCBs(const GameTimer & gt)
{
	auto currMaterialCB = mCurrFrameResource->MaterialCB.get();
	for (auto& material : mMaterials) {
		Material* mat = material.second.get();
		if (mat->NumFramesDirty > 0) {
			XMMATRIX matTransform = XMLoadFloat4x4(&mat->MatTransform);
			MaterialConstants matConstants;
			matConstants.DiffuseAlbedo = mat->DiffuseAlbedo;
			matConstants.FresnelR0 = mat->FresnelR0;
			matConstants.Roughness = mat->Roughness;
			XMStoreFloat4x4(&matConstants.MatTransform, XMMatrixTranspose(matTransform));
			currMaterialCB->CopyData(mat->MatCBIndex, matConstants);
			mat->NumFramesDirty--;
		}
	}
}

void LitWavesApp::UpdateMainPassCB(const GameTimer & gt)
{
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
	XMStoreFloat4x4(&mMainPassCB.InvViewProj, XMMatrixTranspose(invViewProj));
	mMainPassCB.EyePosW = mEyePos;
	mMainPassCB.RenderTargetSize = XMFLOAT2((float)mClientWidth, (float)mClientHeight);
	mMainPassCB.InvRenderTargetSize = XMFLOAT2(1.0f / mClientWidth, 1.0f / mClientHeight);
	mMainPassCB.NearZ = 1.0f;
	mMainPassCB.FarZ = 1000.0f;
	mMainPassCB.TotalTime = gt.TotalTime();
	mMainPassCB.DeltaTime = gt.DeltaTime();
	mMainPassCB.AmbientLight = { 0.25f, 0.25f, 0.35f, 1.0f };
	XMVECTOR lightDir = -MathHelper::SphericalToCartesian(1.0f, mSunTheta, mSunPhi);
	mMainPassCB.FogColor = XMFLOAT4(0.7f, 0.7f, 0.7f, 1.0f);
	mMainPassCB.FogRange = 150.0f;
	mMainPassCB.FogStart = 5.0f;
	XMStoreFloat3(&mMainPassCB.Lights[0].Direction, lightDir);
	mMainPassCB.Lights[0].Strength = { 1.0f, 1.0f, 0.9f };
	mMainPassCB.Lights[1].Direction = { 0.57735f, -0.57735f, 0.57735f };
	mMainPassCB.Lights[1].Strength = { 0.9f, 0.9f, 0.8f };
	mMainPassCB.Lights[2].Direction = { -0.57735f, -0.57735f, 0.57735f };
	mMainPassCB.Lights[2].Strength = { 0.3f, 0.3f, 0.3f };
	mMainPassCB.Lights[3].Direction = { 0.0f, -0.707f, -0.707f };
	mMainPassCB.Lights[3].Strength = { 0.15f, 0.15f, 0.15f };

	mMainPassCB.AmbientLight = { 0.25f, 0.25f, 0.35f, 1.0f };
	auto currPassCB = mCurrFrameResource->PassCB.get();
	currPassCB->CopyData(0, mMainPassCB);
}

void LitWavesApp::UpdateWaves(const GameTimer & gt)
{
	static float t_base = 0.0f;

	if ((mTimer.TotalTime() - t_base) >= 0.25f)
	{
		t_base += 0.25f;

		int i = MathHelper::Rand(4, mWaves->RowCount() - 5);
		int j = MathHelper::Rand(4, mWaves->ColumnCount() - 5);

		float r = MathHelper::RandF(0.2f, 0.5f);

		mWaves->Disturb(i, j, r);
	}

	// Update the wave simulation.
	mWaves->Update(gt.DeltaTime());

	// Update the wave vertex buffer with the new solution.
	auto currWavesVB = mCurrFrameResource->WavesVB.get();
	for (int i = 0; i < mWaves->VertexCount(); ++i)
	{
		Vertex v;

		v.Pos = mWaves->Position(i);
		v.Normal = mWaves->Normal(i);
		v.TexC.x = 0.5f + v.Pos.x / mWaves->Width();
		v.TexC.y = 0.5f - v.Pos.z / mWaves->Depth();

		currWavesVB->CopyData(i, v);
	}

	// Set the dynamic VB of the wave renderitem to the current frame VB.
	mWavesRItem->Geo->VertexBufferGPU = currWavesVB->Resource();
}

void LitWavesApp::UpdateLightning(const GameTimer& gt)
{
	static float t_base = 0.0f;
	if (gt.TotalTime() - t_base > 0.10f) {
		t_base += 0.10f;
		mCurrentLightningFrameIndex = (mCurrentLightningFrameIndex + 1) % 60;
		mMaterials["bolt"]->DiffuseSrvHeapIndex = mCurrentLightningFrameIndex + 5;
		mMaterials["bolt"]->NumFramesDirty = 3;
	}
}

void LitWavesApp::AnimateMaterials(const GameTimer & gt)
{
	auto waterMat = mMaterials["water"].get();
	float& tu = waterMat->MatTransform(3, 0);
	float& tv = waterMat->MatTransform(3, 1);

	tu += 0.1f * gt.DeltaTime();
	tv += 0.02f * gt.DeltaTime();
	if (tu >= 1.0f)
		tu -= 1.0f;

	if (tv >= 1.0f)
		tv -= 1.0f;

	waterMat->MatTransform(3, 0) = tu;
	waterMat->MatTransform(3, 1) = tv;
	waterMat->NumFramesDirty = mNumFrameResources;


}

void LitWavesApp::BuildTextures()
{
	auto createTex = [&](const std::string& name, const std::wstring& fileName) {
		auto tex = std::make_unique<Texture>();
		tex->Name = name;
		tex->Filename = fileName;
		ThrowIfFailed(CreateDDSTextureFromFile12(
			md3dDevice.Get(),
			mCommandList.Get(),
			tex->Filename.c_str(),
			tex->Resource,
			tex->UploadHeap));
		mTextures[tex->Name] = std::move(tex);
	};

	createTex("water", L"Textures//modTextures//water1.dds");
	createTex("grass", L"Textures//grass.dds");
	createTex("wireFence", L"Textures/WireFence.dds");
	createTex("white", L"Textures/white1x1.dds");
	createTex("treeArray", L"Textures//trees//treeArray.dds");

	std::wostringstream ws;
	std::stringstream ss;
	std::wstring num = L"";
	std::wstring filePath = L"";
	std::wstring fileExtension = L".DDS";
	for (UINT i = 1; i < 61; ++i) {
		ws << i;
		ss << i;
		if (i < 10) {
			filePath = L"BoltAnim//boltDDSFiles//Bolt00" + std::wstring(ws.str()) + fileExtension;
			createTex("bolt-" + std::string(ss.str()), filePath);
		}
		else {
			filePath = L"BoltAnim//boltDDSFiles//Bolt0" + std::wstring(ws.str()) + fileExtension;
			createTex("bolt-" + std::string(ss.str()), filePath);
		}
		ws.str(std::wstring());
		ss.str(std::string());
	}
}

void LitWavesApp::BuildDescriptorHeaps()
{
	D3D12_DESCRIPTOR_HEAP_DESC desc;
	desc.NumDescriptors = (UINT)mTextures.size();
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	desc.NodeMask = 0;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(mSrvHeap.GetAddressOf())));
}

void LitWavesApp::BuildTextureDescriptors()
{
	D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
	desc.Texture2D.MostDetailedMip = 0;
	desc.Texture2D.ResourceMinLODClamp = 0.0f;
	desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mSrvHeap->GetCPUDescriptorHandleForHeapStart());
	auto buildTextureDescriptor = [&](const std::unique_ptr<Texture>& tex) {
		auto resource = tex->Resource;
		desc.Format = resource->GetDesc().Format;
		desc.Texture2D.MipLevels = resource->GetDesc().MipLevels;
		md3dDevice->CreateShaderResourceView(resource.Get(), &desc, handle);
		handle.Offset(1, mCbvSrvDescriptorSize);
	};

	buildTextureDescriptor(mTextures["grass"]);
	buildTextureDescriptor(mTextures["water"]);
	buildTextureDescriptor(mTextures["wireFence"]);
	buildTextureDescriptor(mTextures["white"]);

	std::stringstream ss;
	for (int i = 1; i < 61; ++i) {
		ss << i;
		buildTextureDescriptor(mTextures["bolt-" + ss.str()]);
		ss.str(std::string());
	}

	auto buildTexture2DArrayDescriptor = [&](const std::unique_ptr<Texture>& tex) {
		auto resource = tex->Resource;
		desc.Texture2DArray.ArraySize = resource->GetDesc().DepthOrArraySize;
		desc.Texture2DArray.MostDetailedMip = 0;
		desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
		desc.Format = resource->GetDesc().Format;
		desc.Texture2DArray.MipLevels = resource->GetDesc().MipLevels;
		desc.Texture2DArray.ResourceMinLODClamp = 0.0f;
		md3dDevice->CreateShaderResourceView(resource.Get(), &desc, handle);
		handle.Offset(1, mCbvSrvDescriptorSize);
	};

	buildTexture2DArrayDescriptor(mTextures["treeArray"]);
}

std::array<const CD3DX12_STATIC_SAMPLER_DESC, 1> LitWavesApp::GetStaticSamplers()
{
	const CD3DX12_STATIC_SAMPLER_DESC anisotropicWrap(
		0,
		D3D12_FILTER_ANISOTROPIC,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		0.0f,
		8
	);

	return { anisotropicWrap };
}

void LitWavesApp::BuildRootSignature()
{
	CD3DX12_ROOT_PARAMETER slotRootParameter[5];


	slotRootParameter[0].InitAsConstantBufferView(0); // object constants
	slotRootParameter[1].InitAsConstantBufferView(1); // material constnats
	slotRootParameter[2].InitAsConstantBufferView(2); // pass constants

	auto srvTable0 = CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); // textures
	slotRootParameter[3].InitAsDescriptorTable(1, &srvTable0, D3D12_SHADER_VISIBILITY_PIXEL);

	auto srvTable1 = CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1);
	slotRootParameter[4].InitAsDescriptorTable(1, &srvTable1,
		D3D12_SHADER_VISIBILITY_ALL
	);

	auto staticSamplers = GetStaticSamplers();

	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(5, slotRootParameter, (UINT)staticSamplers.size(), staticSamplers.data(),
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
		IID_PPV_ARGS(mRootSignature.GetAddressOf())));
}

void LitWavesApp::BuildShadersAndInputLayout()
{
	const D3D_SHADER_MACRO arrayDraw[] = {
		"FOG", "0",
		"ARRAY_DRAW", "1",
		"ALPHA_TEST", "1",
		NULL, NULL
	};

	mShaders["standardVS"] = d3dUtil::CompileShader(L"Shaders\\AnimatedWaves.hlsl", nullptr, "VS", "vs_5_0");
	mShaders["arrayDrawVS"] = d3dUtil::CompileShader(L"Shaders\\TreeGeometry.hlsl", arrayDraw, "GeoVS", "vs_5_0");
	mShaders["arrayDrawGS"] = d3dUtil::CompileShader(L"Shaders\\TreeGeometry.hlsl", arrayDraw, "GeoGS", "gs_5_0");
	const D3D_SHADER_MACRO alphaDefines[] = {
		"FOG", "0",
		"ALPHA_TEST", "1",
		NULL, NULL
	};

	const D3D_SHADER_MACRO fog[] = {
		"FOG", "0",
		NULL, NULL
	};

	const D3D_SHADER_MACRO color[] = {
		"COLOR", "0",
		NULL, NULL
	};

	mShaders["depthVisualizerPS"] = d3dUtil::CompileShader(L"Shaders\\AnimatedWaves.hlsl", color, "PS", "ps_5_0");
	mShaders["opaquePS"] = d3dUtil::CompileShader(L"Shaders\\AnimatedWaves.hlsl", fog, "PS", "ps_5_0");
	mShaders["alphaTestedPS"] = d3dUtil::CompileShader(L"Shaders\\AnimatedWaves.hlsl", alphaDefines, "PS", "ps_5_0");
	mShaders["arrayDrawPS"] = d3dUtil::CompileShader(L"Shaders\\TreeGeometry.hlsl", arrayDraw, "GeoPS", "ps_5_0");

	mShaders["subdividerPS"] = d3dUtil::CompileShader(L"Shaders\\Subdivide.hlsl", nullptr, "SPS", "ps_5_0");
	mShaders["subdividerGS"] = d3dUtil::CompileShader(L"Shaders\\Subdivide.hlsl", nullptr, "SGS", "gs_5_0");
	mShaders["subdividerVS"] = d3dUtil::CompileShader(L"Shaders\\Subdivide.hlsl", nullptr, "DefaultVS", "vs_5_0");
	mShaders["normalVisualizerGS"] = d3dUtil::CompileShader(L"Shaders\\NormalFaceVisualizer.hlsl", nullptr, "DefaultGS", "gs_5_0");
	mShaders["normalVisualizerPS"] = d3dUtil::CompileShader(L"Shaders\\NormalFaceVisualizer.hlsl", nullptr, "DefaultPS", "ps_5_0");
	mShaders["normalVisualizerVS"] = d3dUtil::CompileShader(L"Shaders\\NormalFaceVisualizer.hlsl", nullptr, "DefaultVS", "vs_5_0");

	mShaders["explosionGS"] = d3dUtil::CompileShader(L"Shaders\\Explosion.hlsl", nullptr, "DefaultGS", "gs_5_0");
	mShaders["explosionPS"] = d3dUtil::CompileShader(L"Shaders\\Explosion.hlsl", nullptr, "DefaultPS", "ps_5_0");
	mShaders["explosionVS"] = d3dUtil::CompileShader(L"Shaders\\Explosion.hlsl", nullptr, "DefaultVS", "vs_5_0");

	mInputLayouts[(int)InputLayouts::Standard] = 
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	mInputLayouts[(int)InputLayouts::TreeArray] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "SIZE", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	};

	mInputLayouts[(int)InputLayouts::Local] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	};
}

void LitWavesApp::BuildLandGeometry()
{
	GeometryGenerator geoGen;
	GeometryGenerator::MeshData grid = geoGen.CreateGrid(160.0f, 160.0f, 50, 50);
	auto box = geoGen.CreateBox(5.0f, 5.0f, 5.0f, 3);
	auto cylinder = geoGen.CreateCylinder(5.0f, 5.0f, 5.0f, 40, 20, false);
	//
	// Extract the vertex elements we are interested and apply the height function to
	// each vertex.  In addition, color the vertices based on their height so we have
	// sandy looking beaches, grassy low hills, and snow mountain peaks.
	//

	auto gridVertexCount = (UINT)grid.Vertices.size();
	auto boxVertexCount = (UINT)box.Vertices.size();
	auto cylinderVertexCount = (UINT)cylinder.Vertices.size();
	auto totalVertexSize = gridVertexCount + boxVertexCount + cylinderVertexCount;

	std::vector<Vertex> vertices(totalVertexSize);
	for (UINT i = 0; i < gridVertexCount; ++i)
	{
		auto& p = grid.Vertices[i].Position;
		vertices[i].Pos = p;
		vertices[i].Pos.y = GetHillsHeight(p.x, p.z);
		vertices[i].Normal = GetHillsNormal(p.x, p.z);
		vertices[i].TexC = grid.Vertices[i].TexC;
	}

	auto baseVertexCount = 0;
	for (UINT i = gridVertexCount; i < gridVertexCount + boxVertexCount; ++i, ++baseVertexCount) {
		vertices[i].Pos = box.Vertices[baseVertexCount].Position;
		vertices[i].Normal = box.Vertices[baseVertexCount].Normal;
		vertices[i].TexC = box.Vertices[baseVertexCount].TexC;
	}

	baseVertexCount = 0;
	for (UINT i = gridVertexCount + boxVertexCount; i < totalVertexSize; ++i, ++baseVertexCount) {
		vertices[i].Pos = cylinder.Vertices[baseVertexCount].Position;
		vertices[i].Normal = cylinder.Vertices[baseVertexCount].Normal;
		vertices[i].TexC = cylinder.Vertices[baseVertexCount].TexC;
	}

	const UINT vbByteSize = totalVertexSize * sizeof(Vertex);

	std::vector<std::uint16_t> gridIndices = grid.GetIndices16();
	std::vector<std::uint16_t> boxIndices = box.GetIndices16();
	std::vector<std::uint16_t> cylinderIndices = cylinder.GetIndices16();
	const UINT gridIndexCount = (UINT)gridIndices.size();
	const UINT boxIndexCount = (UINT)boxIndices.size();
	const UINT cylinderIndexCount = (UINT)cylinderIndices.size();
	const UINT numIndices = gridIndexCount + boxIndexCount + cylinderIndexCount;

	std::vector<std::uint32_t> allIndices(numIndices);
	uint32_t currIndex = 0;
	auto concatIndices = [&](const std::vector<uint16_t>& toConcat) {
		std::for_each(toConcat.begin(), toConcat.end(), [&](uint32_t toAdd) { allIndices[currIndex++] = toAdd; });
	};
	concatIndices(gridIndices);
	concatIndices(boxIndices);
	concatIndices(cylinderIndices);

	const UINT ibByteSize = numIndices * sizeof(std::uint32_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "landGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), allIndices.data(), ibByteSize);

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), allIndices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R32_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry submesh;
	submesh.IndexCount = gridIndexCount;
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;
	geo->DrawArgs["grid"] = submesh;

	submesh.IndexCount = boxIndexCount;
	submesh.StartIndexLocation = gridIndexCount;
	submesh.BaseVertexLocation = gridVertexCount;
	geo->DrawArgs["box"] = submesh;

	submesh.IndexCount = cylinderIndexCount;
	submesh.StartIndexLocation = gridIndexCount + boxIndexCount;
	submesh.BaseVertexLocation = gridVertexCount + boxVertexCount;
	geo->DrawArgs["cylinder"] = submesh;

	mGeometries["landGeo"] = std::move(geo);
}

void LitWavesApp::BuildWavesGeometryBuffers()
{
	std::vector<std::uint16_t> indices(3 * mWaves->TriangleCount()); // 3 indices per face
	assert(mWaves->VertexCount() < 0x0000ffff);

	// Iterate over each quad.
	int m = mWaves->RowCount();
	int n = mWaves->ColumnCount();
	int k = 0;
	for (int i = 0; i < m - 1; ++i)
	{
		for (int j = 0; j < n - 1; ++j)
		{
			indices[k] = i*n + j;
			indices[k + 1] = i*n + j + 1;
			indices[k + 2] = (i + 1)*n + j;

			indices[k + 3] = (i + 1)*n + j;
			indices[k + 4] = i*n + j + 1;
			indices[k + 5] = (i + 1)*n + j + 1;

			k += 6; // next quad
		}
	}

	UINT vbByteSize = mWaves->VertexCount() * sizeof(Vertex);
	UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "waterGeo";

	// Set dynamically.
	geo->VertexBufferCPU = nullptr;
	geo->VertexBufferGPU = nullptr;

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	geo->DrawArgs["grid"] = submesh;

	mGeometries["waterGeo"] = std::move(geo);
}

void LitWavesApp::BuildDepthVisualizationQuads()
{
	std::array<Vertex, 4> quadVertices = {
		Vertex(-0.5f, +0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f), // top left
		Vertex(+0.5f, +0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f), // top right
		Vertex(+0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f), // bottom right
		Vertex(-0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f), // bottom left
	};

	std::array<std::uint16_t, 6> quadIndices = {
		0, 1, 2,
		0, 2, 3,
	};

	auto vbByteSize = (UINT)quadVertices.size() * sizeof(Vertex);
	auto ibByteSize = (UINT)quadIndices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "visualizationQuad";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));

	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), quadVertices.data(), vbByteSize);
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), quadIndices.data(), ibByteSize);

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(), mCommandList.Get(),
		quadVertices.data(), vbByteSize, geo->VertexBufferUploader);
	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(), mCommandList.Get(),
		quadIndices.data(), ibByteSize, geo->IndexBufferUploader);

	SubmeshGeometry quadSubmesh;
	quadSubmesh.BaseVertexLocation = 0;
	quadSubmesh.IndexCount = (UINT)quadIndices.size();
	quadSubmesh.StartIndexLocation = 0;
	geo->DrawArgs["quad"] = quadSubmesh;

	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexBufferByteSize = ibByteSize;

	mGeometries[geo->Name] = std::move(geo);
}

void LitWavesApp::BuildTreeSpriteGeometry()
{
	struct TreeSpriteVertex
	{
		XMFLOAT3 Pos;
		XMFLOAT2 Size;
	};

	static const int treeCount = 16;
	std::array<TreeSpriteVertex, 16> vertices;

	int i = 0;
	while (i < treeCount) {
		float x = MathHelper::RandF(-45.0f, 45.0f);
		float z = MathHelper::RandF(-45.0f, 45.0f);
		float y = GetHillsHeight(x, z);
		if (y > 0) {
			y += 8.0f;
			vertices[i].Pos = XMFLOAT3(x, y, z);
			vertices[i].Size = XMFLOAT2(20.0f, 20.0f);
			++i;
		}
	}

	std::array<std::uint16_t, 16> indices =
	{
		0, 1, 2, 3, 4, 5, 6, 7,
		8, 9, 10, 11, 12, 13, 14, 15
	};

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(TreeSpriteVertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "treeSpritesGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(TreeSpriteVertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	geo->DrawArgs["points"] = submesh;

	mGeometries["treeSpritesGeo"] = std::move(geo);
}

D3D12_GRAPHICS_PIPELINE_STATE_DESC LitWavesApp::BuildOpaquePSO()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc;

	//
	// PSO for opaque objects.
	//
	ZeroMemory(&opaquePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	opaquePsoDesc.InputLayout = { mInputLayouts[(int)InputLayouts::Standard].data(),
			(UINT)mInputLayouts[(int)InputLayouts::Standard].size() };
	opaquePsoDesc.pRootSignature = mRootSignature.Get();
	opaquePsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["standardVS"]->GetBufferPointer()),
		mShaders["standardVS"]->GetBufferSize()
	};
	opaquePsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["opaquePS"]->GetBufferPointer()),
		mShaders["opaquePS"]->GetBufferSize()
	};
	auto rasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	// rasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	opaquePsoDesc.RasterizerState = rasterizerState;
	opaquePsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	opaquePsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	opaquePsoDesc.SampleMask = UINT_MAX;
	opaquePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	opaquePsoDesc.NumRenderTargets = 1;
	opaquePsoDesc.RTVFormats[0] = mBackBufferFormat;
	opaquePsoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	opaquePsoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	opaquePsoDesc.DSVFormat = mDepthStencilFormat;
	opaquePsoDesc.DepthStencilState.StencilEnable = true;

	// increment the stencil count
	opaquePsoDesc.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	opaquePsoDesc.DepthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP_INCR;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&mPSOs["opaque"])));
	return opaquePsoDesc;
}

void LitWavesApp::BuildBlendedPSO(const D3D12_GRAPHICS_PIPELINE_STATE_DESC& prevPSO)
{
	D3D12_BLEND_DESC blendDesc = {};
	blendDesc.AlphaToCoverageEnable = false;
	blendDesc.IndependentBlendEnable = false;

	D3D12_RENDER_TARGET_BLEND_DESC rtbDesc = {};
	rtbDesc.BlendEnable = true;
	rtbDesc.LogicOpEnable = false;
	rtbDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
	rtbDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
	rtbDesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	rtbDesc.DestBlendAlpha = D3D12_BLEND_ZERO;
	rtbDesc.BlendOp = D3D12_BLEND_OP_ADD;
	rtbDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
	rtbDesc.LogicOp = D3D12_LOGIC_OP_NOOP;
	rtbDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	auto transparentPSO = prevPSO;
	transparentPSO.BlendState.RenderTarget[0] = rtbDesc;
	transparentPSO.DepthStencilState.StencilEnable = true;
	transparentPSO.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	transparentPSO.DepthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP_INCR;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&transparentPSO, IID_PPV_ARGS(&mPSOs["transparent"])));

	auto additive = transparentPSO;
	additive.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	additive.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&additive, IID_PPV_ARGS(&mPSOs["additive"])));

	auto depthVisualizer = additive;
	depthVisualizer.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	depthVisualizer.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
	depthVisualizer.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	depthVisualizer.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	depthVisualizer.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
	depthVisualizer.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;
	depthVisualizer.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
	depthVisualizer.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;

	depthVisualizer.PS = {
		reinterpret_cast<BYTE*>(mShaders["depthVisualizerPS"]->GetBufferPointer()),
		(UINT)mShaders["depthVisualizerPS"]->GetBufferSize()
	};

	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&depthVisualizer, IID_PPV_ARGS(&mPSOs["blendDepthVisualizer"])));
}

void LitWavesApp::BuildDepthVisualizationPSOs(const D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc)
{
	D3D12_DEPTH_STENCIL_DESC markerDSS;

	markerDSS.DepthEnable = true;
	markerDSS.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	markerDSS.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	markerDSS.StencilEnable = true;
	markerDSS.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
	markerDSS.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
	markerDSS.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	markerDSS.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	markerDSS.FrontFace.StencilPassOp = D3D12_STENCIL_OP_INCR;
	markerDSS.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;

	markerDSS.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	markerDSS.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	markerDSS.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	markerDSS.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;

	auto markerPSO = desc;
	markerPSO.DepthStencilState = markerDSS;

	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&markerPSO, IID_PPV_ARGS(&mPSOs["stencilMarker"])));

	auto depthVisualizerDSS = markerDSS;
	depthVisualizerDSS.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL;
	depthVisualizerDSS.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	depthVisualizerDSS.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	depthVisualizerDSS.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;

	auto depthVisualizerPSO = desc;
	depthVisualizerPSO.DepthStencilState = depthVisualizerDSS;
	depthVisualizerPSO.PS = {
		reinterpret_cast<BYTE*>(mShaders["depthVisualizerPS"]->GetBufferPointer()),
		mShaders["depthVisualizerPS"]->GetBufferSize()
	};
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&depthVisualizerPSO, IID_PPV_ARGS(&mPSOs["depthVisualizer"])));
}

void LitWavesApp::BuildTreeArrayPSO(const D3D12_GRAPHICS_PIPELINE_STATE_DESC & desc)
{
	auto treeArrayPSO = desc;
	AddShadersToPSO(treeArrayPSO, "arrayDraw");

	treeArrayPSO.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	treeArrayPSO.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	treeArrayPSO.InputLayout = {
		mInputLayouts[(int)InputLayouts::TreeArray].data(),
		(UINT)mInputLayouts[(int)InputLayouts::TreeArray].size()
	};

	treeArrayPSO.BlendState.AlphaToCoverageEnable = true;

	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&treeArrayPSO, IID_PPV_ARGS(&mPSOs["treeArray"])));
}

void LitWavesApp::BuildPSOs()
{
	auto pso = BuildOpaquePSO();
	BuildBlendedPSO(pso);
	BuildTreeArrayPSO(pso);

	auto wireframe = pso;
	wireframe.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	wireframe.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&wireframe, IID_PPV_ARGS(&mPSOs["wireframe"])));
	auto alphaTestedPSO = pso;
	alphaTestedPSO.PS = {
		reinterpret_cast<BYTE*>(mShaders["alphaTestedPS"]->GetBufferPointer()),
		mShaders["alphaTestedPS"]->GetBufferSize()
	};
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&alphaTestedPSO, IID_PPV_ARGS(&mPSOs["alphaTested"])));

	BuildDepthVisualizationPSOs(pso);

	auto subdividerPSO = wireframe;
	AddShadersToPSO(subdividerPSO, "subdivider");

	subdividerPSO.InputLayout = {
		mInputLayouts[(int)InputLayouts::Local].data(),
		mInputLayouts[(int)InputLayouts::Local].size()
	};

	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&subdividerPSO, IID_PPV_ARGS(&mPSOs["subdivider"])));

	auto normalVisualizerPso = pso;
	AddShadersToPSO(normalVisualizerPso, "normalVisualizer");
	normalVisualizerPso.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&normalVisualizerPso, IID_PPV_ARGS(&mPSOs["normalVisualizer"])));

	auto explosionPso = pso;
	AddShadersToPSO(explosionPso, "explosion");
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&explosionPso, IID_PPV_ARGS(&mPSOs["explosion"])));
}

void LitWavesApp::BuildFrameResources()
{
	for (int i = 0; i < mNumFrameResources; ++i)
	{
		mFrameResources.push_back(std::make_unique<FrameResource>(md3dDevice.Get(),
			1, (UINT)mAllRItems.size(), (UINT)mMaterials.size(), mWaves->VertexCount()));
	}
}

void LitWavesApp::BuildMaterials()
{
	auto grass = std::make_unique<Material>(mNumFrameResources);
	grass->Name = "grass";
	grass->MatCBIndex = 0;
	grass->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	grass->FresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);
	grass->Roughness = 0.7f;
	grass->DiffuseSrvHeapIndex = 0;

	auto water = std::make_unique<Material>(mNumFrameResources);
	water->Name = "water";
	water->MatCBIndex = 1;
	water->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 0.3f);
	water->FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
	water->Roughness = 0.0f;
	water->DiffuseSrvHeapIndex = 1;

	auto wirefence = std::make_unique<Material>(mNumFrameResources);
	wirefence->Name = "wirefence";
	wirefence->MatCBIndex = 2;
	wirefence->DiffuseSrvHeapIndex = 2;
	wirefence->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	wirefence->FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
	wirefence->Roughness = 0.25f;

	auto white = std::make_unique<Material>(mNumFrameResources);
	white->Name = "white";
	white->MatCBIndex = 3;
	white->DiffuseSrvHeapIndex = 3;
	white->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	white->FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
	white->Roughness = 0.01f;

	auto treeSprites = std::make_unique<Material>(mNumFrameResources);
	treeSprites->Name = "treeSprites";
	treeSprites->MatCBIndex = 4;
	treeSprites->DiffuseSrvHeapIndex = 4;
	treeSprites->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	treeSprites->FresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);
	treeSprites->Roughness = 0.125f;

	auto bolt = std::make_unique<Material>(mNumFrameResources);
	bolt->Name = "bolt";
	bolt->MatCBIndex = 5;
	bolt->DiffuseSrvHeapIndex = 5;
	bolt->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 0.3f);
	bolt->FresnelR0 = XMFLOAT3(1.0f, 1.0f, 1.0f);
	bolt->Roughness = 0.01f;

	mMaterials["grass"] = std::move(grass);
	mMaterials["water"] = std::move(water);
	mMaterials["wirefence"] = std::move(wirefence);
	mMaterials["white"] = std::move(white);
	mMaterials["bolt"] = std::move(bolt);
	mMaterials["treeSprites"] = std::move(treeSprites);
}

void LitWavesApp::BuildRenderItems()
{
	XMFLOAT4 color = XMFLOAT4(0.05f, 0.05f, 0.05f, 1.0f);
	auto wavesRitem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&wavesRitem->TexTransform, XMMatrixScaling(5.0f, 5.0f, 1.0f));
	wavesRitem->World = MathHelper::Identity4x4();
	wavesRitem->ObjCBIndex = 0;
	wavesRitem->Mat = mMaterials["water"].get();
	wavesRitem->Geo = mGeometries["waterGeo"].get();
	wavesRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	wavesRitem->IndexCount = wavesRitem->Geo->DrawArgs["grid"].IndexCount;
	wavesRitem->StartIndexLocation = wavesRitem->Geo->DrawArgs["grid"].StartIndexLocation;
	wavesRitem->BaseVertexLocation = wavesRitem->Geo->DrawArgs["grid"].BaseVertexLocation;
	wavesRitem->Color = color;
	mWavesRItem = wavesRitem.get();

	mRitemLayer[(int)RenderLayer::Blended].push_back(wavesRitem.get());

	auto gridRitem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&gridRitem->TexTransform, XMMatrixScaling(5.0f, 5.0f, 1.0f));
	gridRitem->World = MathHelper::Identity4x4();
	gridRitem->ObjCBIndex = 1;
	gridRitem->Mat = mMaterials["grass"].get();
	gridRitem->Geo = mGeometries["landGeo"].get();
	gridRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	gridRitem->IndexCount = gridRitem->Geo->DrawArgs["grid"].IndexCount;
	gridRitem->StartIndexLocation = gridRitem->Geo->DrawArgs["grid"].StartIndexLocation;
	gridRitem->BaseVertexLocation = gridRitem->Geo->DrawArgs["grid"].BaseVertexLocation;
	gridRitem->Color = color;
	mRitemLayer[(int)RenderLayer::Opaque].push_back(gridRitem.get());

	auto fenceRItem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&fenceRItem->World, XMMatrixTranslation(-2.0f, 1.0f, 0.0f));
	fenceRItem->ObjCBIndex = 2;
	fenceRItem->Mat = mMaterials["wirefence"].get();
	fenceRItem->Geo = mGeometries["landGeo"].get();
	fenceRItem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	fenceRItem->IndexCount = fenceRItem->Geo->DrawArgs["box"].IndexCount;
	fenceRItem->StartIndexLocation = fenceRItem->Geo->DrawArgs["box"].StartIndexLocation;
	fenceRItem->BaseVertexLocation = fenceRItem->Geo->DrawArgs["box"].BaseVertexLocation;
	fenceRItem->Color = color;
	mRitemLayer[(int)RenderLayer::AlphaTested].push_back(fenceRItem.get());

	auto cylinderRItem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&cylinderRItem->World, XMMatrixTranslation(-2.0f, 2.0f, 0.0f));
	cylinderRItem->ObjCBIndex = 3;
	cylinderRItem->Mat = mMaterials["bolt"].get();
	cylinderRItem->Geo = mGeometries["landGeo"].get();
	cylinderRItem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	cylinderRItem->IndexCount = cylinderRItem->Geo->DrawArgs["cylinder"].IndexCount;
	cylinderRItem->StartIndexLocation = cylinderRItem->Geo->DrawArgs["cylinder"].StartIndexLocation;
	cylinderRItem->BaseVertexLocation = cylinderRItem->Geo->DrawArgs["cylinder"].BaseVertexLocation;
	cylinderRItem->Color = color;
	mRitemLayer[(int)RenderLayer::Additive].emplace_back(cylinderRItem.get());

	auto treeSpritesRitem = std::make_unique<RenderItem>();
	treeSpritesRitem->World = MathHelper::Identity4x4();
	treeSpritesRitem->ObjCBIndex = 4;
	treeSpritesRitem->Mat = mMaterials["treeSprites"].get();
	treeSpritesRitem->Geo = mGeometries["treeSpritesGeo"].get();
	treeSpritesRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
	treeSpritesRitem->IndexCount = treeSpritesRitem->Geo->DrawArgs["points"].IndexCount;
	treeSpritesRitem->StartIndexLocation = treeSpritesRitem->Geo->DrawArgs["points"].StartIndexLocation;
	treeSpritesRitem->BaseVertexLocation = treeSpritesRitem->Geo->DrawArgs["points"].BaseVertexLocation;

	mRitemLayer[(int)RenderLayer::AlphaTestedTreeSprites].push_back(treeSpritesRitem.get());

	auto icoRitem = std::make_unique<RenderItem>();

	XMStoreFloat4x4(&icoRitem->World, XMMatrixScaling(5.0f, 5.0f, 5.0f)*XMMatrixTranslation(-2.0f, 10.0f, 0.0f));
	icoRitem->ObjCBIndex = 5;
	icoRitem->Mat = mMaterials["grass"].get();
	icoRitem->Geo = mSubdivider->GetGeometry();
	icoRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	auto sub = mSubdivider->GetSubmesh("icosahedron");
	icoRitem->IndexCount = sub.IndexCount;
	icoRitem->StartIndexLocation = sub.StartIndexLocation;
	icoRitem->BaseVertexLocation = sub.BaseVertexLocation;
	mRitemLayer[(int)RenderLayer::OptimizedSphere].push_back(icoRitem.get());

	mAllRItems.push_back(std::move(wavesRitem));
	mAllRItems.push_back(std::move(gridRitem));
	mAllRItems.push_back(std::move(fenceRItem));
	mAllRItems.push_back(std::move(cylinderRItem));
	mAllRItems.push_back(std::move(treeSpritesRitem));
	mAllRItems.push_back(std::move(icoRitem));

	UINT numColors = (UINT)mDepthColors.size();

	for (auto& color : mDepthColors) {
		auto depthQuadRItem = std::make_unique<RenderItem>();
		depthQuadRItem->ObjCBIndex = (UINT)mAllRItems.size();
		depthQuadRItem->Mat = mMaterials["white"].get();
		depthQuadRItem->Geo = mGeometries["visualizationQuad"].get();
		depthQuadRItem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		depthQuadRItem->IndexCount = depthQuadRItem->Geo->DrawArgs["quad"].IndexCount;
		depthQuadRItem->BaseVertexLocation = depthQuadRItem->Geo->DrawArgs["quad"].BaseVertexLocation;
		depthQuadRItem->StartIndexLocation = depthQuadRItem->Geo->DrawArgs["quad"].StartIndexLocation;
		XMStoreFloat4(&depthQuadRItem->Color, color);
		mRitemLayer[(int)RenderLayer::DepthVisualizer].push_back(depthQuadRItem.get());
		mAllRItems.push_back(std::move(depthQuadRItem));
	}
}

void LitWavesApp::AddShadersToPSO(D3D12_GRAPHICS_PIPELINE_STATE_DESC & desc, const std::string & shaderPrefix)
{
	auto psString = shaderPrefix + "PS";
	auto vsString = shaderPrefix + "VS";
	auto gsString = shaderPrefix + "GS";
	desc.PS = {
		reinterpret_cast<BYTE*>(mShaders[psString]->GetBufferPointer()),
		mShaders[psString]->GetBufferSize()
	};
	
	desc.VS = {
		reinterpret_cast<BYTE*>(mShaders[vsString]->GetBufferPointer()),
		mShaders[vsString]->GetBufferSize()
	};

	desc.GS = {
		reinterpret_cast<BYTE*>(mShaders[gsString]->GetBufferPointer()),
		mShaders[gsString]->GetBufferSize()
	};
}

void LitWavesApp::DrawRenderItems(ID3D12GraphicsCommandList * cmdList, const std::vector<RenderItem*>& rItems, bool isVisualizers)
{
	UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
	UINT matCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(MaterialConstants));

	auto objectCB = mCurrFrameResource->ObjectCB->Resource();
	auto matCB = mCurrFrameResource->MaterialCB->Resource();

	// For each render item...
	for (size_t i = 0; i < rItems.size(); ++i)
	{
		auto ri = rItems[i];
		cmdList->IASetVertexBuffers(0, 1, &ri->Geo->VertexBufferView());
		cmdList->IASetIndexBuffer(&ri->Geo->IndexBufferView());
		cmdList->IASetPrimitiveTopology(ri->PrimitiveType);

		if (isVisualizers) {
			cmdList->OMSetStencilRef(i);
		}

		D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress() + ri->ObjCBIndex*objCBByteSize;
		D3D12_GPU_VIRTUAL_ADDRESS matCBAddress = matCB->GetGPUVirtualAddress() + ri->Mat->MatCBIndex*matCBByteSize;

		cmdList->SetGraphicsRootConstantBufferView(0, objCBAddress);
		cmdList->SetGraphicsRootConstantBufferView(1, matCBAddress);


		CD3DX12_GPU_DESCRIPTOR_HANDLE texHandle(mSrvHeap->GetGPUDescriptorHandleForHeapStart());
		texHandle.Offset(ri->Mat->DiffuseSrvHeapIndex, mCbvSrvDescriptorSize);
		cmdList->SetGraphicsRootDescriptorTable(3, texHandle);


		cmdList->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
	}

	cmdList->OMSetStencilRef(0);
}

float LitWavesApp::GetHillsHeight(float x, float z) const
{
	return 0.3f*(z*sinf(0.1f*x) + x*cosf(0.1f*z));
}

XMFLOAT3 LitWavesApp::GetHillsNormal(float x, float z) const
{
	// n = (-df/dx, 1, -df/dz)
	XMFLOAT3 n(
		-0.03f*z*cosf(0.1f*x) - 0.3f*cosf(0.1f*z),
		1.0f,
		-0.3f*sinf(0.1f*x) + 0.03f*x*sinf(0.1f*z));

	XMVECTOR unitNormal = XMVector3Normalize(XMLoadFloat3(&n));
	XMStoreFloat3(&n, unitNormal);

	return n;
}
