#pragma once

#include "general_common.h"
#include "updateable.h"
#include "fbo.h"
#include "rreal.h"
//#include "physics.h"

template<class T>
class TTerrainSystem3 : public TUpdateableGameObject<T>
{
public:
	float smallestChunkScaleRadius;
	FBO elevationMapArray;
	FBO elevationRelatedDataMapArray;
private:
	static const unsigned int QUAD_TREE_CHILDREN_COUNT = 4;
	static const int DATA_MAPS_SIZE = 128;
	static const int DATA_MAPS_ARRAY_SIZE = 1;

	struct Chunk : public TGameObject<T>
	{
		Chunk* children[QUAD_TREE_CHILDREN_COUNT];
		// 1.0F when LOD = 0, grows by 2x every LOD level
		unsigned __int32 LODPosX, LODPosZ;
		unsigned __int8 LOD;
		float LODScaleMultiplier;
		int allocatedDataMapIndex, allocatedDataMapIndexX, allocatedDataMapIndexY;
		TTerrainSystem3<T>* system;

		Chunk() {}
		Chunk(const Chunk& other) : TGameObject<T>(other) { }

		Chunk* rtc_GetCopy()
		{
			return new Chunk(*this);
		}

		void InitData(unsigned __int32 LOD, unsigned __int32 LODPosX, unsigned __int32 LODPosZ, float LODScaleMultiplier, int allocatedDataMapIndex, TTerrainSystem3<T>* system)
		{
			this->LOD = LOD;
			this->LODPosX = LODPosX;
			this->LODPosZ = LODPosZ;
			this->LODScaleMultiplier = LODScaleMultiplier;
			this->allocatedDataMapIndex = allocatedDataMapIndex;
			this->system = system;
			allocatedDataMapIndexX = allocatedDataMapIndex % DATA_MAPS_ARRAY_SIZE;
			allocatedDataMapIndexY = allocatedDataMapIndex / DATA_MAPS_ARRAY_SIZE;
			this->transform.SetScale(FVector3(system->smallestChunkScaleRadius * LODScaleMultiplier, 1.0F, system->smallestChunkScaleRadius * LODScaleMultiplier));
			this->transform.SetParent(system->transform);

			TRenderComponent3<T>* c = new TRenderComponent3<T>(nullptr, &system->surfaceMaterial);
			c->overrideCheck = true;
			this->AddExistingComponent(c);
		}
	};

	float absoluteMaxHeight;
	double* localHeights;
	//TTerrainCollider<T>* collider;

	Chunk largestChunk;

	//FBO elevationMapCopyArray;
	Texture2D elevationBrushes;
	int elevationBrushesGridSize;
	float normalizedBrushSize;
	RReal random;

	Mesh* quadMesh;
	TMaterial3<T> surfaceMaterial;
	Texture2DArray albedoAndHeightArray;
	Texture2DArray materialArray;
	Texture2DArray normalsArray;

	Shader elevationPaintShader;
	// need mag also
	//Shader elevationCopyShader;
	Shader elevationRelatedDataGenerationShader;

	unsigned __int32 currentLODPosX, currentLODPosZ;
	unsigned __int8 currentMinVisibleLOD;
public:
	class RenderingProcess : public TRenderingOpaqueProcess3<T>
	{
	private:
		std::unordered_map<const TTerrainSystem3*, std::list<Chunk*>> activeChunkMap;

