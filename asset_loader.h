#pragma once

#include "mesh.h"

struct MeshData
{
	std::vector<uint16_t> indices;
	std::vector<FVector3> positions;
	std::vector<FVector2> texCoords;
	std::vector<FVector3> normals;
	std::vector<FVector3> tangents;
	FVector3 boxMin;
	FVector3 boxMax;
	float radius;

	MeshData() :
		boxMin(-1.0F),
		boxMax(1.0F),
		radius(1.0F)
	{ }

	MeshData(size_t initialVertexCount, size_t initialIndexCount) :
		boxMin(-1.0F),
		boxMax(1.0F),
		radius(1.0F)
	{
		indices.resize(initialIndexCount);
		positions.resize(initialVertexCount);
		texCoords.resize(initialVertexCount);
		normals.resize(initialVertexCount);
		tangents.resize(initialVertexCount);
	}

	MeshData(std::vector<uint16_t>&& indices, std::vector<FVector3>&& positions, std::vector<FVector2>&& texCoords, std::vector<FVector3>&& normals, std::vector<FVector3>&& tangents, const FVector3& boxMin, const FVector3& boxMax, float radius) :
		indices(std::move(indices)),
		positions(std::move(positions)),
		texCoords(std::move(texCoords)),
		normals(std::move(normals)),
		tangents(std::move(tangents)),
		boxMin(boxMin),
		boxMax(boxMax),
		radius(radius)
	{ }

	MeshData(MeshData&&) noexcept = default;
	~MeshData() noexcept = default;
	MeshData& operator=(MeshData&&) noexcept = default;
};

namespace Load
{
	Mesh OBJFile(const std::string& directory);
	MeshData OBJFileData(const std::string& directory);
}