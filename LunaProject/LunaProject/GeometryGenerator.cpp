#include "stdafx.h"
#include "GeometryGenerator.h"

GeometryGenerator::MeshData GeometryGenerator::CreateBox(float width, float height, float depth, uint32 numSubdivisions) {
	//auto halfWidth = 0.5f*width;
	//auto halfHeight = 0.5f*height;
	//auto halfDepth = 0.5f*depth;

	//Vertex v[24];
	//XMFLOAT3 normal(0.0f, 0.0f, -1.0f);
	//XMFLOAT3 tangent(1.0f, 0.0f, 0.0f);

	//// front 
	//v[0] = Vertex(-halfWidth, -halfHeight, -halfDepth, normal.x, normal.y, normal.z, tangent.x, tangent.y, tangent.z, 0.0f, 1.0f);
	//v[1] = Vertex(-halfWidth, +halfHeight, -halfDepth, normal.x, normal.y, normal.z, tangent.x, tangent.y, tangent.z, 0.0f, 0.0f);
	//v[2] = Vertex(+halfWidth, +halfHeight, -halfDepth, normal.x, normal.y, normal.z, tangent.y, tangent.y, tangent.z, 1.0f, 0.0f);
	//v[3] = Vertex(+halfWidth, -halfHeight, -halfDepth, normal.x, normal.y, normal.z, tangent.x, tangent.y, tangent.z, 1.0f, 1.0f);

	//// back
	//normal.z = 1.0f;
	//tangent.x = -1.0f;
	//v[4] = Vertex(-halfWidth, -halfHeight, +halfDepth, normal.x, normal.y, normal.z, tangent.x, tangent.y, tangent.z, 1.0f, 1.0f);
	//v[5] = Vertex(+halfWidth, -halfHeight, +halfDepth, normal.x, normal.y, normal.z, tangent.x, tangent.y, tangent.z, 0.0f, 1.0f);
	//v[6] = Vertex(+halfWidth, +halfHeight, +halfDepth, normal.x, normal.y, normal.z, tangent.x, tangent.y, tangent.z, 0.0f, 0.0f);
	//v[7] = Vertex(-halfWidth, +halfHeight, +halfDepth, normal.x, normal.y, normal.z, tangent.x, tangent.y, tangent.z, 1.0f, 0.0f);

	//// top
	//normal.x = 0.0f;
	//normal.y = 1.0f;
	//tangent.x = 1.0f;
	//v[8] = Vertex(-halfWidth, +halfHeight, -halfDepth, normal.x, normal.y, normal.z, tangent.x, tangent.y, tangent.z, 0.0f, 1.0f);
	//v[9] = Vertex(-halfWidth, +halfHeight, +halfDepth, normal.x, normal.y, normal.z, tangent.x, tangent.y, tangent.z, 0.0f, 0.0f);
	//v[10] = Vertex(+halfWidth, +halfHeight, +halfDepth, normal.x, normal.y, normal.z, tangent.x, tangent.y, tangent.z, 1.0f, 0.0f);
	//v[11] = Vertex(+halfWidth, +halfHeight, -halfDepth, normal.x, normal.y, normal.z, tangent.x, tangent.y, tangent.z, 1.0f, 1.1f);

	//// bottom
	//normal.y = -1.0f;
	//tangent.x = -1.0f;
	//v[12] = Vertex(-halfWidth, -halfHeight, -halfDepth, normal.x, normal.y, normal.z, tangent.x, tangent.y, tangent.z, 1.0f, 1.0f);
	//v[13] = Vertex(+halfWidth, -halfHeight, -halfDepth, normal.x, normal.y, normal.z, tangent.x, tangent.y, tangent.z, 0.0f, 1.0f);
	//v[14] = Vertex(+halfWidth, -halfHeight, +halfDepth, normal.x, normal.y, normal.z, tangent.x, tangent.y, tangent.z, 0.0f, 0.0f);
	//v[15] = Vertex(-halfWidth, -halfHeight, +halfDepth, normal.x, normal.y, normal.z, tangent.x, tangent.y, tangent.z, 1.0f, 0.0f);

	//// left
	//tangent.x = 0.0f;
	//tangent.z = -1.0f;
	//normal.y = 0.0f;
	//normal.x = -1.0f;
	//v[16] = Vertex(-halfWidth, -halfHeight, +halfDepth, normal.x, normal.y, normal.z, tangent.x, tangent.y, tangent.z, 0.0f, 1.0f);
	//v[17] = Vertex(-halfWidth, +halfHeight, +halfDepth, normal.x, normal.y, normal.z, tangent.y, tangent.y, tangent.z, 0.0f, 0.0f);
	//v[18] = Vertex(-halfWidth, +halfHeight, -halfDepth, normal.x, normal.y, normal.z, tangent.x, tangent.y, tangent.z, 1.0f, 0.0f);
	//v[19] = Vertex(-halfWidth, -halfHeight, -halfDepth, normal.x, normal.y, normal.z, tangent.x, tangent.y, tangent.z, 1.0f, 1.0f);

	//// right
	//tangent.z = 1.0f;
	//normal.x = 1.0f;
	//v[20] = Vertex(+halfWidth, -halfHeight, -halfDepth, normal.x, normal.y, normal.z, tangent.x, tangent.y, tangent.z, 0.0f, 1.0f);
	//v[21] = Vertex(+halfWidth, +halfHeight, -halfDepth, normal.x, normal.y, normal.z, tangent.x, tangent.y, tangent.z, 0.0f, 0.0f);
	//v[22] = Vertex(+halfWidth, +halfHeight, +halfDepth, normal.x, normal.y, normal.z, tangent.x, tangent.y, tangent.z, 1.0f, 0.0f);
	//v[23] = Vertex(+halfWidth, -halfHeight, +halfDepth, normal.x, normal.y, normal.z, tangent.x, tangent.y, tangent.z, 1.0f, 1.0f);

	////
	//MeshData meshData;
	//meshData.Vertices.assign(&v[0], &v[24]);

	//// indices

	//uint32 i[36] = {
	//	// front
	//	0, 1, 2,
	//	0, 2, 3,

	//	// back
	//	4, 5, 6,
	//	4, 6, 7,

	//	// front
	//	8, 9, 10,
	//	8, 10, 11,

	//	// bottom
	//	12, 13, 14,
	//	12, 14, 15,

	//	// left
	//	16, 17, 18,
	//	16, 18, 19,

	//	// right
	//	20, 21, 22,
	//	20, 22, 23
	//};

	//meshData.Indices32.assign(&i[0], &i[36]);

	//numSubdivisions = std::min<uint32>(numSubdivisions, 6u);

	//for (uint32 i = 0; i < numSubdivisions; ++i) {
	//	Subdivide(meshData);
	//}
	//return meshData;

	MeshData meshData;

	//
	// Create the vertices.
	//

	Vertex v[24];

	float w2 = 0.5f*width;
	float h2 = 0.5f*height;
	float d2 = 0.5f*depth;

	// Fill in the front face vertex data.
	v[0] = Vertex(-w2, -h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	v[1] = Vertex(-w2, +h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	v[2] = Vertex(+w2, +h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
	v[3] = Vertex(+w2, -h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f);

	// Fill in the back face vertex data.
	v[4] = Vertex(-w2, -h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f);
	v[5] = Vertex(+w2, -h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	v[6] = Vertex(+w2, +h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	v[7] = Vertex(-w2, +h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f);

	// Fill in the top face vertex data.
	v[8] = Vertex(-w2, +h2, -d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	v[9] = Vertex(-w2, +h2, +d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	v[10] = Vertex(+w2, +h2, +d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
	v[11] = Vertex(+w2, +h2, -d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f);

	// Fill in the bottom face vertex data.
	v[12] = Vertex(-w2, -h2, -d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f);
	v[13] = Vertex(+w2, -h2, -d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	v[14] = Vertex(+w2, -h2, +d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	v[15] = Vertex(-w2, -h2, +d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f);

	// Fill in the left face vertex data.
	v[16] = Vertex(-w2, -h2, +d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f);
	v[17] = Vertex(-w2, +h2, +d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f);
	v[18] = Vertex(-w2, +h2, -d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f);
	v[19] = Vertex(-w2, -h2, -d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f);

	// Fill in the right face vertex data.
	v[20] = Vertex(+w2, -h2, -d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f);
	v[21] = Vertex(+w2, +h2, -d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
	v[22] = Vertex(+w2, +h2, +d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f);
	v[23] = Vertex(+w2, -h2, +d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);

	meshData.Vertices.assign(&v[0], &v[24]);

	//
	// Create the indices.
	//

	uint32 i[36];

	// Fill in the front face index data
	i[0] = 0; i[1] = 1; i[2] = 2;
	i[3] = 0; i[4] = 2; i[5] = 3;

	// Fill in the back face index data
	i[6] = 4; i[7] = 5; i[8] = 6;
	i[9] = 4; i[10] = 6; i[11] = 7;

	// Fill in the top face index data
	i[12] = 8; i[13] = 9; i[14] = 10;
	i[15] = 8; i[16] = 10; i[17] = 11;

	// Fill in the bottom face index data
	i[18] = 12; i[19] = 13; i[20] = 14;
	i[21] = 12; i[22] = 14; i[23] = 15;

	// Fill in the left face index data
	i[24] = 16; i[25] = 17; i[26] = 18;
	i[27] = 16; i[28] = 18; i[29] = 19;

	// Fill in the right face index data
	i[30] = 20; i[31] = 21; i[32] = 22;
	i[33] = 20; i[34] = 22; i[35] = 23;

	meshData.Indices32.assign(&i[0], &i[36]);

	// Put a cap on the number of subdivisions.
	numSubdivisions = std::min<uint32>(numSubdivisions, 6u);

	for (uint32 i = 0; i < numSubdivisions; ++i)
		Subdivide(meshData);

	return meshData;
}

GeometryGenerator::MeshData GeometryGenerator::CreateSphere(float radius, uint32 sliceCount, uint32 stackCount) {
	MeshData meshData;

	//
	// Compute the vertices stating at the top pole and moving down the stacks.
	//

	// Poles: note that there will be texture coordinate distortion as there is
	// not a unique point on the texture map to assign to the pole when mapping
	// a rectangular texture onto a sphere.
	Vertex topVertex(0.0f, +radius, 0.0f, 0.0f, +1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	Vertex bottomVertex(0.0f, -radius, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);

	meshData.Vertices.push_back(topVertex);

	float phiStep = XM_PI / stackCount;
	float thetaStep = 2.0f*XM_PI / sliceCount;

	// Compute vertices for each stack ring (do not count the poles as rings).
	for (uint32 i = 1; i <= stackCount - 1; ++i)
	{
		float phi = i*phiStep;

		// Vertices of ring.
		for (uint32 j = 0; j <= sliceCount; ++j)
		{
			float theta = j*thetaStep;

			Vertex v;

			// spherical to cartesian
			v.Position.x = radius*sinf(phi)*cosf(theta);
			v.Position.y = radius*cosf(phi);
			v.Position.z = radius*sinf(phi)*sinf(theta);

			// Partial derivative of P with respect to theta
			v.TangentU.x = -radius*sinf(phi)*sinf(theta);
			v.TangentU.y = 0.0f;
			v.TangentU.z = +radius*sinf(phi)*cosf(theta);

			XMVECTOR T = XMLoadFloat3(&v.TangentU);
			XMStoreFloat3(&v.TangentU, XMVector3Normalize(T));

			XMVECTOR p = XMLoadFloat3(&v.Position);
			XMStoreFloat3(&v.Normal, XMVector3Normalize(p));

			v.TexC.x = theta / XM_2PI;
			v.TexC.y = phi / XM_PI;

			meshData.Vertices.push_back(v);
		}
	}

	meshData.Vertices.push_back(bottomVertex);

	//
	// Compute indices for top stack.  The top stack was written first to the vertex buffer
	// and connects the top pole to the first ring.
	//

	for (uint32 i = 1; i <= sliceCount; ++i)
	{
		meshData.Indices32.push_back(0);
		meshData.Indices32.push_back(i + 1);
		meshData.Indices32.push_back(i);
	}

	//
	// Compute indices for inner stacks (not connected to poles).
	//

	// Offset the indices to the index of the first vertex in the first ring.
	// This is just skipping the top pole vertex.
	uint32 baseIndex = 1;
	uint32 ringVertexCount = sliceCount + 1;
	for (uint32 i = 0; i < stackCount - 2; ++i)
	{
		for (uint32 j = 0; j < sliceCount; ++j)
		{
			meshData.Indices32.push_back(baseIndex + i*ringVertexCount + j);
			meshData.Indices32.push_back(baseIndex + i*ringVertexCount + j + 1);
			meshData.Indices32.push_back(baseIndex + (i + 1)*ringVertexCount + j);

			meshData.Indices32.push_back(baseIndex + (i + 1)*ringVertexCount + j);
			meshData.Indices32.push_back(baseIndex + i*ringVertexCount + j + 1);
			meshData.Indices32.push_back(baseIndex + (i + 1)*ringVertexCount + j + 1);
		}
	}

	//
	// Compute indices for bottom stack.  The bottom stack was written last to the vertex buffer
	// and connects the bottom pole to the bottom ring.
	//

	// South pole vertex was added last.
	uint32 southPoleIndex = (uint32)meshData.Vertices.size() - 1;

	// Offset the indices to the index of the first vertex in the last ring.
	baseIndex = southPoleIndex - ringVertexCount;

	for (uint32 i = 0; i < sliceCount; ++i)
	{
		meshData.Indices32.push_back(southPoleIndex);
		meshData.Indices32.push_back(baseIndex + i);
		meshData.Indices32.push_back(baseIndex + i + 1);
	}

	return meshData;
}

GeometryGenerator::MeshData GeometryGenerator::CreateGeosphere(float radius, uint32 numSubdivisions) {
	MeshData sphere;

	const uint32 division = std::min<uint32>(numSubdivisions, 6u);

	// create icosahedron
	const float X = 0.525731f;
	const float Z = 0.850651f;

	XMFLOAT3 pos[12] =
	{
		XMFLOAT3(-X, 0.0f, Z),  XMFLOAT3(X, 0.0f, Z),
		XMFLOAT3(-X, 0.0f, -Z), XMFLOAT3(X, 0.0f, -Z),
		XMFLOAT3(0.0f, Z, X),   XMFLOAT3(0.0f, Z, -X),
		XMFLOAT3(0.0f, -Z, X),  XMFLOAT3(0.0f, -Z, -X),
		XMFLOAT3(Z, X, 0.0f),   XMFLOAT3(-Z, X, 0.0f),
		XMFLOAT3(Z, -X, 0.0f),  XMFLOAT3(-Z, -X, 0.0f)
	};

	uint32 k[60] =
	{
		1,4,0,  4,9,0,  4,5,9,  8,5,4,  1,8,4,
		1,10,8, 10,3,8, 8,3,5,  3,2,5,  3,7,2,
		3,10,7, 10,6,7, 6,11,7, 6,0,11, 6,1,0,
		10,1,6, 11,0,9, 2,11,9, 5,2,9,  11,2,7
	};

	sphere.Vertices.resize(12);
	sphere.Indices32.assign(&k[0], &k[60]);
	for (int i = 0; i < 12; ++i) {
		sphere.Vertices[i].Position = pos[i];
	}

	for (uint32 i = 0; i < division; ++i) {
		Subdivide(sphere);
	}

	uint32 sz = sphere.Vertices.size();
	for (uint32 i = 0; i < sz; ++i) {
		// project the vertices onto sphere
		auto normal = XMVector3Normalize(XMLoadFloat3(&sphere.Vertices[i].Position));
		XMVECTOR p = radius*normal;
		XMStoreFloat3(&sphere.Vertices[i].Position, p);
		XMStoreFloat3(&sphere.Vertices[i].Normal, normal);

		// Derive texture coordinates from spherical coordinates.
		float theta = atan2f(sphere.Vertices[i].Position.z, sphere.Vertices[i].Position.x);

		// Put in [0, 2pi].
		if (theta < 0.0f)
			theta += XM_2PI;

		float phi = acosf(sphere.Vertices[i].Position.y / radius);

		sphere.Vertices[i].TexC.x = theta / XM_2PI;
		sphere.Vertices[i].TexC.y = phi / XM_PI;

		// Partial derivative of P with respect to theta
		sphere.Vertices[i].TangentU.x = -radius*sinf(phi)*sinf(theta);
		sphere.Vertices[i].TangentU.y = 0.0f;
		sphere.Vertices[i].TangentU.z = +radius*sinf(phi)*cosf(theta);

		XMVECTOR T = XMLoadFloat3(&sphere.Vertices[i].TangentU);
		XMStoreFloat3(&sphere.Vertices[i].TangentU, XMVector3Normalize(T));
	}
	return sphere;
}

GeometryGenerator::MeshData GeometryGenerator::CreateCylinder(float bottomRadius, float topRadius, float height, uint32 sliceCount, uint32 stackCount) {

	MeshData cylinder;
	const float heightOffset = -0.5f * height;
	const float dh = height / stackCount;
	const float dr = (topRadius - bottomRadius) / stackCount;
	const float dTheta = XM_2PI / sliceCount;

	const uint32 ringCount = stackCount + 1;
	float r, x, z, y, s, c;
	for (uint32 i = 0; i < ringCount; ++i) {
		r = dr * i + bottomRadius;
		y = dh * i + heightOffset;
		for (uint32 j = 0; j <= sliceCount; ++j) {
			c = cosf(dTheta * j);
			s = sinf(dTheta * j);
			x = r*c;
			z = r*s;
			Vertex vertex;
			vertex.Position = XMFLOAT3(x, y, z);
			vertex.TexC.x = (float)j / sliceCount;
			vertex.TexC.y = (float)i / stackCount;
			vertex.TangentU = XMFLOAT3(-s, 0.0f, c);

			float dr = bottomRadius - topRadius;
			XMFLOAT3 bitangent(dr*c, -height, dr*s);

			XMVECTOR T = XMLoadFloat3(&vertex.TangentU);
			XMVECTOR B = XMLoadFloat3(&bitangent);
			XMVECTOR N = XMVector3Normalize(XMVector3Cross(T, B));
			XMStoreFloat3(&vertex.Normal, N);
			cylinder.Vertices.emplace_back(vertex);
		}

		uint32 ringVertexCount = sliceCount + 1;
		for (uint32 i = 0; i < ringCount; ++i) {
			for (uint32 j = 0; j < sliceCount; ++j) {
				uint32 bottomLeft = i*ringVertexCount + j;
				uint32 topLeft = (i + 1)*ringVertexCount + j;
				cylinder.Indices32.emplace_back(bottomLeft);
				cylinder.Indices32.emplace_back(topLeft);
				cylinder.Indices32.emplace_back(topLeft + 1);

				cylinder.Indices32.emplace_back(bottomLeft);
				cylinder.Indices32.emplace_back(topLeft + 1);
				cylinder.Indices32.emplace_back(bottomLeft + 1);
			}
		}
	}

	BuildCylinderTopCap(bottomRadius, topRadius, height, sliceCount, stackCount, cylinder);
	BuildCylinderBottomCap(bottomRadius, topRadius, height, sliceCount, stackCount, cylinder);


	return cylinder;
}

GeometryGenerator::MeshData GeometryGenerator::CreateGrid(float width, float depth, uint32 m, uint32 n) {
	MeshData meshData;

	uint32 vertexCount = m*n;
	uint32 faceCount = (m - 1)*(n - 1) * 2;

	//
	// Create the vertices.
	//

	float halfWidth = 0.5f*width;
	float halfDepth = 0.5f*depth;

	float dx = width / (n - 1);
	float dz = depth / (m - 1);

	float du = 1.0f / (n - 1);
	float dv = 1.0f / (m - 1);

	meshData.Vertices.resize(vertexCount);
	for (uint32 i = 0; i < m; ++i)
	{
		float z = halfDepth - i*dz;
		for (uint32 j = 0; j < n; ++j)
		{
			float x = -halfWidth + j*dx;

			meshData.Vertices[i*n + j].Position = XMFLOAT3(x, 0.0f, z);
			meshData.Vertices[i*n + j].Normal = XMFLOAT3(0.0f, 1.0f, 0.0f);
			meshData.Vertices[i*n + j].TangentU = XMFLOAT3(1.0f, 0.0f, 0.0f);

			// Stretch texture over grid.
			meshData.Vertices[i*n + j].TexC.x = j*du;
			meshData.Vertices[i*n + j].TexC.y = i*dv;
		}
	}

	//
	// Create the indices.
	//

	meshData.Indices32.resize(faceCount * 3); // 3 indices per face

											  // Iterate over each quad and compute indices.
	uint32 k = 0;
	for (uint32 i = 0; i < m - 1; ++i)
	{
		for (uint32 j = 0; j < n - 1; ++j)
		{
			meshData.Indices32[k] = i*n + j;
			meshData.Indices32[k + 1] = i*n + j + 1;
			meshData.Indices32[k + 2] = (i + 1)*n + j;

			meshData.Indices32[k + 3] = (i + 1)*n + j;
			meshData.Indices32[k + 4] = i*n + j + 1;
			meshData.Indices32[k + 5] = (i + 1)*n + j + 1;

			k += 6; // next quad
		}
	}

	return meshData;
}

void GeometryGenerator::Subdivide(MeshData& meshData) {
	MeshData inputCopy = meshData;

	meshData.Vertices.resize(0);
	meshData.Indices32.resize(0);

	uint32 numTris = (uint32)inputCopy.Indices32.size() / 3;
	for (uint32 i = 0; i < numTris; ++i) {
		Vertex v0 = inputCopy.Vertices[inputCopy.Indices32[i * 3 + 0]];
		Vertex v1 = inputCopy.Vertices[inputCopy.Indices32[i * 3 + 1]];
		Vertex v2 = inputCopy.Vertices[inputCopy.Indices32[i * 3 + 2]];

		Vertex m0 = MidPoint(v0, v1);
		Vertex m1 = MidPoint(v1, v2);
		Vertex m2 = MidPoint(v0, v2);

		// vertices
		meshData.Vertices.emplace_back(v0); // 0
		meshData.Vertices.emplace_back(v1); // 1
		meshData.Vertices.emplace_back(v2); // 2
		meshData.Vertices.emplace_back(m0); // 3
		meshData.Vertices.emplace_back(m1); // 4
		meshData.Vertices.emplace_back(m2); // 5

		// indices (the multiplier is 6 because we've added 6 vertices)
		meshData.Indices32.emplace_back(i * 6 + 0);
		meshData.Indices32.emplace_back(i * 6 + 3);
		meshData.Indices32.emplace_back(i * 6 + 5);

		meshData.Indices32.emplace_back(i * 6 + 3);
		meshData.Indices32.emplace_back(i * 6 + 1);
		meshData.Indices32.emplace_back(i * 6 + 4);

		meshData.Indices32.emplace_back(i * 6 + 3);
		meshData.Indices32.emplace_back(i * 6 + 4);
		meshData.Indices32.emplace_back(i * 6 + 5);

		meshData.Indices32.emplace_back(i * 6 + 4);
		meshData.Indices32.emplace_back(i * 6 + 2);
		meshData.Indices32.emplace_back(i * 6 + 5);
	}
}

GeometryGenerator::Vertex GeometryGenerator::MidPoint(const Vertex& v0, const Vertex& v1) {

	// position
	XMVECTOR p0 = XMLoadFloat3(&v0.Position);
	XMVECTOR p1 = XMLoadFloat3(&v1.Position);
	// normals
	XMVECTOR n0 = XMLoadFloat3(&v0.Normal);
	XMVECTOR n1 = XMLoadFloat3(&v1.Normal);
	// tangents
	XMVECTOR t0 = XMLoadFloat3(&v0.TangentU);
	XMVECTOR t1 = XMLoadFloat3(&v1.TangentU);
	// textures
	XMVECTOR tex0 = XMLoadFloat2(&v0.TexC);
	XMVECTOR tex1 = XMLoadFloat2(&v1.TexC);

	XMVECTOR pos = 0.5f*(p0 + p1);
	XMVECTOR normal = XMVector3Normalize(0.5f*(n0 + n1));
	XMVECTOR tangent = XMVector3Normalize(0.5f*(t0 + t1));
	XMVECTOR tex = 0.5f*(tex0 + tex1);

	Vertex v;
	XMStoreFloat3(&v.Position, pos);
	XMStoreFloat3(&v.Normal, normal);
	XMStoreFloat3(&v.TangentU, tangent);
	XMStoreFloat2(&v.TexC, tex);

	return v;
}

void GeometryGenerator::BuildCylinderBottomCap(float bottomRadius, float topRadius, float height, uint32 sliceCount, uint32 stackCount, MeshData& meshData) {
	const float dTheta = XM_2PI / sliceCount;
	const XMFLOAT3 normal = XMFLOAT3(0.0f, -1.0f, 0.0f);
	const XMFLOAT3 tangent = XMFLOAT3(-1.0f, 0.0f, 0.0f);
	const float y = -0.5f * height;
	const uint32 baseIndex = meshData.Vertices.size();
	for (uint32 i = 0; i < sliceCount; ++i) {
		float x = bottomRadius*cosf(dTheta * i);
		float z = bottomRadius*sinf(dTheta * i);
		float u = x / height - 0.5f;
		float v = z / height - 0.5f;
		meshData.Vertices.emplace_back(
			x, y, z,
			normal.x, normal.y, normal.z,
			tangent.x, tangent.y, tangent.z,
			u, v
		);
	}

	const uint32 centerIndex = meshData.Vertices.size();
	meshData.Vertices.emplace_back(
		0.0f, y, 0.0f,
		normal.x, normal.y, normal.z,
		tangent.x, tangent.y, tangent.z,
		-0.5f, -0.5f
	);

	for (uint32 i = 0; i < sliceCount; ++i) {
		meshData.Indices32.emplace_back(centerIndex);
		meshData.Indices32.emplace_back(baseIndex + i);
		meshData.Indices32.emplace_back(baseIndex + i + 1);
	}
}

void GeometryGenerator::BuildCylinderTopCap(float bottomRadius, float topRadius, float height, uint32 sliceCount, uint32 stackCount, MeshData& meshData) {
	// dTheta degrees per slice
	const float dTheta = XM_2PI / sliceCount;
	const XMFLOAT3 normal = XMFLOAT3(0.0f, 1.0f, 0.0f);
	const XMFLOAT3 tangent = XMFLOAT3(1.0f, 0.0f, 0.0f);
	const float y = 0.5f * height;
	const uint32 baseIndex = meshData.Vertices.size();
	for (uint32 i = 0; i < sliceCount; ++i) {
		// calculate x, z
		float x = topRadius*cosf(dTheta*i);
		float z = topRadius*sinf(dTheta*i);

		float u = x / height + 0.5f;
		float v = z / height + 0.5f;
		meshData.Vertices.emplace_back(
			x, y, z,
			normal.x, normal.y, normal.z,
			tangent.x, tangent.y, tangent.z,
			u, v);
	}

	const uint32 centerIndex = meshData.Vertices.size();
	meshData.Vertices.emplace_back(
		0.0f, y, 0.0f,
		normal.x, normal.y, normal.z,
		tangent.x, tangent.y, tangent.z,
		0.5f, 0.5f
	);

	// add the indices
	for (uint32 i = 0; i < sliceCount; ++i) {
		meshData.Indices32.emplace_back(centerIndex);
		meshData.Indices32.emplace_back(baseIndex + i + 1);
		meshData.Indices32.emplace_back(baseIndex + i);
	}
}


GeometryGenerator::MeshData GeometryGenerator::CreateHyperboloidOneSheet(float height, float a, float b,
	float c, uint32 sliceCount, uint32 stackCount) {
	// don't allow bogus values
	assert(sliceCount > 1);
	assert(stackCount > 1);

	MeshData hyperboloid;

	// constants
	const float scaleCoeff = a*b / c;
	const float dTheta = XM_2PI / sliceCount;
	const float heightOffset = - (height / 2.0f);
	const float dh = height / stackCount;
	const uint32 ringCount = stackCount + 1;

	const uint32 ringVertexCount = sliceCount + 1;
	float x, y, z, theta, magRho;
	x = y = z = theta = magRho = 0;
	for (uint32 i = 0; i < ringCount; ++i) {
		y = i*dh + heightOffset;
		for (uint32 j = 0; j < ringVertexCount; ++j) {
			theta = j*dTheta;
			magRho = CalculateMagRhoForHyperboloid(theta, y, a, b, c);
			x = magRho*cosf(theta);
			z = magRho*sinf(theta);

			Vertex v;
			v.Position = XMFLOAT3(x, y, z);
			hyperboloid.Vertices.emplace_back(v);
		}
	}

	// build indices
	uint32 n = stackCount - 1;

	uint32 bottomLeft = 0;
	for (uint32 i = 0; i < ringCount; ++i) {
		for (uint32 j = 0; j < sliceCount; ++j) {
			bottomLeft = i*sliceCount + j;
			hyperboloid.Indices32.emplace_back(bottomLeft); // bottom left
			hyperboloid.Indices32.emplace_back(bottomLeft + sliceCount); // top left;
			hyperboloid.Indices32.emplace_back(bottomLeft + 1); // bottom right
			hyperboloid.Indices32.emplace_back(bottomLeft + 1); // bottom right
			hyperboloid.Indices32.emplace_back(bottomLeft + sliceCount); // top left
			hyperboloid.Indices32.emplace_back(bottomLeft + sliceCount + 1); // top right
		}
	}
	BuildHyperboloidCap(height, a, b, c, sliceCount, stackCount, hyperboloid);
	BuildHyperboloidCap(height, a, b, c, sliceCount, stackCount, hyperboloid, true);
	return hyperboloid;
}

void GeometryGenerator::BuildHyperboloidCap(float height, float a, float b, float c, uint32 sliceCount, uint32 stackCount, MeshData& meshData, bool bottom) {
	
	const float y = bottom ? -(height / 2.0f) : +(height / 2.0f);
	const float dTheta = XM_2PI / sliceCount;

	float x, z, rho, theta;
	const uint32 baseVertexIndex = (uint32)meshData.Vertices.size();
	
	for (uint32 i = 0; i <= sliceCount; ++i) {
		theta = dTheta*i;
		rho = CalculateMagRhoForHyperboloid(theta, y, a, b, c);
		x = rho*cosf(theta);
		z = rho*sinf(theta);
		Vertex v;
		v.Position = XMFLOAT3(x, y, z);
		meshData.Vertices.emplace_back(v);
	}

	const uint32 centerIndex = (uint32)meshData.Vertices.size();
	Vertex center;
	center.Position = XMFLOAT3(0.0f, y, 0.0f);

	meshData.Vertices.emplace_back(center);

	for (uint32 i = 0; i < sliceCount; ++i) {
		// center
		meshData.Indices32.emplace_back(centerIndex);
		if (bottom) {
			meshData.Indices32.emplace_back(baseVertexIndex + i);
			meshData.Indices32.emplace_back(baseVertexIndex + i + 1);
		}
		else {
			meshData.Indices32.emplace_back(baseVertexIndex + i + 1);
			meshData.Indices32.emplace_back(baseVertexIndex + i);
		}

	}
}

GeometryGenerator::MeshData GeometryGenerator::CreateEllipsoid(float a, float b, float c, uint32 sliceCount, uint32 stackCount) {
	MeshData ellipsoid;
	
	assert(a > 0);
	assert(b > 0);
	assert(c > 0);


	const uint32 ringCount = stackCount + 1;
	const float dTheta = XM_2PI / sliceCount;
	const float heightOffset = -c;
	const float dh = c * 2.0f / (stackCount);
	Vertex topVertex, bottomVertex;
	topVertex.Position = XMFLOAT3(+0.0f, +c, +0.0f);
	bottomVertex.Position = XMFLOAT3(+0.0, -c, +0.0f);
	ellipsoid.Vertices.emplace_back(bottomVertex);
	float x, y, z, theta, rho;
	for (uint32 i = 1; i < stackCount; ++i) {
		y = dh * i + heightOffset;
		for (uint32 j = 0; j <= sliceCount; ++j) {
			theta = dTheta*j;
			rho = CalculateMagRhoForEllipsoid(theta, y, a, b, c);
			x = rho*cosf(theta);
			z = rho*sinf(theta);
			Vertex v;
			v.Position = XMFLOAT3(x, y, z);
			ellipsoid.Vertices.emplace_back(v);
		}
	}

	const uint32 baseTopIndex = (UINT)ellipsoid.Vertices.size();
	ellipsoid.Vertices.emplace_back(topVertex);

	uint32 ringVertexCount = sliceCount + 1;

	for (uint32 i = 1; i < sliceCount; ++i) {
		ellipsoid.Indices32.emplace_back(0);
		ellipsoid.Indices32.emplace_back(i);
		ellipsoid.Indices32.emplace_back(i + 1);
	}

	uint32 baseIndex = 1;
	for (uint32 i = 0; i < stackCount - 2; ++i) {
		for (uint32 j = 0; j < sliceCount; ++j) {
			uint32 bottomLeft = baseIndex + i*ringVertexCount + j;
			uint32 topLeft = baseIndex + (i + 1)*ringVertexCount + j;
			ellipsoid.Indices32.emplace_back(bottomLeft);
			ellipsoid.Indices32.emplace_back(topLeft);
			ellipsoid.Indices32.emplace_back(topLeft + 1);

			ellipsoid.Indices32.emplace_back(bottomLeft);
			ellipsoid.Indices32.emplace_back(topLeft + 1);
			ellipsoid.Indices32.emplace_back(bottomLeft + 1);
		}
	}

	for (uint32 i = 0; i < sliceCount; ++i) {
		ellipsoid.Indices32.emplace_back(baseTopIndex);
		ellipsoid.Indices32.emplace_back(baseTopIndex - ringVertexCount + i + 1);
		ellipsoid.Indices32.emplace_back(baseTopIndex - ringVertexCount + i);
	}

	return ellipsoid;
}

GeometryGenerator::MeshData GeometryGenerator::CreatePyramid(float width, float height, float p, uint32 numSubdivisions) {
	assert(width > 0);
	assert(height > 0);
	assert(p > 0);

	const float percentHeight = p*height;
	const float farX = +width / 2.0f;
	const float farZ = +width / 2.0f;
	const float top = +percentHeight / 2.0f;
	const float bottom = -percentHeight / 2.0f;

	MeshData pyramid;
	auto pushVertex = [&](const Vertex& v) { pyramid.Vertices.emplace_back(v); };
	
	Vertex bottomPosXZ, bottomPosXNegZ, bottomNegXZ, bottomNegXPosZ;
	bottomPosXZ.Position = XMFLOAT3(+farX, bottom, +farZ);
	bottomPosXNegZ.Position = XMFLOAT3(+farX, bottom, -farZ);
	bottomNegXZ.Position = XMFLOAT3(-farX, bottom, -farZ);
	bottomNegXPosZ.Position = XMFLOAT3(-farX, bottom, +farZ);

	// standard pyramid
	if (1.0f - p <= 0.005) {
		Vertex topVertex;
		topVertex.Position = XMFLOAT3(0.0f, top, 0.0f);

		pushVertex(topVertex);
		pushVertex(bottomPosXZ);
		pushVertex(bottomPosXNegZ);
		pushVertex(bottomNegXZ);
		pushVertex(bottomNegXPosZ);

		std::vector<uint32_t> indices = {
			0, 1, 2, // right
			0, 2, 3, // back
			0, 3, 4, // left
			0, 4, 1, // front
			4, 3, 2,
			4, 2, 1
		};

		pyramid.Indices32 = std::move(indices);
	}
	else {
		// calculate far x and z using similar triangles
		const float halfBaseDiagonal = width / 2.0f * sqrtf(2.0f);
		const float topFarX = (1 - p)*halfBaseDiagonal / sqrtf(2.0f);
		const float topFarZ = topFarX;

		Vertex topPosXZ, topPosXNegZ, topNegXZ, topNegXPosZ;
		topPosXZ.Position = XMFLOAT3(+topFarX, top, +topFarZ);
		topPosXNegZ.Position = XMFLOAT3(+topFarX, top, -topFarZ);
		topNegXZ.Position = XMFLOAT3(-topFarX, top, -topFarZ);
		topNegXPosZ.Position = XMFLOAT3(-topFarX, top, +topFarZ);

		pushVertex(topPosXZ);
		pushVertex(topPosXNegZ);
		pushVertex(topNegXZ);
		pushVertex(topNegXPosZ);
		pushVertex(bottomPosXZ);
		pushVertex(bottomPosXNegZ);
		pushVertex(bottomNegXZ);
		pushVertex(bottomNegXPosZ);

		std::vector<uint32_t> indices = { 
			1, 0, 4,
			1, 4, 5,
			2, 1, 5,
			2, 5, 6,
			2, 6, 7,
			2, 7, 3,
			0, 3, 7,
			0, 7, 4,
			4, 6, 5,
			4, 7, 6,
			3, 1, 2,
			3, 0, 1
		};
		pyramid.Indices32 = std::move(indices);
	}
	auto divisions = std::min<uint32_t>(6u, numSubdivisions);
	for (uint32 i = 0; i < divisions; ++i)
		Subdivide(pyramid);

	return pyramid;
}

GeometryGenerator::MeshData GeometryGenerator::CreateObjectFromXYTrace(const std::vector<XMFLOAT2>& points, uint32 sliceCount, float degrees) {
	MeshData object;
	float dTheta = degrees / sliceCount;
	for (auto& point : points) {
		for (uint32 i = 0; i <= sliceCount; ++i) {
			Vertex v;
			v.Position = XMFLOAT3(point.x*cosf(i*dTheta), point.y, -point.x*sinf(i*dTheta));
			object.Vertices.emplace_back(v);
		}
	}

	uint32 numPoints = (UINT)points.size();
	uint32 ringVertexCount = sliceCount + 1;
	for (uint32 i = 0; i < numPoints - 1; ++i) {
		for (uint32 j = 0; j < sliceCount; ++j) {
			uint32 topLeft = i*ringVertexCount + j;
			object.Indices32.emplace_back(topLeft);
			object.Indices32.emplace_back(topLeft + 1);
			object.Indices32.emplace_back(topLeft + ringVertexCount + 1);
			object.Indices32.emplace_back(topLeft + ringVertexCount + 1);
			object.Indices32.emplace_back(topLeft + ringVertexCount);
			object.Indices32.emplace_back(topLeft);
		}
	}

	// add top
	Vertex top;
	top.Position = XMFLOAT3(0.0f, points[numPoints - 1].y, 0.0f);
	object.Vertices.emplace_back(top);
	uint32 baseTop = (UINT)object.Vertices.size() - 1;
	for (uint32 i = 0; i <= sliceCount; ++i) {
		object.Indices32.emplace_back(baseTop);
		object.Indices32.emplace_back(baseTop - i - 1);
		object.Indices32.emplace_back(baseTop - i);
	}
	return object;
}

/*
* Rho Vector is vector in plane y = k
* Can use magnitude to calculate x, z using polar coordinates
* where x = |Rho|*cos(theta), z = |Rho|*sin(theta)
*/
float GeometryGenerator::CalculateMagRhoForHyperboloid(float theta, float k, float a, float b, float c) {
	return (a*b/c)*sqrtf(
		(c*c + k*k) / (pow(b*cosf(theta), 2) + (pow(a*sinf(theta), 2)))
	);
}

float GeometryGenerator::CalculateMagRhoForEllipsoid(float theta, float k, float a, float b, float c) {
	return (a*b / c)*sqrtf(
		(c*c - k*k) / (pow(b*cosf(theta), 2) + pow(a*sinf(theta), 2)));
}