		const Shader shader;
		Mesh chunkMesh;
	public:
		RenderingProcess() : shader("Internal/Shaders/Rendering/Terrain/land")
		{
			FVector2 vertices[DATA_MAPS_SIZE * DATA_MAPS_SIZE];
			glm::vec2 texCoords[DATA_MAPS_SIZE * DATA_MAPS_SIZE];
			glm::vec2 heightTexCoords[DATA_MAPS_SIZE * DATA_MAPS_SIZE];
			int indicesCount = 6 * (DATA_MAPS_SIZE - 1) * (DATA_MAPS_SIZE - 1);
			unsigned short* indices = new unsigned short[indicesCount];
			int indexPointer = 0;

			float size = 1.0F / (DATA_MAPS_SIZE - 1);
			int vertexPointer = 0;
			for (unsigned int z = 0; z < DATA_MAPS_SIZE; ++z)
			{
				for (unsigned int x = 0; x < DATA_MAPS_SIZE; ++x)
				{
					vertices[vertexPointer].x = -0.5F + x * size;
					vertices[vertexPointer].y = -0.5F + z * size;
					texCoords[vertexPointer].x = static_cast<float>(x);
					texCoords[vertexPointer].y = static_cast<float>(z);
					// TODO: VBO should be ints
					heightTexCoords[vertexPointer].x = static_cast<float>(x);
					heightTexCoords[vertexPointer].y = static_cast<float>(z);

					++vertexPointer;
				}
			}

			for (unsigned int lgz = 0; lgz < DATA_MAPS_SIZE - 1; ++lgz)
			{
				for (unsigned int lgx = 0; lgx < DATA_MAPS_SIZE - 1; ++lgx)
				{
					unsigned int topLeft = (lgz * DATA_MAPS_SIZE) + lgx;
					unsigned int topRight = topLeft + 1;
					unsigned int bottomLeft = ((lgz + 1) * DATA_MAPS_SIZE) + lgx;
					unsigned int bottomRight = bottomLeft + 1;
					indices[indexPointer++] = topLeft;
					indices[indexPointer++] = bottomLeft;
					indices[indexPointer++] = topRight;
					indices[indexPointer++] = topRight;
					indices[indexPointer++] = bottomLeft;
					indices[indexPointer++] = bottomRight;
				}
			}

			chunkMesh.Reload(Mesh::Descriptor(DATA_MAPS_SIZE * DATA_MAPS_SIZE).AddArray(&vertices[0]).AddArray(&texCoords[0]).AddArray(&heightTexCoords[0]).AddIndices(indices, indicesCount));
		}

		void AddObject(const TRenderComponent3<T>& object) override
		{
			Chunk* chunk = reinterpret_cast<Chunk*>(object.gameObject);
			activeChunkMap[chunk->system].push_back(chunk);
		}

		void RemoveObject(const TRenderComponent3<T>& object) override
		{
			Chunk* chunk = reinterpret_cast<Chunk*>(object.gameObject);
			auto systemPair = activeChunkMap.find(chunk->system);
			systemPair->second.remove(chunk);
			if (systemPair->second.empty())
				activeChunkMap.erase(systemPair);
		}

		void Run(const FMatrix4x4& projectionView) override
		{
			shader.Bind();
			chunkMesh.Bind();
			for (auto& systemPair : activeChunkMap)
			{
				systemPair.first->elevationRelatedDataMapArray.GetTexture(0).Bind(0);
				systemPair.first->elevationMapArray.GetTexture(0).Bind(1);
				systemPair.first->elevationRelatedDataMapArray.GetTexture(0).Bind(2);
				systemPair.first->albedoAndHeightArray.Bind(3);
				systemPair.first->materialArray.Bind(4);
				systemPair.first->normalsArray.Bind(5);
				for (Chunk* chunk : systemPair.second)
				{
					Shader::LoadMatrix4x4(0, projectionView * chunk->transform.GetFloatMatrix());
					Shader::LoadVector2(1, IVector2(chunk->allocatedDataMapIndexX * DATA_MAPS_SIZE, chunk->allocatedDataMapIndexY * DATA_MAPS_SIZE));
					chunkMesh.DrawElements();
				}
			}
		}
	};

