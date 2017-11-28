#ifndef LIT_COLUMNS_H
#define LIT_COLUMNS_H

#include "D3DApp.h"
#include "RenderItem.h"
#include "FrameResource.h"
#include "GeometryGenerator.h"
#include "GeometricObject.h"

using namespace CrateDemo;
using LightingDemo::RenderItem;

class LitColumns : public D3DApp {
public:
	LitColumns(HINSTANCE hInstance);
	LitColumns(const LitColumns& rhs) = delete;
	LitColumns& operator=(const LitColumns& rhs) = delete;
	~LitColumns();

	virtual bool Initialize() override;

private:
	virtual void OnResize() override;
	void OnMouseDown(WPARAM btnState, int x, int y);
	void OnMouseUp(WPARAM btnState, int x, int y);
	void OnMouseMove(WPARAM btnState, int x, int y);
	void OnKeyboardInput(const GameTimer& gt);
	virtual void Update(const GameTimer& gt) override;
	virtual void Draw(const GameTimer & gt) override;

	void UpdateCamera(const GameTimer& gt);

	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 1> GetStaticSamplers();

	void BuildRootSignature();
	void BuildDescriptorHeaps();
	void BuildGeometry();
	void BuildRenderItems();
	void BuildTextures();
	void SetShadersAndInputLayout();
	void BuildFrameResources();
	void BuildConstantBuffers();
	void BuildTextureDescriptors();
	void BuildMaterials();
	void BuildPSOs();

	void UpdateObjectCBs(const GameTimer& gt);
	void UpdateMaterialCBs(const GameTimer& gt);
	void UpdatePassCB(const GameTimer& gt);

	void DrawRenderItems(const std::vector<std::unique_ptr<RenderItem>>& renderItems);

private:
	XMFLOAT4X4 mProj = MathHelper::Identity4x4();
	XMFLOAT4X4 mView = MathHelper::Identity4x4();
	XMFLOAT3 mEyePos = { 0.0f, 0.0f, -1.0f };
	ComPtr<ID3D12DescriptorHeap> mCbvHeap;
	ComPtr<ID3D12RootSignature> mRootSignature;

	// frame resource is going to be circular array
	FrameResource* mCurrFrameResource = nullptr;
	int mCurrFrameResourceIndex = 0;
	std::vector<std::unique_ptr<FrameResource>> mFrameResources;
	const int mNumFrameResources = 3;

	std::unordered_map<std::string, ComPtr<ID3DBlob>> mShaders;
	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;

	std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> mPSOs;

	PassConstants mMainPassCB;
	uint32_t mPassCbIndexOffset = 0;
	uint32_t mMatCbIndexOffset = 0;
	uint32_t mTextureCbIndexOffset = 0;

	std::unique_ptr<GeometricObject> mGeometry;
	std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> mGeometries;
	std::unordered_map<std::string, std::unique_ptr<Material>> mMaterials;
	std::unordered_map<std::string, std::unique_ptr<Texture>> mTextures;
	std::vector<std::string> mTextureNames;

	std::vector<std::unique_ptr<RenderItem>> mAllRenderItems;

	float mTheta = 1.5f*XM_PI;
	float mPhi = 0.2f*XM_PI;
	float mRadius = 15.0f;
	POINT mLastMousePos;

	float mSunTheta = XM_PI;
	float mSunPhi = 0.0f;
};

#endif
