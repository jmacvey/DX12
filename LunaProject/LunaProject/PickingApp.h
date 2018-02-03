#pragma once
#ifndef PICKING_APP_H
#define PICKING_APP_H

#include "Camera.h"
#include "D3DApp.h"
#include "InstancingFrameResource.h"
#include <DirectXCollision.h>
#include "SkullGeometry.h"
#include "CarGeometry.h"
#include <vector>


using namespace InstancingDemo;
using namespace DirectX;
using namespace DirectX::PackedVector;

class PickingApp : public D3DApp {
public:
	PickingApp(HINSTANCE hInstance);
	PickingApp() = delete;
	PickingApp(const PickingApp& rhs) = delete;
	PickingApp& operator=(const PickingApp& rh) = delete;

	virtual bool Initialize();

	void BuildGeometry();
	void BuildRenderItems();
	void BuildDescriptorHeaps();
	void BuildDescriptors();
	void BuildCarInstance(RenderItem* rItem);
	void BuildInstanceDescriptors();
	void BuildMaterials();
	void CompileShadersAndInputLayout();
	void BuildRootSignature();
	void BuildPSOs();
	void BuildFrameResources();

	// Inherited via D3DApp
	virtual void Update(const GameTimer& gt) override;
	virtual void Draw(const GameTimer& gt) override;

	void DrawRenderItems(const std::vector<RenderItem*>& renderItems);
	void TryDrawPickedTriangle();

	void UpdateInstances(const GameTimer& gt);
	void UpdateMainPassCB(const GameTimer& gt);

	void MarkRenderItemsDirty();

	// Convenience overrides for handling mouse input
	virtual void OnMouseDown(WPARAM btnState, int x, int y) override;
	virtual void OnMouseUp(WPARAM btnState, int x, int y) override;
	virtual void OnMouseMove(WPARAM btnState, int x, int y) override;
	virtual void OnResize() override;

	void OnKeyboardInput(const GameTimer& gt);

	void Pick(int sx, int sy);

private:
	std::unique_ptr<Camera> mCamera;

	const int mNumFrameResources = 3;
	int mPassCbOffset = 0;
	int mCurrFrameIndex = 0;
	int mNumInstances = 125;

	std::vector<std::unique_ptr<FrameResource>> mFrameResources;
	FrameResource* mCurrentFrameResource = nullptr;

	ComPtr<ID3D12DescriptorHeap> mCbvSrvHeap;
	ComPtr<ID3D12RootSignature> mRootSignature;


	std::vector<std::unique_ptr<RenderItem>> mAllRenderItems;
	std::vector<RenderItem*> mRenderLayer[(UINT)RenderLayers::Count];

	std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> mGeometries;
	std::unordered_map<std::string, std::unique_ptr<Material>> mMaterials;
	std::unordered_map<std::string, ComPtr<ID3DBlob>> mShaders;
	std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> mPSOs;
	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;

	PassConstants mMainPassCB = {};

	POINT mLastMousePos = { 0, 0 };

	SkullGeometry mSkull;
	CarGeometry mCar;
	RenderItem* mPickedTriangle;
	XMFLOAT4 mPickingRayOrigin = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	XMFLOAT4 mPickingRayDir = XMFLOAT4(0.0f, 0.0f, 1.0f, 0.0f);

	struct Vertex {
		XMFLOAT3 Position;
	};
};


#endif