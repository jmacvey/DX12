#pragma once
#ifndef CUBE_APP_H
#define CUBE_APP_H

#include "D3DApp.h"

#include "Camera.h"
#include "D3DApp.h"
#include "InstancingFrameResource.h"
#include <DirectXCollision.h>
#include "SkullGeometry.h"
#include "DynamicCubeMap.h"
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
	void BuildCubeAxes();
	void CompileShadersAndInputLayout();
	void BuildRootSignature();
	void BuildCommonRootSignature();
	void BuildPSOs();
	void BuildFrameResources();
	void BuildCubeMaps();
	void BuildObjectTextures();
	void BuildCubeMapDsv();

	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> GetStaticSamplers() const;

	// Inherited via D3DApp
	virtual void Update(const GameTimer& gt) override;
	virtual void Draw(const GameTimer& gt) override;

	void DrawSceneToCubeMap();
	void DrawRenderItems(const std::vector<RenderItem*>& renderItems, UINT instanceOffset, CD3DX12_GPU_DESCRIPTOR_HANDLE* srvHandle = nullptr);

	void UpdateInstances(const GameTimer& gt);
	void UpdateMainPassCB(const GameTimer& gt);
	void UpdateCubeMapPassCBs(const GameTimer& gt);
	void UpdateLights();
	void UpdateCubeCams(float x, float y, float z);

	void MarkRenderItemsDirty();

	// Convenience overrides for handling mouse input
	virtual void OnMouseDown(WPARAM btnState, int x, int y) override;
	virtual void OnMouseUp(WPARAM btnState, int x, int y) override;
	virtual void OnMouseMove(WPARAM btnState, int x, int y) override;
	virtual void OnResize() override;

	virtual void CreateRtvAndDsvDescriptorHeaps() override;
	void OnKeyboardInput(const GameTimer& gt);
	void AnimateSphere(RenderItem* mRenderItem, const GameTimer& gt);

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

	ComPtr<ID3D12Resource> mCubeMapDsv;
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
	UINT mDynamicCubeMapSrvOffset = 0;
	UINT mNumPassConstantBuffers = 7;
	SkyType mActiveSky = SkyType::Grass;
	float mSunPhi = XM_PI;

	CD3DX12_CPU_DESCRIPTOR_HANDLE mhCubeMapDsv;
	std::unique_ptr<DynamicCubeMap> mDynamicCubeMap = nullptr;
	UINT mCubeMapSize = 0;
	DXGI_FORMAT mCubeMapFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	Camera mCubeCameras[6];
	XMFLOAT3 mCubeAxes[6];
	XMFLOAT3 mCenterSpherePos = { 0.0f, 15.0f, 0.0f };
	UINT mPathRadius = 10.0f;
	XMFLOAT4X4 spherePositions[10];
};

#endif