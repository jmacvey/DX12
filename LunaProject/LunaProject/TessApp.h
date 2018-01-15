#pragma once
#ifndef TESS_APP_H
#define TESS_APP_H

#include "FrameResource.h"
#include "D3DApp.h"
#include "RenderItem.h"
#include "GeometryGenerator.h"
#include "DDSTextureLoader.h"

using Microsoft::WRL::ComPtr;
using LightingDemo::TessFrameResource;
using LightingDemo::RenderItem;
using LightingDemo::TessLayer;
using LightingDemo::PassConstantsThrough;
using LightingDemo::ObjectConstants;

class TessApp : public D3DApp {
public:
	TessApp() = delete;
	TessApp(HINSTANCE hInstance);
	TessApp(const TessApp& rhs) = delete;
	TessApp& operator=(const TessApp& rhs) = delete;
	
	void BuildRootSignature();
	void BuildShadersAndInputLayout();
	void BuildPSOs();
	void BuildFrameResources();
	void BuildDescriptorHeaps();
	void BuildDescriptors();
	void BuildShaderResourceViews();
	void BuildGridGeometry();
	void BuildRenderItems();
	void BuildMaterials();
	void BuildTextures();

	void UpdateCB(const GameTimer& gt);
	void UpdatePassCB(const GameTimer& gt);

	virtual bool Initialize() override;
public:

	// Inherited via D3DApp
	virtual void Update(const GameTimer& gt) override;
	virtual void Draw(const GameTimer& gt) override;
	virtual void OnResize() override;
	
	virtual void OnMouseUp(WPARAM btnState, int x, int y);
	virtual void OnMouseDown(WPARAM btnState, int x, int y);
	virtual void OnMouseMove(WPARAM btnState, int x, int y);
	void HandleKeyboardInput(const GameTimer& gt);

	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 1> GetStaticSamplers() const;

private:
	void UpdateCamera(const GameTimer& gt);
	void ToggleRenderLayer(TessLayer rLayer, const std::string& psoStr);

private:
	ComPtr<ID3D12RootSignature> mRootSignature;
	std::unordered_map<std::string, ComPtr<ID3DBlob>> mShaders;
	std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> mPSOs;
	std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> mGeometries;
	std::unordered_map<std::string, std::unique_ptr<Material>> mMaterials;
	std::unordered_map<std::string, std::unique_ptr<Texture>> mTextures;

	int mMainPassCbOffset = 0;
	int mMatCbOffset = 0;
	int mTextureOffset = 0;
	const int mNumFrameResources = 3;
	int mCurrentFrameResourceIndex = 0;
	std::vector<std::unique_ptr<TessFrameResource>> mFrameResources;
	TessFrameResource* mCurrentFrameResource = nullptr;
	PassConstantsThrough mPassConstants;

	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;
	std::vector<std::unique_ptr<RenderItem>> mRenderItems;
	std::vector<RenderItem*> mRenderLayers[(UINT)TessLayer::Count];

	ComPtr<ID3D12DescriptorHeap> mCbvHeap;

private:
	XMFLOAT4X4 mView = MathHelper::Identity4x4();
	XMFLOAT4X4 mProj = MathHelper::Identity4x4();
	XMFLOAT4X4 mViewProj = MathHelper::Identity4x4();
	XMFLOAT3 mEyePos = { 0.0f, 0.0f, 0.0f };

private:
	float mPhi = XM_PIDIV2;
	float mTheta = XM_PIDIV4;
	float mRadius = 20.0f;
	POINT mLastMousePos = { 0, 0 };
	float mNearZ = 5.0f;
	float mFarZ = 1000.0f;

	float mSunTheta = 1.25f*XM_PI;
	float mSunPhi = XM_PIDIV4;

	struct Vertex {
		XMFLOAT3 Position;
	};

	TessLayer mActiveRenderLayer = TessLayer::Opaque;
	ComPtr<ID3D12PipelineState> mActivePSO;
};

#endif // TESS_APP_H