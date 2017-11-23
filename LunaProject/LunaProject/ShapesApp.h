#ifndef SHAPES_APP_H
#define SHAPES_APP_H

#include "D3DApp.h"
#include "RenderItem.h"
#include "FrameResource.h"
#include "GeometryGenerator.h"
#include "SkullGeometry.h"

using namespace LightingDemo;

class ShapesApp : public D3DApp {

public:
	ShapesApp(HINSTANCE hInstance);
	ShapesApp(const ShapesApp& rhs) = delete;
	ShapesApp& operator=(const ShapesApp& rhs) = delete;
	~ShapesApp();

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
	void UpdateObjectCBs(const GameTimer& gt);
	void UpdateMainPassCB(const GameTimer& gt);
	void AddThreePointLighting();
	void AddPointLighting();
	void AddSpotLighting();
	void UpdateMaterialCBs(const GameTimer& gt);

	void BuildDescriptorHeaps();
	void BuildConstantBufferViews();
	void BuildRootSignature();
	void BuildShadersAndInputLayout();
	void BuildShapeGeometry();
	void BuildMaterials();
	void BuildPSOs();
	void BuildFrameResources();
	void BuildRenderItems();
	void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& rItems);

private:
	std::vector<std::unique_ptr<FrameResource>> mFrameResources;
	FrameResource* mCurrFrameResource = nullptr;
	int mCurrFrameResourceIndex = 0;

	ComPtr<ID3D12RootSignature> mRootSignature = nullptr;
	ComPtr<ID3D12DescriptorHeap> mCbvHeap = nullptr;
	ComPtr<ID3D12DescriptorHeap> mSrvDescriptorHeap = nullptr;

	std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> mGeometries;
	std::unordered_map<std::string, ComPtr<ID3DBlob>> mShaders;
	std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> mPSOs;
	std::unordered_map<std::string, std::unique_ptr<Material>> mMaterials;

	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;

	// list of all render items
	std::vector<std::unique_ptr<RenderItem>> mAllRitems;
	// render items deviced by PSO
	std::vector<RenderItem*> mOpaqueRitems;
	
	PassConstants mMainPassCB;
	UINT mMainPassCbvOffset = 0;
	int mNumFrameResources = 3;

	bool mIsWireFrame = false;

	XMFLOAT3 mEyePos = { 0.0f, 0.0f, 0.0f };
	XMFLOAT4X4 mView = MathHelper::Identity4x4();
	XMFLOAT4X4 mProj = MathHelper::Identity4x4();

	float mTheta = 1.5f*XM_PI;
	float mPhi = 0.2f*XM_PI;
	float mRadius = 15.0f;
	std::vector<XMFLOAT3> mSphereLocations;

	POINT mLastMousePos;
	SkullGeometry skull;
};

#endif