#ifndef STENCIL_APP_H
#define STENCIL_APP_H

#include "GeometricObject.h"
#include "GeometryGenerator.h"
#include "D3DApp.h"
#include "FrameResource.h"
#include "RenderItem.h"
#include "SkullGeometry.h"

using namespace CrateDemo;
using LightingDemo::RenderItem;
using LightingDemo::RenderLayer;

class StencilApp : public D3DApp {
public:
	StencilApp(HINSTANCE hInstance);
	StencilApp(const StencilApp& rhs) = delete;
	StencilApp& operator=(const StencilApp& rhs) = delete;
	~StencilApp();
	bool Initialize() override;

	// initialization methods
private:

	void LoadTextures();
	void BuildFrameResources();
	void BuildGeometry();
	void BuildTextures();
	void BuildMaterials();
	void BuildRenderItems();
	void DrawRenderItems(const std::vector<RenderItem*> rItems);

	void CreateRootSignature();
	void CreateShaderInputLayout();
	void BuildOpaquePSO(D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc);
	void CreatePSOs();

	// update methods
private:
	void OnResize();
	void Update(const GameTimer& gt);
	void UpdateCamera(const GameTimer& gt);
	void UpdateObjectCBs(const GameTimer& gt);
	void UpdateMainPassCB(const GameTimer& gt);

	void Draw(const GameTimer& gt);

	// Convenience overrides for handling mouse input
	void OnMouseDown(WPARAM btnState, int x, int y);
	void OnMouseUp(WPARAM btnState, int x, int y);
	void OnMouseMove(WPARAM btnState, int x, int y);

private:

	int mNumFrameResources = 3;
	std::vector<std::unique_ptr<FrameResource>> mFrameResources;
	FrameResource* mCurrentFrameResource;
	int mCurrentFrameResourceIndex = 0;

	ComPtr<ID3D12RootSignature> mRootSignature;
	std::unordered_map<std::string, ComPtr<ID3DBlob>> mShaders;
	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;

	std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> mPSOs;

	PassConstants mMainPassCB = {};

	std::vector<std::unique_ptr<RenderItem>> mRenderItems;
	std::vector<RenderItem*> mRenderLayers[(UINT)RenderLayer::Count];

	std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> mGeometries;
	SkullGeometry mSkull;
	std::unordered_map<std::string, std::unique_ptr<Texture>> mTextures;
	std::unordered_map<std::string, std::unique_ptr<Material>> mMaterials;

	XMFLOAT4X4 mView = MathHelper::Identity4x4();
	XMFLOAT4X4 mProj = MathHelper::Identity4x4();

	float mPhi = (1.0f/3.0f)*XM_PI;
	float mTheta = XM_PIDIV4;
	float mRadius = 300.0f;
	XMFLOAT3 mEyePos = { 0.0f, 0.0f, 0.0f };

	float mSunPhi = XM_PIDIV4;
	float mSunTheta = XM_PIDIV4;

	const float mNearPlane = 1.0f;
	const float mFarPlane = 1000.0f;
	POINT mLastMousePos;

	const float mGridWidth = 160.0f;
	const float mGridDepth = 120.0f;
};

#endif