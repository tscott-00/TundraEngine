#include "asset_loader.h"

#include <fstream>

#include "debug.h"

MeshData::MeshData() :
	boxMin(-1.0F),
	boxMax(1.0F),
	radius(1.0F)
{ }

MeshData::MeshData(size_t initialVertexCount, size_t initialIndexCount) :
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

MeshData::MeshData(std::vector<uint16_t>&& indices, std::vector<FVector3>&& positions, std::vector<FVector2>&& texCoords, std::vector<FVector3>&& normals, std::vector<FVector3>&& tangents, const FVector3& boxMin, const FVector3& boxMax, float radius) :
	indices(std::move(indices)),
	positions(std::move(positions)),
	texCoords(std::move(texCoords)),
	normals(std::move(normals)),
	tangents(std::move(tangents)),
	boxMin(boxMin),
	boxMax(boxMax),
	radius(radius)
{ }

namespace {
	inline std::vector<std::string> SplitString(const std::string &s, char delim) {
		std::vector<std::string> elems;

		const char* cstr = s.c_str();
		size_t strLength = s.length();
		size_t start = 0;
		size_t end = 0;

		while (end <= strLength) {
			while (end <= strLength) {
				if (cstr[end] == delim)
					break;
				end++;
			}

			elems.push_back(s.substr(start, end - start));
			start = end + 1;
			end = start;
		}

		return elems;
	}
}

namespace {
	struct Vertex {
		static const unsigned short NO_INDEX = UINT16_MAX;

		unsigned short index;
		unsigned short texCoordIndex;
		unsigned short normalIndex;
		FVector3 position;
		unsigned short duplicateVertexIndex;
		std::vector<FVector3> tangents;

		Vertex(unsigned short index, FVector3 position) :
			index(index),
			texCoordIndex(NO_INDEX),
			normalIndex(NO_INDEX),
			position(position),
			duplicateVertexIndex(NO_INDEX)
		{ }

		inline bool IsSet() {
			return texCoordIndex != NO_INDEX && normalIndex != NO_INDEX;
		}

		inline bool HasDuplicateVertex() {
			return duplicateVertexIndex != NO_INDEX;
		}

		inline FVector3 GetAveragedTangent() {
			if (tangents.empty())
				return FVector3(1.0F, 0.0F, 0.0F);

			FVector3 total(0.0F, 0.0F, 0.0F);
			for (FVector3& tangent : tangents)
				total += tangent;

			return Normalize(total);
		}
	};

	Vertex& DealWithAlreadyProcessedVertex(Vertex& previousVertex, unsigned short newTextureIndex, unsigned short newNormalIndex, std::vector<unsigned short>& indices, std::vector<Vertex>& vertices) {
		if (previousVertex.texCoordIndex == newTextureIndex && previousVertex.normalIndex == newNormalIndex) {
			indices.push_back(previousVertex.index);
			return previousVertex;
		} else {
			if (previousVertex.HasDuplicateVertex())
				return DealWithAlreadyProcessedVertex(vertices[previousVertex.duplicateVertexIndex], newTextureIndex, newNormalIndex, indices, vertices);
			else {
				Vertex duplicateVertex(previousVertex);
				
				duplicateVertex.index = static_cast<unsigned short>(vertices.size());
				duplicateVertex.texCoordIndex = newTextureIndex;
				duplicateVertex.normalIndex = newNormalIndex;

				vertices.push_back(duplicateVertex);
				previousVertex.duplicateVertexIndex = duplicateVertex.index;
				indices.push_back(duplicateVertex.index);

				return vertices[duplicateVertex.index];
			}
		}
	}

	Vertex& ProcessVertex(const std::vector<std::string>& vertexText, std::vector<Vertex>& vertices, std::vector<unsigned short>& indices) {
		unsigned short index = static_cast<unsigned short>(std::stoi(vertexText[0]) - 1);
		unsigned short textureIndex = static_cast<unsigned short>(std::stoi(vertexText[1]) - 1);
		unsigned short normalIndex = static_cast<unsigned short>(std::stoi(vertexText[2]) - 1);

		Vertex& vertex = vertices[index];
		if (!vertex.IsSet()) {
			vertex.texCoordIndex = textureIndex;
			vertex.normalIndex = normalIndex;
			indices.push_back(index);
			return vertex;
		} else
			return DealWithAlreadyProcessedVertex(vertex, textureIndex, normalIndex, indices, vertices);
	}

