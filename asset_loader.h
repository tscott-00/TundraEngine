#pragma once

#include "mesh.h"

struct MeshData {
	std::vector<uint16_t> indices;
	std::vector<FVector3> positions;
	std::vector<FVector2> texCoords;
	std::vector<FVector3> normals;
	std::vector<FVector3> tangents;
	FVector3 boxMin;
	FVector3 boxMax;
	float radius;

	MeshData();
	MeshData(size_t initialVertexCount, size_t initialIndexCount);
	MeshData(std::vector<uint16_t>&& indices, std::vector<FVector3>&& positions, std::vector<FVector2>&& texCoords, std::vector<FVector3>&& normals, std::vector<FVector3>&& tangents, const FVector3& boxMin, const FVector3& boxMax, float radius);

	MeshData(MeshData&&) noexcept = default;
	~MeshData() noexcept = default;
	MeshData& operator=(MeshData&&) noexcept = default;
};

namespace Load {
	Mesh OBJFile(const std::string& directory);
	MeshData OBJFileData(const std::string& directory);
}
