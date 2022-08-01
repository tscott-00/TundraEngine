#include "mesh.h"

#include "debug.h"

uint64_t Mesh::activeCirculationCount = UINT64_C(0);

Mesh::MeshBox::MeshBox(GLuint vertexArrayObject, GLuint* vertexBufferObjects, unsigned int bufferCount, unsigned int drawCount) :
	vertexArrayObject(vertexArrayObject),
	vertexBufferObjects(vertexBufferObjects),
	bufferCount(bufferCount),
	drawCount(drawCount),
	vertexCapacity(0),
	indexCapacity(0)
{
	static uint64_t totalCirculationIDCount = UINT64_C(0);
	circulationID = totalCirculationIDCount++;
	++activeCirculationCount;
}

Mesh::MeshBox::~MeshBox()
{
	--activeCirculationCount;
	glDeleteBuffers(bufferCount, vertexBufferObjects);
	glDeleteVertexArrays(1, &vertexArrayObject);
	delete[] vertexBufferObjects;
}

Mesh::Descriptor::Array::Array(GLsizeiptr arrDataSize, const void* arr, GLint typeSize, GLenum type, bool isStatic) :
	arrDataSize(arrDataSize),
	arr(arr),
	typeSize(typeSize),
	type(type),
	isStatic(isStatic)
{ }

Mesh::Descriptor::Descriptor(unsigned int numVertices)
{
	this->numVertices = numVertices;
	indices = nullptr;
	longIndices = nullptr;
	numIndices = 0;
	areIndicesStatic = true;

	AABBMin = FVector3(-1.0F, -1.0F, -1.0F);
	AABBMax = FVector3(1.0F, 1.0F, 1.0F);
	boundingRadius = 1.0F;
}

Mesh::Descriptor::Descriptor(const TextData& data)
{
	FloatData positions = data.GetFloats("pos");
	FloatData texCoords = data.GetFloats("tex");
	FloatData normals = data.GetFloats("nor");
	FloatData tangents = data.GetFloats("tan");
	UIntData indices = data.GetUInts("ind");
	FloatData bounds = data.GetFloats("bounds");
	longIndices = nullptr;

	numVertices = positions.count / 3;
	if (positions.count == 0)
		Console::Error("All mesh vertices are missing...");
	AddArray(positions.floats, 3);
	if (texCoords.count != 0)
		AddArray(texCoords.floats, 2);
	if (normals.count != 0)
		AddArray(normals.floats, 3);
	if (tangents.count != 0)
		AddArray(tangents.floats, 3);
	if (indices.count != 0)
	{
		this->indices = indices.ints;
		numIndices = indices.count;
	}
	else
	{
		this->indices = nullptr;
	}
	if (bounds.count == 7)
	{
		AABBMin = FVector3(bounds.floats[0], bounds.floats[1], bounds.floats[2]);
		AABBMax = FVector3(bounds.floats[3], bounds.floats[4], bounds.floats[5]);
		boundingRadius = bounds.floats[6];
	}
	else
	{
		AABBMin = FVector3(-1.0F, -1.0F, -1.0F);
		AABBMax = FVector3(1.0F, 1.0F, 1.0F);
		boundingRadius = 1.0F;
	}
}

Mesh::Descriptor& Mesh::Descriptor::AddArray(const FVector2* arr, bool isStatic)
{
	arrs.push_back(Array(numVertices * sizeof(FVector2), arr, 2, GL_FLOAT, isStatic));
	
	return *this;
}

Mesh::Descriptor& Mesh::Descriptor::AddArray(const FVector3* arr, bool isStatic)
{
	arrs.push_back(Array(numVertices * sizeof(FVector3), arr, 3, GL_FLOAT, isStatic));

	return *this;
}

Mesh::Descriptor& Mesh::Descriptor::AddArray(const FColor* arr, bool isStatic)
{
	arrs.push_back(Array(numVertices * sizeof(FColor), arr, 4, GL_FLOAT, isStatic));

	return *this;
}

Mesh::Descriptor& Mesh::Descriptor::AddArray(const float* arr, unsigned int vectorSize, bool isStatic)
{
	arrs.push_back(Array(numVertices * sizeof(float) * vectorSize, arr, vectorSize, GL_FLOAT, isStatic));

	return *this;
}

Mesh::Descriptor& Mesh::Descriptor::AddIndices(const unsigned short* indices, unsigned int numIndices, bool isStatic)
{
	this->indices = indices;
	longIndices = nullptr;
	this->numIndices = numIndices;
	areIndicesStatic = isStatic;

	return *this;
}

Mesh::Descriptor& Mesh::Descriptor::AddIndices(const unsigned int* indices, unsigned int numIndices, bool isStatic)
{
	this->longIndices = indices;
	this->indices = nullptr;
	this->numIndices = numIndices;
	areIndicesStatic = isStatic;

	return *this;
}

Mesh::Descriptor& Mesh::Descriptor::AddBounds(const FVector3 AABBMin, const FVector3& AABBMax, float boundingRadius)
{
	this->AABBMin = AABBMin;
	this->AABBMax = AABBMax;
	this->boundingRadius = boundingRadius;

	return *this;
}

