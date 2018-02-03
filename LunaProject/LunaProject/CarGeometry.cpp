#include "stdafx.h"
#include "CarGeometry.h"
#include <fstream>

using namespace DirectX;
const std::string CarGeometry::DEFAULT_FILE_LOCATION = "Models//car.txt";

CarGeometry::CarGeometry() : mFileLocation(CarGeometry::DEFAULT_FILE_LOCATION)
{
	ReadData();
}

CarGeometry::CarGeometry(const std::string & fileLocation) : mFileLocation(fileLocation)
{
	ReadData();
}

std::vector<GeometryGenerator::Vertex>& CarGeometry::GetVertices()
{
	return mVertices;
}

std::vector<std::uint32_t>& CarGeometry::GetIndices()
{
	return mIndices;
}

void CarGeometry::ReadData()
{
	std::ifstream istream(mFileLocation, std::ios::in);
	std::string buffer;
	if (istream.good()) {
		istream >> buffer;
		istream >> mNumVertices;
		istream >> buffer;
		istream >> mNumTriangles;
		for (UINT i = 0; i < 4; ++i)
			istream >> buffer;

		ReadVertices(istream);
		for (UINT i = 0; i < 3; ++i)
			istream >> buffer;
		ReadIndices(istream);
		istream.close();
	}
}

void CarGeometry::ReadVertices(std::ifstream & fstream)
{
	XMFLOAT3 vMin3f(+MathHelper::Infinity, +MathHelper::Infinity, +MathHelper::Infinity);
	XMFLOAT3 vMax3f(-MathHelper::Infinity, -MathHelper::Infinity, -MathHelper::Infinity);

	XMVECTOR vMin = XMLoadFloat3(&vMin3f);
	XMVECTOR vMax = XMLoadFloat3(&vMax3f);
	mVertices.resize(mNumVertices);
	for (UINT i = 0; i < mNumVertices; ++i) {
		fstream >> mVertices[i].Position.x;
		fstream >> mVertices[i].Position.y;
		fstream >> mVertices[i].Position.z;
		fstream >> mVertices[i].Normal.x;
		fstream >> mVertices[i].Normal.y;
		fstream >> mVertices[i].Normal.z;

		XMVECTOR P = XMLoadFloat3(&mVertices[i].Position);

		vMin = XMVectorMin(vMin, P);
		vMax = XMVectorMax(vMax, P);
	}
	XMStoreFloat3(&vMax3f, vMax);
	XMStoreFloat3(&vMin3f, vMin);

	XMStoreFloat3(&mBoundingBox.Center, 0.5f*(vMax + vMin));
	XMStoreFloat3(&mBoundingBox.Extents, 0.5f*(vMax - vMin));
}

void CarGeometry::ReadIndices(std::ifstream & fstream)
{
	const UINT numIndices = mNumTriangles * 3;
	mIndices.resize(numIndices);
	for (UINT i = 0; i < numIndices; ++i) {
		fstream >> mIndices[i];
	}
}

BoundingBox CarGeometry::GetBoundingBox() const
{
	return mBoundingBox;
}
