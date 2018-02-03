#ifndef CAR_GEOMETRY_H
#define CAR_GEOMETRY_H

#include "GeometryGenerator.h"
#include <vector>
#include <string>

class CarGeometry {
public:
	CarGeometry();
	CarGeometry(const std::string& fileLocation);


	std::vector<GeometryGenerator::Vertex>& GetVertices();
	std::vector<std::uint32_t>& GetIndices();
	BoundingBox GetBoundingBox() const;

private:
	void ReadData();
	void ReadVertices(std::ifstream& fstream);
	void ReadIndices(std::ifstream& fstream);
private:
	const static std::string DEFAULT_FILE_LOCATION;
	std::vector<GeometryGenerator::Vertex> mVertices;
	std::vector<std::uint32_t> mIndices;
	std::string mFileLocation;

	UINT mNumVertices;
	UINT mNumTriangles;
	BoundingBox mBoundingBox;
};

#endif