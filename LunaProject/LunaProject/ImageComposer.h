#pragma once
#ifndef IMAGE_COMPOSER_H

#include "stdafx.h"

using Microsoft::WRL::ComPtr;

class ImageComposer {
public:
	ImageComposer() = delete;
	ImageComposer(const ImageComposer& rhs) = delete;
	ImageComposer& operator=(const ImageComposer& rhs) = delete;
	ImageComposer(ComPtr<ID3D12Device>)
};

#endif // IMAGE_COMPOSER_H