#ifndef LIT_WAVES_H
#define LIT_WAVES_H

#include "D3DApp.h"
#include "RenderItem.h"
#include "FrameResource.h"
#include "Waves.h"
#include "GeometryGenerator.h"
#include "GeometricObject.h"
#include "DDSTextureLoader.h"
#include "Subdivider.h"
#include "Camera.h"

using namespace CrateDemo;
using LightingDemo::RenderItem;
using LightingDemo::RenderLayer;
using LightingDemo::InputLayouts;


class LitWavesApp : public D3DApp {
public:
	LitWavesApp(HINSTANCE hInstance);
	LitWavesApp(const LitWavesApp& rhs) = delete;
	LitWavesApp& operator=(const LitWavesApp& rhs) = delete;
	~LitWavesApp();

	virtual bool Initialize() override;

private:
	virtual void OnResize() override;
	virtual void Update(const GameTimer& gt) override;
	virtual void Draw(const GameTimer& gt) override;

	virtual void OnMouseDown(WPARAM btnState, int x, int y) override;
	virtual void OnMouseUp(WPARAM btnState, int x, int y) override;
	virtual void OnMouseMove(WPARAM btnState, int x, int y) override;

	void OnKeyboardInput(const GameTimer& gt);
	void UpdateCamera(const GameTimer& gt);
	void UpdateDepthVisualizers(const GameTimer& gt);
	void UpdateObjectCBs(const GameTimer& gt);
	void UpdateMaterialCBs(const GameTimer& gt);
	void UpdateMainPassCB(const GameTimer& gt);
	void UpdateWaves(const GameTimer& gt);
	void UpdateLightning(const GameTimer& gt);
	void AnimateMaterials(const GameTimer& gt);

	void BuildTextures();
	void BuildDescriptorHeaps();
	void BuildTextureDescriptors();

	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 1> GetStaticSamplers();
	void BuildRootSignature();
	void BuildShadersAndInputLayout();
	void BuildLandGeometry();
	void BuildWavesGeometryBuffers();
	void BuildDepthVisualizationQuads();
	void BuildTreeSpriteGeometry();
	D3D12_GRAPHICS_PIPELINE_STATE_DESC BuildOpaquePSO();
	void BuildBlendedPSO(const D3D12_GRAPHICS_PIPELINE_STATE_DESC& prevPSO);
	void BuildDepthVisualizationPSOs(const D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc);
	void BuildTreeArrayPSO(const D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc);
	void BuildPSOs();
	void BuildFrameResources();
	void BuildMaterials();
	void BuildRenderItems();
	void AddShadersToPSO(D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc, const std::string& shaderPrefix);
	void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& rItems, bool isVisualizers = false);
	float GetHillsHeight(float x, float z) const;
	XMFLOAT3 GetHillsNormal(float x, float z) const;
	void RollCamera(const GameTimer& gt, float rollTime);

private:
	std::vector<std::unique_ptr<FrameResource>> mFrameResources;
	FrameResource* mCurrFrameResource = nullptr;
	int mCurrFrameResourceIndex = 0;
	int mNumFrameResources = 3;
	UINT mCbvSrvDescriptorSize = 0;

	ComPtr<ID3D12RootSignature> mRootSignature = nullptr;
	ComPtr<ID3D12DescriptorHeap> mSrvHeap = nullptr;

	std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> mGeometries;
	std::unordered_map<std::string, std::unique_ptr<Material>> mMaterials;
	std::unordered_map<std::string, std::unique_ptr<Texture>> mTextures;
	std::unordered_map<std::string, ComPtr<ID3DBlob>> mShaders;
	std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> mPSOs;

	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayouts[(int)InputLayouts::Count];

	RenderItem* mWavesRItem = nullptr;

	UINT mCurrentLightningFrameIndex = 1;

	std::vector<std::unique_ptr<RenderItem>> mAllRItems;
	std::vector<RenderItem*>  mRitemLayer[(int)RenderLayer::Count];

	std::unique_ptr<Waves> mWaves;

	PassConstants mMainPassCB = {};

	XMFLOAT3 mEyePos = { 0.0f, 0.0f, 0.0f };
	XMFLOAT4X4 mView = MathHelper::Identity4x4();
	XMFLOAT4X4 mProj = MathHelper::Identity4x4();

	float mRolling = false;
	float mTheta = -XM_PIDIV2;
	float mPhi = XM_PIDIV2 - 0.1f;
	float mRadius = 50.0f;

	float mSunTheta = 1.25f*XM_PI;
	float mSunPhi = XM_PIDIV4;

	bool mDepthVisualizerEnabled = false;
	bool mBlendVisualizerEnabled = false;
	bool mNormalVisualizerEnabled = false;

	std::array<XMVECTORF32, 6> mDepthColors = {
		DirectX::Colors::White,		// 0
		DirectX::Colors::Blue,		// 1
		DirectX::Colors::Green,		// 2
		DirectX::Colors::Yellow,	// 3
		DirectX::Colors::Red		// 4
	};

	POINT mLastMousePos;
	std::unique_ptr<Subdivider> mSubdivider;
	std::unique_ptr<Camera> mCamera;
};

#endif