#include "stdafx.h"

#ifndef HEADER_GUARD_H
#define HEADER_GUARD_H

template<typename T>
UploadBuffer<T>::UploadBuffer(ID3D12Device* device, UINT elementCount, bool isConstantBuffer) :
	mIsConstantBuffer(isConstantBuffer) {

	mElementByteSize = sizeof(T);

	if (isConstantBuffer)
		mElementByteSize = d3dUtil::CalcConstantBufferByteSize(byteSize);

	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(mElementByteSize * elementCount),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&mUploadBuffer)
	));

	ThrowIfFailed(mUploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mMappedData)));
};

template<typename T>
UploadBuffer<T>::~UploadBuffer() {
	if (mUploadBuffer != nullptr)
		mUploadBuffer->Unmap(0, nullptr);

	mMappedData = nullptr;
};

#endif