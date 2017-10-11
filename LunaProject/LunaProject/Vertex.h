#ifndef VERTEX_H
#define VERTEX_H

#include "stdafx.h"
using namespace DirectX;
using namespace DirectX::PackedVector;

namespace VertexTypes {

	struct GenericVertex {
		XMFLOAT3 Pos;
		XMFLOAT4 Color;
	};

	struct VPosData {
		XMFLOAT3 pos;
	};

	struct VColorData {
		XMFLOAT4 Color;
	};

	struct EfficientColorVertex {
		XMFLOAT3 Pos;
		XMCOLOR Color;
	};

};

#endif