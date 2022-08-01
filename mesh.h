#pragma once

#include "general_common.h"

#include <memory>

#include <glm/glm.hpp>
#include <GL/glew.h>

#include "text_loader.h"
#include "game_math.h"
#include "color.h"

// TODO: drawCount should be glsizei, and other variables should also use propper opengl types
class Mesh
{
	friend struct std::hash<Mesh>;
public:
	static uint64_t activeCirculationCount;
private:
	struct MeshBox
	{
		GLuint vertexArrayObject;
		GLuint* vertexBufferObjects;
		unsigned int bufferCount;
		unsigned int drawCount;
		uint64_t circulationID;
		GLsizei vertexCapacity;
		GLsizei indexCapacity;

		MeshBox(GLuint vertexArrayObject, GLuint* vertexBufferObjects, unsigned int bufferCount, unsigned int drawCount);
		~MeshBox();
	};

	std::shared_ptr<MeshBox> meshBox;
public:
	struct Descriptor
	{
		struct Array
		{
			GLsizeiptr arrDataSize;
			const void* arr;
			GLint typeSize;
			GLenum type;
			bool isStatic;

			Array(GLsizeiptr arrDataSize, const void* arr, GLint typeSize, GLenum type, bool isStatic);
		};

		unsigned int numVertices;
		std::vector<Array> arrs;

		unsigned int numIndices;
		const unsigned short* indices;
		const unsigned int* longIndices;
		bool areIndicesStatic;

		FVector3 AABBMin, AABBMax;
		float boundingRadius;

		Descriptor(unsigned int numVertices);
		Descriptor(const TextData& data);

		Descriptor& AddArray(const FVector2* arr, bool isStatic = true);
		Descriptor& AddArray(const FVector3* arr, bool isStatic = true);
		Descriptor& AddArray(const FColor* arr, bool isStatic = true);
		Descriptor& AddArray(const float* arr, unsigned int vectorSize, bool isStatic = true);
		// WARNING: Only 1 can be used!
		Descriptor& AddIndices(const unsigned short* indices, unsigned int numIndices, bool isStatic = true);
		Descriptor& AddIndices(const unsigned int* indices, unsigned int numIndices, bool isStatic = true);
		
		Descriptor& AddBounds(const FVector3 AABBMin, const FVector3& AABBMax, float boundingRadius);
		Descriptor& CalculateBoundsFromLastArray();
	};

	// TODO: Put in box
	FVector3 AABBMin, AABBMax;
	float boundingRadius;

	Mesh() :
		AABBMin(-1.0F),
		AABBMax(1.0F),
		boundingRadius(1.0F)
	{ }

	~Mesh() noexcept = default;
	Mesh(const Descriptor& descriptor);
	Mesh(const Mesh&) noexcept = default;
	Mesh(Mesh&&) noexcept = default;
	Mesh& operator=(const Mesh&) noexcept = default;
	Mesh& operator=(Mesh&&) noexcept = default;
	bool operator==(const Mesh& other) const;

	bool IsLoaded() const
	{
		return meshBox != nullptr;
	}

	void Substitute(unsigned int index, const float* arr, GLint typeSize, unsigned short offset, unsigned short count);
	void Substitute(unsigned int index, const FVector2* arr, unsigned short offset, unsigned short count);
	void Substitute(unsigned int index, const FVector3* arr, unsigned short offset, unsigned short count);
	void Substitute(unsigned int index, const FColor* arr, unsigned short offset, unsigned short count);
	// Must use the same type of indices.
	void SubstituteIndices(const unsigned short* indices, unsigned int offset, unsigned int count);
	void SubstituteIndices(const unsigned int* indices, unsigned int offset, unsigned int count);
	// Will create new vao if needed, otherwise it just substitutes.
	void Rebuild(const Descriptor& descriptor); // Only works if the type of mesh isn't changed (same num of vbos etc.)

	inline void SetDrawCount(unsigned int drawCount)
	{
		meshBox->drawCount = drawCount;
	}

	inline unsigned int GetDrawCount() const
	{
		return meshBox->drawCount;
	}

	inline void Bind() const
	{
 		glBindVertexArray(meshBox->vertexArrayObject);
	}

	// TODO: mode = GL_TRIANGLES?
	inline void DrawElements() const
	{
		glDrawElements(GL_TRIANGLES, meshBox->drawCount, GL_UNSIGNED_SHORT, 0);
	}

	inline void DrawElements(GLenum mode) const
	{
		glDrawElements(mode, meshBox->drawCount, GL_UNSIGNED_SHORT, 0);
	}

	inline void DrawElementsLong() const
	{
		glDrawElements(GL_TRIANGLES, meshBox->drawCount, GL_UNSIGNED_INT, 0);
	}

	inline void DrawElementsLong(GLenum mode) const
	{
		glDrawElements(mode, meshBox->drawCount, GL_UNSIGNED_INT, 0);
	}

	inline void DrawArrays() const
	{
		glDrawArrays(GL_TRIANGLES, 0, meshBox->drawCount);
	}

	inline void DrawArrays(GLenum mode) const
	{
		glDrawArrays(mode, 0, meshBox->drawCount);
	}

	inline uint64_t GetCirculationID()
	{
		return meshBox->circulationID;
	}
};

namespace std
{
	template <>
	struct hash<Mesh>
	{
		std::size_t operator()(const Mesh& k) const
		{
			return hash<unsigned int>()(k.meshBox->vertexArrayObject);
		}
	};
}