	TTerrainSystem3(float smallestChunkScaleRadius, float absoluteMaxHeight, unsigned __int8 largestLOD, const Texture2D& elevationBrushes, int elevationBrushesGridSize, const Texture2DArray& albedoAndHeightArray, const Texture2DArray& materialArray, const Texture2DArray& normalsArray) :
		surfaceMaterial("TerLand"),
		albedoAndHeightArray(albedoAndHeightArray),
		materialArray(materialArray),
		normalsArray(normalsArray)
	{
		this->smallestChunkScaleRadius = smallestChunkScaleRadius;
		this->absoluteMaxHeight = absoluteMaxHeight;
		this->elevationBrushes = elevationBrushes;
		this->elevationBrushesGridSize = elevationBrushesGridSize;
		normalizedBrushSize = 1.0F / elevationBrushesGridSize;
		quadMesh = &Internal::Storage::Rendering::quadMesh;
		random = 133742069;

		localHeights = new double[DATA_MAPS_SIZE * DATA_MAPS_SIZE];
		//for (int i = 0; i < 32 * 32; ++i)
		//	localHeights[i] = 0.0;
		// Removed temporarily
		//collider = new TTerrainCollider<T>(localHeights, DATA_MAPS_SIZE, -static_cast<double>(absoluteMaxHeight), static_cast<double>(absoluteMaxHeight));
		//this->AddExistingComponent(collider);

		elevationPaintShader = Shader("Internal/Shaders/Processing/Terrain/elevation_paint");
		//elevationCopyShader = Shader();
		elevationRelatedDataGenerationShader = Shader("Internal/Shaders/Processing/Terrain/elevation_related_data");

		int dataMapsArrayPixelSize = DATA_MAPS_SIZE * DATA_MAPS_ARRAY_SIZE;
		Texture2D elevationMapArrayTex(Texture2D::Descriptor(Texture2D::Descriptor::Image(dataMapsArrayPixelSize, dataMapsArrayPixelSize, GL_R32F)));
		elevationMapArray.Reload(&elevationMapArrayTex, 1, dataMapsArrayPixelSize, dataMapsArrayPixelSize);
		/*Texture2D elevationRelatedDataMapArrayTextures[2] = {
			Texture2D(Texture2D::Descriptor(Texture2D::Descriptor::Image(dataMapsArrayPixelSize, dataMapsArrayPixelSize, GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE))),
			Texture2D(Texture2D::Descriptor(Texture2D::Descriptor::Image(dataMapsArrayPixelSize, dataMapsArrayPixelSize, GL_R8I, GL_RED, GL_UNSIGNED_BYTE)))
		};
		elevationRelatedDataMapArray.Reload(&elevationRelatedDataMapArrayTextures[0], 2, dataMapsArrayPixelSize, dataMapsArrayPixelSize);*/
		//Texture2D elevationRelatedDataMapArrayTex(Texture2D::Descriptor(Texture2D::Descriptor::Image(dataMapsArrayPixelSize, dataMapsArrayPixelSize, GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE)));
		Texture2D elevationRelatedDataMapArrayTex(Texture2D::Descriptor(Texture2D::Descriptor::Image(dataMapsArrayPixelSize, dataMapsArrayPixelSize, GL_RGB8)));
		elevationRelatedDataMapArray.Reload(&elevationRelatedDataMapArrayTex, 1, dataMapsArrayPixelSize, dataMapsArrayPixelSize);

		//int elevationMapCopyArrayPixelSize = DATA_MAPS_SIZE * 2;
		//Texture2D elevationMapCopyArrayTex(Texture2D::Descriptor(Texture2D::Descriptor::Image(elevationMapCopyArrayPixelSize, elevationMapCopyArrayPixelSize, GL_R32F, GL_RED, GL_FLOAT)));
		//elevationMapCopyArray.Reload(&elevationMapCopyArrayTex, 1, elevationMapCopyArrayPixelSize, elevationMapCopyArrayPixelSize);

		largestChunk.InitData(largestLOD, 0, 0, powf(2.0F, static_cast<float>(largestLOD)), 0, this);
	}

	void Generate(int index0, int index1, int layers)
	{
		glDisable(GL_CULL_FACE);
		glDisable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE);

