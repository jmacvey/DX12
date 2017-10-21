#ifndef BOX_DEMO_H
#define BOX_DEMO_H

#include "UploadBuffer.h"
#include "D3DApp.h"
#include "Vertex.h"
#include "GeometryGenerator.h"

using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::PackedVector;
using std::unique_ptr;
using VertexTypes::GenericVertex;
using VertexTypes::VColorData;
using VertexTypes::VPosData;

struct ObjectConstants {
	XMFLOAT4X4 WorldViewProj = MathHelper::Identity4x4();
	XMFLOAT4 PulseColor = XMFLOAT4(Colors::DarkCyan);
	float Time = 0;
};

class BoxApp : public D3DApp {
public:
	BoxApp(HINSTANCE hinstance);

	BoxApp(const BoxApp& rhs) = delete;

	BoxApp& operator=(const BoxApp& rhs) = delete;

	~BoxApp();

	virtual bool Initialize() override;

private:
	virtual void OnResize() override;
	virtual void Update(const GameTimer& gt) override;
	virtual void Draw(const GameTimer& gt) override;

	virtual void OnMouseDown(WPARAM btnState, int x, int y) override;
	virtual void OnMouseUp(WPARAM btnState, int x, int y) override;
	virtual void OnMouseMove(WPARAM btnState, int x, int y) override;

	void BuildDescriptorHeaps();
	void BuildConstantBuffers();
	void BuildRootSignature();
	void BuildShaders();
	void BuildBoxIndices();
	void BuildBoxPositions();
	void BuildBoxColors();
	void BuildBoxGeometry();
	void BuildPyramidGeometry();
	void BuildPSO();
	void SetScissorRects();
	void BuildGeometry();
	void BuildCylinder();
	void BuildSphere();
	void BuildGrid();
	void BuildBox();

private:
	ComPtr<ID3D12RootSignature> mRootSignature = nullptr;
	ComPtr<ID3D12DescriptorHeap> mCbvHeap = nullptr;
	
	// constant buffer data
	unique_ptr<UploadBuffer<ObjectConstants>> mObjectCB = nullptr;
	
	// geoemetry data
	unique_ptr<MeshGeometry> mBoxGeo = nullptr;
	unique_ptr<MeshGeometry> mBoxColorData = nullptr;
	unique_ptr<MeshGeometry> mBoxPosData = nullptr;
	unique_ptr<MeshGeometry> mPyramidData = nullptr;
	unique_ptr<MeshGeometry> mGeo = nullptr;
	unique_ptr<MeshGeometry> mCylinder = nullptr;
	unique_ptr<MeshGeometry> mSphere = nullptr;
	unique_ptr<MeshGeometry> mGrid = nullptr;
	unique_ptr<MeshGeometry> mBox = nullptr;

	// index data
	std::array<std::uint16_t, 36> mBoxIndices;
	UINT mibByteSize;

	// shader byte code
	ComPtr<ID3DBlob> mvsByteCode = nullptr;
	ComPtr<ID3DBlob> mpsByteCode = nullptr;

	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;
	
	ComPtr<ID3D12PipelineState> mPSO = nullptr;

	XMFLOAT4X4 mWorldPyramid = MathHelper::Identity4x4();
	XMFLOAT4X4 mWorldBox = XMFLOAT4X4(
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		1, 0, 2, 1
	);
	XMFLOAT4X4 mView = MathHelper::Identity4x4();
	XMFLOAT4X4 mProj = MathHelper::Identity4x4();

	float mTheta = 1.5f*XM_PI;
	float mPhi = XM_PIDIV4;
	float mRadius = 5.0f;

	POINT mLastMousePos;
};

#endif
