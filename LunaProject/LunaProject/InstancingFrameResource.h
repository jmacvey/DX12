#pragma once
#ifndef INSTANCE_FRAME_RESOURCE_H
#define INSTANCE_FRAME_RESOURCE_H

#include <vector>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include "MathHelper.h"
#include "d3dUtil.h"
#include "UploadBuffer.h"

using namespace DirectX;
using namespace DirectX::PackedVector;

namespace InstancingDemo {
	struct InstanceData {
		XMFLOAT4X4 World = MathHelper::Identity4x4();
		XMFLOAT4X4 TexTransform = MathHelper::Identity4x4();
		UINT MaterialIndex;
		UINT DiffuseMapIndex;
		UINT InstancePad1;
		UINT InstancePad2;
	};

	struct RenderItem {
		inline RenderItem() {};
		int NumFramesDirty = 3;

		std::string Name;
		std::string GeoName;
		// geometry data
		MeshGeometry* Geo;
		D3D12_PRIMITIVE_TOPOLOGY PrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		UINT IndexCount = 0;
		UINT StartIndexLocation = 0;
		UINT InstanceCount = 0;
		UINT StartInstanceLocation = 0;
		int BaseVertexLocation = 0;
		std::vector<InstanceData> Instances;

		BoundingBox Bounds;
		bool Visible = true;
		bool IgnoreBoundingBox = false;
		UINT DiffuseMapIndex = 0;
	};



	enum RenderLayers {
		Skulls,
		Cars,
		Opaque,
		Reflectors,
		Skies,
		Count
	};
	//
	struct PassConstants {
		XMFLOAT4X4 View = MathHelper::Identity4x4();
		XMFLOAT4X4 InvView = MathHelper::Identity4x4();
		XMFLOAT4X4 Proj = MathHelper::Identity4x4();
		XMFLOAT4X4 InvProj = MathHelper::Identity4x4();
		XMFLOAT4X4 ViewProj = MathHelper::Identity4x4();
		XMFLOAT4X4 InvViewProj = MathHelper::Identity4x4();
		XMFLOAT3 EyePos = { 0.0f, 0.0f, 0.0f };
		float cbPerObjectPad0 = 0.0f;
		XMFLOAT2 RenderTargetSize = { 0.0f, 0.0f };
		XMFLOAT2 InvRenderTargetSize = { 0.0f, 0.0f };
		float NearZ = 0.0f;;
		float FarZ = 0.0f;
		float TotalTime = 0.0f;
		float DeltaTime = 0.0f;
		XMFLOAT4 AmbientLight = { 0.0f, 0.0f, 0.0f, 0.0f };
		Light Lights[MaxLights];
	};

	struct MaterialData
	{
		DirectX::XMFLOAT4 DiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
		DirectX::XMFLOAT3 FresnelR0 = { 0.01f, 0.01f, 0.01f };
		float Roughness = 64.0f;

		// Used in texture mapping.
		DirectX::XMFLOAT4X4 MatTransform = MathHelper::Identity4x4();

		UINT DiffuseMapIndex = 0;
		UINT MaterialPad0;
		UINT MaterialPad1;
		UINT MaterialPad2;
	};

	class FrameResource {
	public:
		FrameResource(ID3D12Device* device, UINT passCount, UINT maxInstanceCount, UINT materialCount) {
			ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
				IID_PPV_ARGS(&CmdListAlloc)));

			if (passCount != 0) {
				PassCB = std::make_unique<UploadBuffer<PassConstants>>(device, passCount, true);
			}

			if (maxInstanceCount != 0) {
				InstanceCB = std::make_unique<UploadBuffer<InstanceData>>(device, maxInstanceCount, false);
			}

			if (materialCount != 0) {
				MaterialCB = std::make_unique<UploadBuffer<MaterialData>>(device, materialCount, false);
			}
		};
		FrameResource(ID3D12Device* device, UINT passCount, const std::unordered_map<std::string, UINT>& instanceData, UINT materialCount) {
			ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
				IID_PPV_ARGS(&CmdListAlloc)));

			if (passCount != 0) {
				PassCB = std::make_unique<UploadBuffer<PassConstants>>(device, passCount, true);
			}

			if (instanceData.size() != 0) {
				for (auto& tuple : instanceData) {
					RenderItemInstanceCBs[tuple.first] = std::make_unique<UploadBuffer<InstanceData>>(device, tuple.second, false);
				}
			}

			if (materialCount != 0) {
				MaterialCB = std::make_unique<UploadBuffer<MaterialData>>(device, materialCount, false);
			}
		};

		FrameResource() = delete;
		FrameResource(const FrameResource& rhs) = delete;
		FrameResource& operator=(const FrameResource& rhs) = delete;
		~FrameResource() {};

		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CmdListAlloc;

		std::unique_ptr<UploadBuffer<PassConstants>> PassCB;
		std::unique_ptr<UploadBuffer<InstanceData>> InstanceCB;
		std::unique_ptr<UploadBuffer<MaterialData>> MaterialCB;
		std::unordered_map<std::string, std::unique_ptr<UploadBuffer<InstanceData>>> RenderItemInstanceCBs;
		UINT64 Fence;

	};

};

#endif