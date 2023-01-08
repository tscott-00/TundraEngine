#pragma once

#include "rendering.h"

// Animated particle systems are single-use. The animations will not play properly if the system is used between multiple active objects.
// TODO: Could actually have a different class do animations and only allow one instance of the system in the list (hash of system ptr to int count)
class BillboardParticleSystem : public Component3 {
	friend class BillboardParticleRenderProcess;
private:
	Texture2D texture;
	GLsizei maxParticleCount;

	GLuint particleSSBO;
	GLsizei particleCount;

	float fadeStart;
	float fadeEnd;
public:
	class Animation {

	};

	struct ParticleData {
		FVector4 positionAndScale;
		FColor color;
		FVector4 subTexCoords;

		ParticleData();
		ParticleData(const FVector4& positionAndScale, const FColor& color, const FVector4& subTexCoords);
	};

	/*
	If particleData != nullptr, it must have maxParticleCount elements.
	*/
	BillboardParticleSystem(const Texture2D& texture, ParticleData* particleData, GLsizei maxParticleCount, bool staticDraw, float fadeStart, float fadeEnd);
	~BillboardParticleSystem();

	// data starts and index 0 and replaces the system's data starting at index dataOffset
	void UploadState(ParticleData* data, uint32_t dataSize, uint32_t dataOffset);
	
	void OnEnable() override;
	void OnDisable() override;

	cant_copy(BillboardParticleSystem);
};

class BillboardParticleRenderProcess : public RenderingOpaqueProcess3 {
private:
	const Shader shader;
	std::unordered_map<Texture2D, std::list<BillboardParticleSystem*>> particleSystems;
public:
	BillboardParticleRenderProcess();

	void Run(const RMatrix4x4& projectionView, const Camera3* camera);

	void AddObject(BillboardParticleSystem* system);
	void RemoveObject(BillboardParticleSystem* system);

	// Using these is invalid, materials should not be particles
	void AddObject(const RenderComponent3& object) override;
	void RemoveObject(const RenderComponent3& object) override;
};