		elevationMapArray.Bind();
		elevationPaintShader.Bind();
		quadMesh->Bind();
		elevationBrushes.Bind(0);
		glClearColor(0.0F, 0.0F, 0.0F, 0.0F);
		glClear(GL_COLOR_BUFFER_BIT);
		/*Shader::LoadVector2(1, FVector2(normalizedBrushSize, normalizedBrushSize));
		Shader::LoadFloat(4, 10.0F);
		int primaryBrushIndex = random.GetByte(0, 0) % (elevationBrushesGridSize * elevationBrushesGridSize);
		int primaryBrushX = primaryBrushIndex % elevationBrushesGridSize;
		int primaryBrushY = primaryBrushIndex / elevationBrushesGridSize;
		int secondaryBrushIndex = random.GetByte(69, 420) % (elevationBrushesGridSize * elevationBrushesGridSize);
		int secondaryBrushX = secondaryBrushIndex % elevationBrushesGridSize;
		int secondaryBrushY = secondaryBrushIndex / elevationBrushesGridSize;
		FVector2 dataMapArrayElementSize(1.0F / DATA_MAPS_ARRAY_SIZE, 1.0F / DATA_MAPS_ARRAY_SIZE);
		FVector2 dataMapArrayElementPos(
		-1.0F + 1.0F / DATA_MAPS_ARRAY_SIZE + largestChunk.allocatedDataMapIndexX * 2.0F / DATA_MAPS_ARRAY_SIZE,
		-1.0F + 1.0F / DATA_MAPS_ARRAY_SIZE + largestChunk.allocatedDataMapIndexX * 2.0F / DATA_MAPS_ARRAY_SIZE);
		FTransform2 brushTransform(dataMapArrayElementPos, dataMapArrayElementSize);
		Shader::LoadMatrix3x3(0, brushTransform.GetMatrix());
		Shader::LoadVector2(2, FVector2(primaryBrushX * normalizedBrushSize, primaryBrushY * normalizedBrushSize));
		Shader::LoadVector2(3, FVector2(secondaryBrushX * normalizedBrushSize, secondaryBrushY * normalizedBrushSize));
		quadMesh->DrawArrays(GL_TRIANGLE_STRIP);*/
		FVector2 dataMapArrayElementSize(1.0F / DATA_MAPS_ARRAY_SIZE, 1.0F / DATA_MAPS_ARRAY_SIZE);
		FVector2 dataMapArrayElementPos(
			-1.0F + 1.0F / DATA_MAPS_ARRAY_SIZE + largestChunk.allocatedDataMapIndexX * 2.0F / DATA_MAPS_ARRAY_SIZE,
			-1.0F + 1.0F / DATA_MAPS_ARRAY_SIZE + largestChunk.allocatedDataMapIndexX * 2.0F / DATA_MAPS_ARRAY_SIZE);
		for (int i = 0; i < layers; ++i)
		{
			Shader::LoadVector2(1, FVector2(normalizedBrushSize, normalizedBrushSize));
			Shader::LoadFloat(4, 1.0F);
			int primaryBrushIndex = abs(random.GetByte(0, 0 + i * 241901)) % (elevationBrushesGridSize * elevationBrushesGridSize);
			int primaryBrushX = primaryBrushIndex % elevationBrushesGridSize;
			int primaryBrushY = primaryBrushIndex / elevationBrushesGridSize;
			int secondaryBrushIndex = abs(random.GetByte(69 + i * 8914, 420)) % (elevationBrushesGridSize * elevationBrushesGridSize);
			int secondaryBrushX = secondaryBrushIndex % elevationBrushesGridSize;
			int secondaryBrushY = secondaryBrushIndex / elevationBrushesGridSize;
			FTransform2 brushTransform(dataMapArrayElementPos, dataMapArrayElementSize);
			Shader::LoadMatrix3x3(0, brushTransform.GetMatrix());
			Shader::LoadVector2(2, FVector2(primaryBrushX * normalizedBrushSize, primaryBrushY * normalizedBrushSize));
			Shader::LoadVector2(3, FVector2(secondaryBrushX * normalizedBrushSize, secondaryBrushY * normalizedBrushSize));
			quadMesh->DrawArrays(GL_TRIANGLE_STRIP);
		}

		glDisable(GL_BLEND);
		elevationRelatedDataMapArray.Bind();
		elevationRelatedDataGenerationShader.Bind();
		quadMesh->Bind();
		elevationMapArray.GetTexture(0).Bind(0);
		Shader::LoadVector2(0, FVector2(static_cast<float>(DATA_MAPS_SIZE), static_cast<float>(DATA_MAPS_SIZE)));
		Shader::LoadVector2(1, IVector2(largestChunk.allocatedDataMapIndexX * DATA_MAPS_SIZE, largestChunk.allocatedDataMapIndexY * DATA_MAPS_SIZE));
		Shader::LoadVector2(2, dataMapArrayElementSize);
		Shader::LoadVector2(3, dataMapArrayElementPos);
		Shader::LoadVector2(4, IVector2(index0, index1));
		// load dest size, dest pos, src size, src offset
		quadMesh->DrawArrays(GL_TRIANGLE_STRIP);

		float* rawData = new float[DATA_MAPS_SIZE * DATA_MAPS_SIZE];
		elevationMapArray.Bind();
		glReadPixels(0, 0, DATA_MAPS_SIZE, DATA_MAPS_SIZE, GL_RED, GL_FLOAT, rawData);
		for (int z = 0; z < DATA_MAPS_SIZE; z++)
		{
			for (int x = 0; x < DATA_MAPS_SIZE; x++)
			{
				localHeights[x + z * DATA_MAPS_SIZE] = static_cast<double>(rawData[x + z * DATA_MAPS_SIZE]);
			}
		}
		delete[] rawData;
	}

	virtual ~TTerrainSystem3()
	{
		delete[] localHeights;
	}

	// Currently not allowed because FBO doesn't have = or copy constructor
	TTerrainSystem3* rtc_GetCopy() const override
	{
		return nullptr;
	}

	void Update(const GameTime& time) override
	{

	}
};

namespace Internal
{
	namespace Terrain
	{
		template<class T>
		void Init()
		{
			std::vector<TObjectRenderingProcess3<T>*> terLandProcess;
			terLandProcess.push_back(new typename TTerrainSystem3<T>::RenderingProcess());
			Rendering::RegisterProcess("TerLand", terLandProcess);
		}
	}
}

typedef TTerrainSystem3<FTransform3> FTerrainSystem3;
typedef TTerrainSystem3<DTransform3> DTerrainSystem3;