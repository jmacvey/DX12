#include "stdafx.h"

#ifndef LUNA_UTILITY_H
#define LUNA_UTILITY_H

#include "d3dUtil.h"

using Microsoft::WRL::ComPtr;

template<typename T>
class UploadBuffer {
public:

	UploadBuffer(ID3D12Device* device, UINT elementCount, bool isConstantBuffer) : mIsConstantBuffer(isConstantBuffer) {

		mElementByteSize = sizeof(T);

		if (isConstantBuffer)
			mElementByteSize = d3dUtil::CalcConstantBufferByteSize(mElementByteSize);

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

	UploadBuffer(const UploadBuffer& rhs) = delete;
	UploadBuffer& operator=(const UploadBuffer& rhs) = delete;

	ID3D12Resource* Resource() const {
		return mUploadBuffer.Get();
	}

	void CopyData(int elementIndex, const T& data) {
		memcpy(&mMappedData[elementIndex], &data, sizeof(T));
	}

	~UploadBuffer() {
		if (mUploadBuffer != nullptr) {
			mUploadBuffer->Unmap(0, nullptr);
			mUploadBuffer = nullptr;
		}
	}
private:

	ComPtr<ID3D12Resource> mUploadBuffer;
	BYTE* mMappedData = nullptr;

	UINT mElementByteSize = 0;
	bool mIsConstantBuffer = false;
};

#endif