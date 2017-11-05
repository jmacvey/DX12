#ifndef SKULL_GEOMETRY_H
#define SKULL_GEOMETRY_H

#include "GeometryGenerator.h"

class SkullGeometry {
public:
	static const std::string FilePath;

	SkullGeometry();
	SkullGeometry(const SkullGeometry& skullGeometry) = delete;
	SkullGeometry& operator=(const SkullGeometry& skullGeometry) = delete;


	std::vector<GeometryGenerator::Vertex>& GetVertices();
	std::vector<uint16_t>& GetIndices();

	std::uint32_t GetIndexCount() const;

private:

	void ReadFile();
	void ReadNumIndices(std::ifstream& ifStream);
	void ReadNumVertices(std::ifstream& ifStream);
	void ReadVertices(std::ifstream& ifStream);
	void ReadIndices(std::ifstream& ifStream);
	void ReadInputConvertToFloat(std::ifstream& ifStream, std::string& buffer, float* destinationAddr, bool readLine = false);
	std::vector<GeometryGenerator::Vertex> mVertices;
	std::vector<uint16_t> mIndices;
	uint32_t numVertices = 0;
	uint32_t numIndices = 0;
	uint32_t numTriangles = 0;
};

#endif