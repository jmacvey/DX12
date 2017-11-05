#ifndef GROWING_PYRAMID_H
#define GROWING_PYRAMID_H

#include "D3DApp.h"
#include "GeometryGenerator.h"
#include "GameTimer.h"
#include "FrameResource.h"
#include "RenderItem.h"
#include "GeometricObject.h"

using ShapesDemo::RenderItem;

class GrowingPyramid : public D3DApp {

public:
	GrowingPyramid(HINSTANCE hInstance);
	GrowingPyramid(const GrowingPyramid& growingPyramid) = delete;
	GrowingPyramid& operator=(const GrowingPyramid& growingPyramid) = delete;

	~GrowingPyramid();
	virtual bool Initialize() override;

protected:
	void BuildRenderItems();
	void BuildGeometry();
	void BuildFrameResources();
	void BuildRootSignature();
	void BuildShadersAndInputLayout();
	void CreatePipelineStateObjects();
	void BuildDescriptorHeaps();
	void BuildCBVs();

private:

	void UpdateCamera(const GameTimer& gt);
	void UpdateObjectCBs(const GameTimer& gt);
	void UpdateMainPassCB(const GameTimer& gt);

	// Inherited via D3DApp
	virtual void Update(const GameTimer& gt) override;
	virtual void Draw(const GameTimer& gt) override;

	void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& rItems);

	virtual void OnResize() override;
	virtual void OnMouseDown(WPARAM btnState, int x, int y) override;
	virtual void OnMouseUp(WPARAM btnState, int x, int y) override;
	virtual void OnMouseMove(WPARAM btnState, int x, int y) override;

	void OnKeyboardInput(const GameTimer& gt);

private:
	
	ComPtr<ID3D12DescriptorHeap> mCbvHeap = nullptr;
	FrameResource* mCurrFrameResource = nullptr;
	std::vector<std::unique_ptr<FrameResource>> mFrameResources;
	uint16_t mCurrFrameResourceIndex = 0;
	uint16_t mNumFrameResources = 3;
	uint32_t mMainPassCbvOffset = 0;

	std::unique_ptr<GeometricObject> mObject;

	ComPtr<ID3D12RootSignature> mRootSignature;
	std::unordered_map<std::string, ComPtr<ID3DBlob>> mShaders;
	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;
	std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> mPSOs;
	std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> mGeometries;
	std::vector<std::unique_ptr<RenderItem>> mAllRenderItems;
	std::vector<RenderItem*> mOpaqueRenderItems;

	POINT mLastMousePos;
	XMFLOAT4X4 mView = MathHelper::Identity4x4();
	XMFLOAT4X4 mProj = MathHelper::Identity4x4();
	XMFLOAT3 mEyePos = { 0.0f, 0.0f, 0.0f };
	
	PassConstants mMainPassCB;

	bool mIsWireframe = false;
	float mTheta = 1.5f*XM_PI;
	float mPhi = 0.2f*XM_PI;
	float mRadius = 15.0f;
};

#endif