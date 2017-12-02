#ifndef STENCIL_APP_H
#define STENCIL_APP_H

#include "GeometricObject.h"
#include "GeometryGenerator.h"
#include "D3DApp.h"
#include "FrameResource.h"
#include "RenderItem.h"
#include "SkullGeometry.h"
#include "DDSTextureLoader.h"

using namespace CrateDemo;
using LightingDemo::RenderItem;
using LightingDemo::RenderLayer;
using namespace DirectX;
using namespace DirectX::PackedVector;

class StencilApp : public D3DApp {
public:
	StencilApp(HINSTANCE hInstance);
	StencilApp(const StencilApp& rhs) = delete;
	StencilApp& operator=(const StencilApp& rhs) = delete;
	~StencilApp();
	bool Initialize() override;

	// initialization methods
private:
	
	void StoreConstants();
	void LoadTextures();
	void BuildFrameResources();
	void BuildSkullGeometry();
	void BuildRoomGeometry();
	void BuildMaterials();
	void BuildRenderItems();
	void BuildDescriptorHeaps();
	void BuildTextureDescriptors();
	void BuildMaterialDescriptors();
	void DrawRenderItems(const std::vector<RenderItem*> rItems);

	void CreateRootSignature();
	void CreateShaderInputLayout();
	void BuildOpaquePSO(D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc);
	void BuildMirrorPSO(const D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc);
	D3D12_GRAPHICS_PIPELINE_STATE_DESC BuildTransparentPSO(const D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc);
	void BuildReflectionPSO(const D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc);
	void BuildShadowPSO(const D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc);

	void CreatePSOs();

	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 1> GetStaticSamplers() const;
	CD3DX12_GPU_DESCRIPTOR_HANDLE GetDescriptorHandleWithOffset(UINT&& offset);

	// update methods
private:
	void OnResize();
	void Update(const GameTimer& gt);
	void UpdateCamera(const GameTimer& gt);
	void UpdateObjectCBs(const GameTimer& gt);
	void UpdateMainPassCB(const GameTimer& gt);
	void UpdateReflectedPassCB(const GameTimer& gt);

	void Draw(const GameTimer& gt);

	// Convenience overrides for handling mouse input
	void OnMouseDown(WPARAM btnState, int x, int y);
	void OnMouseUp(WPARAM btnState, int x, int y);
	void OnMouseMove(WPARAM btnState, int x, int y);
	void OnKeyboardInput(const GameTimer& gt);

private:

	int mNumFrameResources = 3;
	std::vector<std::unique_ptr<FrameResource>> mFrameResources;
	FrameResource* mCurrentFrameResource;
	int mCurrentFrameResourceIndex = 0;

	ComPtr<ID3D12RootSignature> mRootSignature;
	ComPtr<ID3D12DescriptorHeap> mSrvCbvHeap;
	std::unordered_map<std::string, ComPtr<ID3DBlob>> mShaders;
	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;

	std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> mPSOs;

	PassConstants mMainPassCB = {};
	PassConstants mReflectedPassCB = {};

	std::vector<std::unique_ptr<RenderItem>> mRenderItems;
	std::vector<RenderItem*> mRenderLayers[(UINT)RenderLayer::Count];

	RenderItem* mSkullRItem = nullptr;
	RenderItem* mReflectedSkullRItem = nullptr;
	RenderItem* mSkullShadowRItem = nullptr;

	std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> mGeometries;
	SkullGeometry mSkull;
	std::unordered_map<std::string, std::unique_ptr<Texture>> mTextures;
	std::unordered_map<std::string, std::unique_ptr<Material>> mMaterials;
	int mMaterialIndexStart = 0;
	std::vector<std::string> mTextureNames;

	XMFLOAT4X4 mView = MathHelper::Identity4x4();
	XMFLOAT4X4 mProj = MathHelper::Identity4x4();

	float mPhi = (1.0f/3.0f)*XM_PI;
	float mTheta = -XM_PIDIV4;
	float mRadius = 20.0f;
	XMFLOAT3 mEyePos = { 0.0f, 0.0f, 0.0f };

	float mSunPhi = XM_PIDIV4;
	float mSunTheta = XM_PIDIV4;

	float mSkullRotation = XM_PIDIV2;
	float mSkullScale = 0.25f;

	const float mNearPlane = 1.0f;
	const float mFarPlane = 1000.0f;
	POINT mLastMousePos;

	const float mGridWidth = 160.0f;
	const float mGridDepth = 120.0f;

	XMFLOAT4X4 mReflectionMatrix = MathHelper::Identity4x4();
	XMFLOAT4X4 mSkullSMatrix = MathHelper::Identity4x4();
	XMFLOAT4X4 mSkullRMatrix = MathHelper::Identity4x4();
	XMFLOAT4X4 mSkullTMatrix = MathHelper::Identity4x4();
	XMFLOAT4X4 mShadowMatrix = MathHelper::Identity4x4();
	XMFLOAT4X4 mShadowOffset = MathHelper::Identity4x4();

	Light mLights[3];
	XMFLOAT4 mAmbientLight = { 0.25f, 0.25f, 0.35f, 1.0f };
};

#endif