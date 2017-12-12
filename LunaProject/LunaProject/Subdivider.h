#ifndef  SUBDIVIDER_H
#define SUBDIVIDER_H

#include "D3DApp.h"
#include "GeometryGenerator.h"
#include "FrameResource.h"

using CrateDemo::LocalVertex;

class Subdivider {
public:
	Subdivider(ComPtr<ID3D12GraphicsCommandList> cmdList, ComPtr<ID3D12Device> device);
	Subdivider& operator=(const Subdivider& rhs) = delete;
	Subdivider(const Subdivider& rhs) = delete;
	~Subdivider();

	void BuildGeometry();
	inline MeshGeometry* GetGeometry() { return mGeo.get(); }
	inline SubmeshGeometry GetSubmesh(const std::string& name) {
		return mGeo->DrawArgs[name];
	}

	D3D12_INPUT_LAYOUT_DESC GetInputLayout() const;

private:
	ComPtr<ID3D12GraphicsCommandList> mCmdList;
	ComPtr<ID3D12Device> mDevice;
	
	std::unique_ptr<MeshGeometry> mGeo;

	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;
};

#endif // ! SUBDIVIDER_H