	void CalculateTangents(Vertex& v0, Vertex& v1, Vertex& v2, const std::vector<FVector2>& texCoords) {
		FVector3 deltaPos0 = v1.position - v0.position;
		FVector3 deltaPos1 = v2.position - v0.position;
		FVector2 uv0 = texCoords[v0.texCoordIndex];
		FVector2 uv1 = texCoords[v1.texCoordIndex];
		FVector2 uv2 = texCoords[v2.texCoordIndex];
		FVector2 deltaUV0 = uv1 - uv0;
		FVector2 deltaUV1 = uv2 - uv0;

		float r = 1.0F / (deltaUV0.x * deltaUV1.y - deltaUV0.y * deltaUV1.x);
		deltaPos0 *= deltaUV1.y;
		deltaPos1 *= deltaUV0.y;
		FVector3 tangent = deltaPos0 - deltaPos1;

		v0.tangents.push_back(tangent);
		v1.tangents.push_back(tangent);
		v2.tangents.push_back(tangent);
	}
}

#include <iostream>
Mesh Load::OBJFile(const std::string& directory) {
	std::string fullDirectory = "./Resources/" + directory;
	std::ifstream file;
	file.open(fullDirectory.c_str());

	std::vector<Vertex> vertices;
	std::vector<FVector2> texCoords;
	std::vector<FVector3> normals;
	std::vector<unsigned short> indices;

	std::string line;
	if (file.is_open()) {
		while (file.good()) {
			getline(file, line);
			std::vector<std::string> parts = SplitString(line, ' ');

			if (parts[0] == "v")
				vertices.push_back(Vertex(static_cast<unsigned short>(vertices.size()), FVector3(stof(parts[1]), stof(parts[2]), stof(parts[3]))));
			else if (parts[0] == "vt")
				texCoords.push_back(FVector2(stof(parts[1]), stof(parts[2])));
			else if (parts[0] == "vn")
				normals.push_back(FVector3(stof(parts[1]), stof(parts[2]), stof(parts[3])));
			else if (parts[0] == "f")
				break;
		}
		while (file.good()) {
			std::vector<std::string> parts = SplitString(line, ' ');
			if (parts[0] == "f") {
				std::vector<std::string> vertexText0 = SplitString(parts[1], '/');
				std::vector<std::string> vertexText1 = SplitString(parts[2], '/');
				std::vector<std::string> vertexText2 = SplitString(parts[3], '/');
				
				Vertex vertex0 = ProcessVertex(vertexText0, vertices, indices);
				Vertex vertex1 = ProcessVertex(vertexText1, vertices, indices);
				Vertex vertex2 = ProcessVertex(vertexText2, vertices, indices);

				CalculateTangents(vertex0, vertex1, vertex2, texCoords);
			}

			getline(file, line);
		}
	} else {
		Console::Error("Error loading OBJ File: " + fullDirectory + " could not be opened");
		return Mesh();
	}

	// TODO: Just use arrays?
	std::vector<FVector3> finalPositions;
	std::vector<FVector2> finalTexCoords;
	std::vector<FVector3> finalNormals;
	std::vector<FVector3> finalTangents;
	finalPositions.reserve(vertices.size());
	finalTexCoords.reserve(vertices.size());
	finalNormals.reserve(vertices.size());
	finalTangents.reserve(vertices.size());

	float furthestPoint = 0.0F;
	float minX = INFINITY;
	float minY = INFINITY;
	float minZ = INFINITY;
	float maxX = -INFINITY;
	float maxY = -INFINITY;
	float maxZ = -INFINITY;

	// TODO: Are indices here set up correctly? Is everything really removed?
	for (Vertex& vertex : vertices) {
		// TODO: Remove unused vertices?
		if (!vertex.IsSet()) {
			vertex.texCoordIndex = 0;
			vertex.normalIndex = 0;
		}

		float distance = glm::length(vertex.position);
		if (distance > furthestPoint)
			furthestPoint = distance;

		if (vertex.position.x < minX)
			minX = vertex.position.x;
		if (vertex.position.x > maxX)
			maxX = vertex.position.x;

		if (vertex.position.y < minY)
			minY = vertex.position.y;
		if (vertex.position.y > maxY)
			maxY = vertex.position.y;

		if (vertex.position.z < minZ)
			minZ = vertex.position.z;
		if (vertex.position.z > maxZ)
			maxZ = vertex.position.z;

		finalPositions.push_back(vertex.position);
		finalTexCoords.push_back(texCoords[vertex.texCoordIndex]);
		finalNormals.push_back(normals[vertex.normalIndex]);
		finalTangents.push_back(vertex.GetAveragedTangent());
	}

	return Mesh(Mesh::Descriptor(static_cast<unsigned short>(vertices.size())).AddIndices(&indices[0], static_cast<unsigned int>(indices.size())).AddArray(&finalPositions[0]).AddArray(&finalTexCoords[0]).AddArray(&finalNormals[0]).AddArray(&finalTangents[0]).AddBounds(FVector3(minX, minY, minZ), FVector3(maxX, maxY, maxZ), furthestPoint));
}