Mesh::Mesh(const Descriptor& descriptor)
{
	AABBMin = descriptor.AABBMin;
	AABBMax = descriptor.AABBMax;
	boundingRadius = descriptor.boundingRadius;

	unsigned int bufferCount = static_cast<unsigned int>(descriptor.arrs.size());
	unsigned int drawCount = descriptor.numVertices;
	if (descriptor.indices != nullptr || descriptor.longIndices != nullptr)
	{
		++bufferCount;
		drawCount = descriptor.numIndices;
	}

	GLuint vertexArrayObject;
	glGenVertexArrays(1, &vertexArrayObject);
	glBindVertexArray(vertexArrayObject);

	GLuint* vertexBufferObjects = new GLuint[bufferCount];
	glGenBuffers(bufferCount, vertexBufferObjects);

	for (unsigned int i = 0; i < descriptor.arrs.size(); ++i)
	{
		glBindBuffer(GL_ARRAY_BUFFER, vertexBufferObjects[i]);
		glBufferData(GL_ARRAY_BUFFER, descriptor.arrs[i].arrDataSize, descriptor.arrs[i].arr, descriptor.arrs[i].isStatic ? GL_STATIC_DRAW : GL_DYNAMIC_DRAW);
		glEnableVertexAttribArray(i);
		glVertexAttribPointer(i, descriptor.arrs[i].typeSize, descriptor.arrs[i].type, GL_FALSE, 0, 0);
	}

	if (descriptor.indices != nullptr)
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vertexBufferObjects[bufferCount - 1]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, descriptor.numIndices * sizeof(unsigned short), descriptor.indices, descriptor.areIndicesStatic ? GL_STATIC_DRAW : GL_DYNAMIC_DRAW);
	}
	else if (descriptor.longIndices != nullptr)
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vertexBufferObjects[bufferCount - 1]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, descriptor.numIndices * sizeof(unsigned int), descriptor.longIndices, descriptor.areIndicesStatic ? GL_STATIC_DRAW : GL_DYNAMIC_DRAW);
	}

	glBindVertexArray(0);

	meshBox = std::shared_ptr<MeshBox>(new MeshBox(vertexArrayObject, vertexBufferObjects, bufferCount, drawCount));
	meshBox->vertexCapacity = descriptor.numVertices;
	meshBox->indexCapacity = descriptor.numIndices;
}

bool Mesh::operator==(const Mesh& other) const
{
	return meshBox == other.meshBox;
}

//#include <iostream>

void Mesh::Substitute(unsigned int index, const float* arr, GLint typeSize, unsigned short offset, unsigned short count)
{
	//std::cerr << "subbing floats " << index << " " << typeSize << " " << count << " " << offset << std::endl;

	glBindBuffer(GL_ARRAY_BUFFER, meshBox->vertexBufferObjects[index]);
	glBufferSubData(GL_ARRAY_BUFFER, offset, count * typeSize * 4, arr);
}

void Mesh::Substitute(unsigned int index, const FVector2* arr, unsigned short offset, unsigned short count)
{
	glBindBuffer(GL_ARRAY_BUFFER, meshBox->vertexBufferObjects[index]);
	glBufferSubData(GL_ARRAY_BUFFER, offset, count * sizeof(FVector2), arr);
}

void Mesh::Substitute(unsigned int index, const FVector3* arr, unsigned short offset, unsigned short count)
{
	glBindBuffer(GL_ARRAY_BUFFER, meshBox->vertexBufferObjects[index]);

	//GLint size = 0;
	//glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);
	//std::cerr << index << " " << offset << " " << count * sizeof(FVector3) << " " << size << std::endl;
	glBufferSubData(GL_ARRAY_BUFFER, offset, count * sizeof(FVector3), arr);
}

void Mesh::Substitute(unsigned int index, const FColor* arr, unsigned short offset, unsigned short count)
{
	glBindBuffer(GL_ARRAY_BUFFER, meshBox->vertexBufferObjects[index]);
	glBufferSubData(GL_ARRAY_BUFFER, offset, count * sizeof(FColor), arr);
}

void Mesh::SubstituteIndices(const unsigned short* indices, unsigned int offset, unsigned int count)
{
	//std::cerr << "subbing unsigned short indices " << count << " " << offset << std::endl;
	
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshBox->vertexBufferObjects[meshBox->bufferCount - 1]);
	glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, offset, count * sizeof(unsigned short), indices);
}

void Mesh::SubstituteIndices(const unsigned int* indices, unsigned int offset, unsigned int count)
{
	//std::cerr << "subbing unsigned int indices " << count << " " << offset << std::endl;
	
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshBox->vertexBufferObjects[meshBox->bufferCount - 1]);
	glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, offset, count * sizeof(unsigned int), indices);
}

void Mesh::Rebuild(const Descriptor& descriptor)
{
	if (meshBox == nullptr || descriptor.numVertices > meshBox->vertexCapacity || descriptor.numIndices > meshBox->indexCapacity)
	{
		//std::cerr << "building new..." << std::endl;
		
		operator=(Mesh(descriptor));
	}
	else
	{
		//std::cerr << "subbing old..." << std::endl;
		
		if (descriptor.indices != nullptr || descriptor.longIndices != nullptr)
			SetDrawCount(descriptor.numIndices);
		else
			SetDrawCount(descriptor.numVertices);
		
		Bind();
		
		//std::cerr << "mesh op " << meshBox->indexCapacity << " " << meshBox->vertexCapacity << std::endl;

		if (descriptor.indices != nullptr)
			SubstituteIndices(descriptor.indices, 0, descriptor.numIndices);
		else if (descriptor.longIndices != nullptr)
			SubstituteIndices(descriptor.longIndices, 0, descriptor.numIndices);
		for (int i = 0; i < descriptor.arrs.size(); ++i)
			Substitute(i, reinterpret_cast<const float*>(descriptor.arrs[i].arr), descriptor.arrs[i].typeSize, 0, descriptor.numVertices);
	}
}
