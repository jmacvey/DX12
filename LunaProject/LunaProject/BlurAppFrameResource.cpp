#include "stdafx.h"
#include "BlurAppFrameResource.h"


BlurDemo::FrameResource::FrameResource(ID3D12Device* device, UINT passCount, UINT objectCount, UINT materialCount, UINT waveVertCount)
{
	ThrowIfFailed(device->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(CmdListAlloc.GetAddressOf())));

	//  FrameCB = std::make_unique<UploadBuffer<FrameConstants>>(device, 1, true);
	PassCB = std::make_unique<UploadBuffer<PassConstants>>(device, passCount, true);
	MaterialCB = std::make_unique<UploadBuffer<MaterialConstants>>(device, materialCount, true);
	ObjectCB = std::make_unique<UploadBuffer<ObjectConstants>>(device, objectCount, true);

	if (waveVertCount != 0) {
		WavesVB = std::make_unique<UploadBuffer<Vertex>>(device, waveVertCount, false);
	}
}

BlurDemo::FrameResource::~FrameResource()
{

}