MeshData Load::OBJFileData(const std::string& directory) {
	std::string fullDirectory = "./Resources/" + directory;
	std::ifstream file;
	file.open(fullDirectory.c_str());

	std::vector<Vertex> vertices;
	std::vector<FVector2> texCoords;
	std::vector<FVector3> normals;
	std::vector<unsigned short> indices;

	std::string line;
	if (file.is_open()) {
		while (file.good()) {
			getline(file, line);
			std::vector<std::string> parts = SplitString(line, ' ');

			if (parts[0] == "v")
				vertices.push_back(Vertex(static_cast<unsigned short>(vertices.size()), FVector3(stof(parts[1]), stof(parts[2]), stof(parts[3]))));
			else if (parts[0] == "vt")
				texCoords.push_back(FVector2(stof(parts[1]), stof(parts[2])));
			else if (parts[0] == "vn")
				normals.push_back(FVector3(stof(parts[1]), stof(parts[2]), stof(parts[3])));
			else if (parts[0] == "f")
				break;
		}
		while (file.good()) {
			std::vector<std::string> parts = SplitString(line, ' ');
			if (parts[0] == "f") {
				std::vector<std::string> vertexText0 = SplitString(parts[1], '/');
				std::vector<std::string> vertexText1 = SplitString(parts[2], '/');
				std::vector<std::string> vertexText2 = SplitString(parts[3], '/');

				Vertex vertex0 = ProcessVertex(vertexText0, vertices, indices);
				Vertex vertex1 = ProcessVertex(vertexText1, vertices, indices);
				Vertex vertex2 = ProcessVertex(vertexText2, vertices, indices);

				CalculateTangents(vertex0, vertex1, vertex2, texCoords);
			}

			getline(file, line);
		}
	} else {
		Console::Error("Error loading OBJ File: " + fullDirectory + " could not be opened");
		return MeshData();
	}

	// TODO: Just use arrays?
	std::vector<FVector3> finalPositions;
	std::vector<FVector2> finalTexCoords;
	std::vector<FVector3> finalNormals;
	std::vector<FVector3> finalTangents;
	finalPositions.reserve(vertices.size());
	finalTexCoords.reserve(vertices.size());
	finalNormals.reserve(vertices.size());
	finalTangents.reserve(vertices.size());

	float furthestPoint = 0.0F;
	float minX = INFINITY;
	float minY = INFINITY;
	float minZ = INFINITY;
	float maxX = -INFINITY;
	float maxY = -INFINITY;
	float maxZ = -INFINITY;

	// TODO: Are indices here set up correctly? Is everything really removed?
	for (Vertex& vertex : vertices) {
		// TODO: Remove unused vertices?
		if (!vertex.IsSet()) {
			vertex.texCoordIndex = 0;
			vertex.normalIndex = 0;
		}

		float distance = glm::length(vertex.position);
		if (distance > furthestPoint)
			furthestPoint = distance;

		if (vertex.position.x < minX)
			minX = vertex.position.x;
		if (vertex.position.x > maxX)
			maxX = vertex.position.x;

		if (vertex.position.y < minY)
			minY = vertex.position.y;
		if (vertex.position.y > maxY)
			maxY = vertex.position.y;

		if (vertex.position.z < minZ)
			minZ = vertex.position.z;
		if (vertex.position.z > maxZ)
			maxZ = vertex.position.z;

		finalPositions.push_back(vertex.position);
		finalTexCoords.push_back(texCoords[vertex.texCoordIndex]);
		finalNormals.push_back(normals[vertex.normalIndex]);
		finalTangents.push_back(vertex.GetAveragedTangent());
	}

	return MeshData(std::move(indices), std::move(finalPositions), std::move(finalTexCoords), std::move(finalNormals), std::move(finalTangents), FVector3(minX, minY, minZ), FVector3(maxX, maxY, maxZ), furthestPoint);
}
