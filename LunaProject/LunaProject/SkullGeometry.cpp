#include "stdafx.h"
#include <string>
#include <fstream>
#include "SkullGeometry.h"

const std::string SkullGeometry::FilePath = "Models//skull.txt";

SkullGeometry::SkullGeometry()
{
	ReadFile();
}

std::vector<GeometryGenerator::Vertex>& SkullGeometry::GetVertices()
{
	return mVertices;
}

std::vector<uint16_t>& SkullGeometry::GetIndices()
{
	return mIndices;
}

std::uint32_t SkullGeometry::GetIndexCount() const
{
	return (UINT)mIndices.size();
}

BoundingBox SkullGeometry::GetBoundingBox() const
{
	return mBoundingBox;
}

void SkullGeometry::ReadFile()
{
	std::ifstream ifStream(SkullGeometry::FilePath);
	ReadNumVertices(ifStream);
	ReadNumIndices(ifStream);
	ReadVertices(ifStream);
	ReadIndices(ifStream);
	ifStream.close();
}

void SkullGeometry::ReadNumIndices(std::ifstream & ifStream)
{
	std::string buffer;
	ifStream >> buffer;
	std::getline(ifStream, buffer);
	numTriangles = (UINT)stoi(buffer);
	numIndices = numTriangles * 3;
}

void SkullGeometry::ReadNumVertices(std::ifstream & ifStream)
{
	std::string buffer;
	ifStream >> buffer;
	std::getline(ifStream, buffer);
	numVertices = (UINT)stoi(buffer);
}

void SkullGeometry::ReadVertices(std::ifstream & ifStream)
{
	std::string buffer;
	GeometryGenerator::Vertex v;
	XMFLOAT3 vMin3f(+MathHelper::Infinity, +MathHelper::Infinity, +MathHelper::Infinity);
	XMFLOAT3 vMax3f(-MathHelper::Infinity, -MathHelper::Infinity, -MathHelper::Infinity);
	XMVECTOR vMin = XMLoadFloat3(&vMin3f);
	XMVECTOR vMax = XMLoadFloat3(&vMax3f);
	auto addVertex = [&](std::ifstream& ifStream) {
		ReadInputConvertToFloat(ifStream, buffer, &v.Position.x);
		ReadInputConvertToFloat(ifStream, buffer, &v.Position.y);
		ReadInputConvertToFloat(ifStream, buffer, &v.Position.z);
		ReadInputConvertToFloat(ifStream, buffer, &v.Normal.x);
		ReadInputConvertToFloat(ifStream, buffer, &v.Normal.y);
		ReadInputConvertToFloat(ifStream, buffer, &v.Normal.z, true);

		XMVECTOR P = XMLoadFloat3(&v.Position);
		XMFLOAT3 spherePos;
		XMStoreFloat3(&spherePos, XMVector3Normalize(P));

		float theta = atan2f(spherePos.z, spherePos.x);
		if (theta < 0.0f) {
			theta += XM_2PI;
		}

		float phi = acosf(spherePos.y);

		v.TexC.x = theta / (XM_2PI);
		v.TexC.y = phi / XM_PI;



		vMin = XMVectorMin(vMin, P);
		vMax = XMVectorMax(vMax, P);
		mVertices.emplace_back(std::move(v));
	};

	// Label and opening brace
	for (uint16_t i = 0; i < 2; ++i) {
		std::getline(ifStream, buffer);
	}
	// vertex data
	for (uint32_t i = 0; i < numVertices; ++i) {
		addVertex(ifStream);
	}
	
	// store the bounding box
	XMStoreFloat3(&mBoundingBox.Center, 0.5f*(vMax + vMin));
	XMStoreFloat3(&mBoundingBox.Extents, 0.5f*(vMax - vMin));

	// closing brace
	std::getline(ifStream, buffer);
}

void SkullGeometry::ReadIndices(std::ifstream & ifStream)
{
	std::string buffer;
	// label and opening brace
	for (uint16_t i = 0; i < 2; ++i) {
		std::getline(ifStream, buffer);
	}

	for (uint32_t i = 0; i < numTriangles; ++i) {
		for (uint16_t j = 0; j < 3; ++j) {
			ifStream >> buffer;
			mIndices.emplace_back((UINT)stoi(buffer));
		}
		// clear to next line
		std::getline(ifStream, buffer);
	}
}

void SkullGeometry::ReadInputConvertToFloat(std::ifstream & ifStream, std::string& buffer, float* destinationAddr, bool readLine)
{
	if (readLine) {
		std::getline(ifStream, buffer);
	}
	else {
		ifStream >> buffer;
	}
	*destinationAddr = stof(buffer);
}
