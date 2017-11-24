#ifndef CRATE_APP_H
#define CRATE_APP_H

#include "D3DApp.h"
#include "FrameResource.h"
#include "RenderItem.h"
#include "GeometricObject.h"
#include "GeometryGenerator.h"
#include "DDSTextureLoader.h"

using namespace CrateDemo;
using LightingDemo::RenderItem;

class CrateApp : public D3DApp {
public:
	CrateApp(HINSTANCE hInstance);
	CrateApp(const CrateApp& rhs) = delete;
	CrateApp& operator=(const CrateApp& rhs) = delete;
	~CrateApp();

	virtual bool Initialize() override;

private:
	void BuildRootSignature();
	void SetShadersAndInputLayout();

	void BuildGeometry();
	void BuildMaterials();
	void BuildRenderItems();
	void BuildDescriptorHeaps();
	void BuildConstantBuffers();
	void BuildFrameResources();
	void BuildTextures();
	void BuildTextureDescriptors();
	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 1> GetStaticSamplers();

	void BuildPSOs();
private:
	virtual void OnResize() override;
	virtual void Update(const GameTimer & gt) override;
	void OnKeyboardInput(const GameTimer& gt);
	void UpdateCamera(const GameTimer& gt);
	void UpdateObjectCBs(const GameTimer& gt);
	void UpdatePassCB(const GameTimer& gt);

	virtual void Draw(const GameTimer & gt) override;
	void DrawRenderItems(const std::vector<std::unique_ptr<RenderItem>>& rItems);

	virtual void OnMouseDown(WPARAM btnState, int x, int y);
	virtual void OnMouseUp(WPARAM btnState, int x, int y); 
	virtual void OnMouseMove(WPARAM btnState, int x, int y);

private:
	const int mNumFrameResources = 3;
	std::vector<std::unique_ptr<FrameResource>> mFrameResources;
	int mCurrentFrameIndex = 0;
	FrameResource* mCurrentFrameResource;

	std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> mGeometries;
	std::unordered_map<std::string, std::unique_ptr<Material>> mMaterials;
	std::unordered_map<std::string, std::unique_ptr<Texture>> mTextures;
	std::vector<std::unique_ptr<RenderItem>> mRenderItems;
	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;
	
	PassConstants mMainPassCB;
	int mPassCbIndexOffset = 0;
	int mTexCIndexOffset = 0;

	ComPtr<ID3D12DescriptorHeap> mCbvHeap;
	ComPtr<ID3D12DescriptorHeap> mSrvHeap;
	ComPtr<ID3D12RootSignature> mRootSignature;
	std::unordered_map<std::string, ComPtr<ID3DBlob>> mShaders;
	std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> mPSOs;


	// camera work
	XMFLOAT3 mEyePos = { 0.0f, 0.0f, 0.0f };
	XMFLOAT4X4 mView = MathHelper::Identity4x4();
	XMFLOAT4X4 mProj = MathHelper::Identity4x4();

	float mPhi = XM_PIDIV2;
	float mTheta = 0.0f;
	float mRadius = 5.0f;
	POINT mLastMousePos;

	// lighting
	float mSunPhi = XM_PIDIV4;
	float mSunTheta = XM_PIDIV4;
};

#endif