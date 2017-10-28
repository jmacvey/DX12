#ifndef RENDER_ITEM_H
#define RENDER_ITEM_H

#include "stdafx.h"

namespace ShapesDemo {
	using namespace DirectX;

	struct RenderItem {
		inline RenderItem() { }
		inline RenderItem(UINT numFrameResources, D3D12_PRIMITIVE_TOPOLOGY primitiveType) : NumFramesDirty(numFrameResources), PrimitiveType(primitiveType) {}
		inline RenderItem(UINT numFrameResources) : NumFramesDirty(numFrameResources) {};
		// World transform for object given in local coordinate space
		XMFLOAT4X4 World = MathHelper::Identity4x4();

		int NumFramesDirty = 3;

		MeshGeometry* Geo;

		D3D12_PRIMITIVE_TOPOLOGY PrimitiveType;

		UINT IndexCount = 0;
		UINT StartIndexLocation = 0;
		int BaseVertexLocation = 0;
		UINT ObjCBIndex = 0;
	};
}

#endif
