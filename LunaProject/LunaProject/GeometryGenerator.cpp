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
	return MeshData();
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

	for (int i = 0; i < division; ++i) {
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
	//const float halfWidth = 0.5f*width; // <- total column width
	//const float halfDepth = 0.5f*depth; // <- total row width

	//const uint32 numVertices = m*n;
	//const float dx = width / (n - 1); // <- face width
	//const float dz = depth / (m - 1); // <- face height
	//const float du = 1.0f / (n - 1);
	//const float dv = 1.0f / (m - 1);
	//MeshData grid;
	//grid.Vertices.resize(numVertices);
	//float z, x;
	//for (uint32 i = 0; i < m; ++i) {
	//	z = halfDepth - dz * i;
	//	for (uint32 j = 0; j < n; ++j) {
	//		x = -halfWidth + j * dx;
	//		uint32 offset = i * n + j;
	//		grid.Vertices[offset].Position = XMFLOAT3(x, 0.0f, z);
	//		grid.Vertices[offset].Normal = XMFLOAT3(0.0f, 1.0f, 0.0f);
	//		grid.Vertices[offset].TangentU = XMFLOAT3(1.0f, 0.0f, 0.0f);
	//		grid.Vertices[offset].TexC = XMFLOAT2(du, dv);
	//	}
	//}

	//// triangle faces = num faces * 2
	//const uint32 triangleFaceCount = (m - 1)*(n - 1) * 2;

	//// 3 indices per face
	//grid.Indices32.resize(triangleFaceCount * 3);

	//uint32 k = 0;
	//uint32 columnCount = m - 1;
	//uint32 rowCount = n - 1;
	//for (uint32 i = 0; i < columnCount; ++i) {
	//	for (uint32 j = 0; j < rowCount; ++j) {

	//		uint32 topLeft = i * n + j;
	//		uint32 topRight = topLeft + 1;

	//		grid.Indices32[k] = topLeft;
	//		grid.Indices32[k + 1] = topRight;
	//		grid.Indices32[k + 2] = topLeft + n;

	//		grid.Indices32[k + 3] = topRight;
	//		grid.Indices32[k + 4] = topRight + n;
	//		grid.Indices32[k + 5] = topLeft + n;
	//		k += 6;
	//	}
	//}
	//return grid;
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