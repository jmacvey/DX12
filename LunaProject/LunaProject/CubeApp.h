#pragma once
#ifndef CUBE_APP_H
#define CUBE_APP_H

#include "D3DApp.h"

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

enum InputLayoutType {
	Sky = 0,
	Opaque,
	Count
};

enum SkyType {
	Grass = 0,
	Desert,
	Snow,
	Sunset
};

class CubeApp : public D3DApp {
public:
	CubeApp(HINSTANCE hInstance);
	CubeApp() = delete;
	CubeApp(const CubeApp& rhs) = delete;
	CubeApp& operator=(const CubeApp& rh) = delete;

	virtual bool Initialize();

private:

	void BuildGeometry();
	void BuildRenderItems();
	void BuildDescriptorHeaps();
	void BuildDescriptors();
	void BuildInstanceDescriptors();
	void BuildMaterials();
	void CompileShadersAndInputLayout();
	void BuildRootSignature();
	void BuildCommonRootSignature();
	void BuildPSOs();
	void BuildFrameResources();
	void BuildCubeMaps();
	void BuildObjectTextures();

	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> GetStaticSamplers() const;

	// Inherited via D3DApp
	virtual void Update(const GameTimer& gt) override;
	virtual void Draw(const GameTimer& gt) override;

	void DrawRenderItems(const std::vector<RenderItem*>& renderItems, UINT instanceOffset);

	void UpdateInstances(const GameTimer& gt);
	void UpdateMainPassCB(const GameTimer& gt);

	void MarkRenderItemsDirty();

	// Convenience overrides for handling mouse input
	virtual void OnMouseDown(WPARAM btnState, int x, int y) override;
	virtual void OnMouseUp(WPARAM btnState, int x, int y) override;
	virtual void OnMouseMove(WPARAM btnState, int x, int y) override;
	virtual void OnResize() override;

	void OnKeyboardInput(const GameTimer& gt);

private:
	std::unique_ptr<Camera> mCamera;

	const int mNumFrameResources = 3;
	int mPassCbOffset = 0;
	int mCubeMapSrvOffset = 0;
	int mObjectTextureSrvOffset = 0;
	int mCurrFrameIndex = 0;
	int mNumInstances = 125;

	std::vector<std::unique_ptr<FrameResource>> mFrameResources;
	FrameResource* mCurrentFrameResource = nullptr;

	ComPtr<ID3D12DescriptorHeap> mCbvSrvHeap;
	ComPtr<ID3D12RootSignature> mRootSignature;
	ComPtr<ID3D12RootSignature> mCommonRootSignature;


	std::vector<std::unique_ptr<RenderItem>> mAllRenderItems;
	std::vector<RenderItem*> mRenderLayer[(UINT)RenderLayers::Count];

	std::unordered_map<std::string, std::unique_ptr<Texture>> mCubeMaps;
	std::unordered_map<std::string, std::unique_ptr<Texture>> mObjectTextures;
	std::vector<std::string> mCubeMapNames;
	std::vector<std::string> mObjectTextureNames;
	std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> mGeometries;
	std::unordered_map<std::string, std::unique_ptr<Material>> mMaterials;
	std::unordered_map<std::string, ComPtr<ID3DBlob>> mShaders;
	std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> mPSOs;
	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout[(UINT)InputLayoutType::Count];
	PassConstants mMainPassCB = {};

	POINT mLastMousePos = { 0, 0 };

	SkullGeometry mSkull;

	struct Vertex {
		XMFLOAT3 Position;
		XMFLOAT3 Normal;
		XMFLOAT2 TexC;
	};

	std::unordered_map<std::string, UINT> mRenderItemInstanceCounts;

	UINT mPassCbvRootParamIndex = 0;
	UINT mCubeMapSrvRootParamIndex = 0;
	UINT mInstanceDataRootParamIndex = 0;
	UINT mObjectTextureRootParamIndex = 0;
	SkyType mActiveSky = SkyType::Grass;
};

#endif