#include "particles.h"
#include "events_state.h"

BillboardParticleSystem::ParticleData::ParticleData() :
	positionAndScale(0.0F, 0.0F, 0.0F, 1.0F),
	color(FColor::WHITE),
	subTexCoords(0.0, 0.0, 1.0, 1.0)
{ }

BillboardParticleSystem::ParticleData::ParticleData(const FVector4& positionAndScale, const FColor& color, const FVector4& subTexCoords) :
	positionAndScale(positionAndScale),
	color(color),
	subTexCoords(subTexCoords)
{ }

BillboardParticleSystem::BillboardParticleSystem(const Texture2D& texture, ParticleData* particleData, GLsizei maxParticleCount, bool staticDraw, float fadeStart, float fadeEnd) :
	texture(texture),
	maxParticleCount(maxParticleCount),
	fadeStart(fadeStart),
	fadeEnd(fadeEnd) {
	particleCount = maxParticleCount;

	glGenBuffers(1, &particleSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, particleSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, maxParticleCount * sizeof(ParticleData), particleData != nullptr ? particleData : 0, staticDraw ? GL_STATIC_DRAW : GL_DYNAMIC_DRAW);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

BillboardParticleSystem::~BillboardParticleSystem() {
	glDeleteBuffers(1, &particleSSBO);
}

void BillboardParticleSystem::OnEnable() {
	if (!Internal::Storage::Rendering::isDead)
		dynamic_cast<BillboardParticleRenderProcess*>(Internal::Storage::RenderingObj::processRegistry["BillboardParticles"][0])->AddObject(this);
}

void BillboardParticleSystem::OnDisable() {
	if (!Internal::Storage::Rendering::isDead)
		dynamic_cast<BillboardParticleRenderProcess*>(Internal::Storage::RenderingObj::processRegistry["BillboardParticles"][0])->RemoveObject(this);
}

BillboardParticleRenderProcess::BillboardParticleRenderProcess() :
	shader("Internal/Shaders/billboard_particles")
{ }

void BillboardParticleRenderProcess::Run(const RMatrix4x4& projectionView, const Camera3* camera) {
	RMatrix4x4 myProjectionView;
	// Still tests for depth, just doesn't write, so it will not work in a full scene with aux camera planes.
	if (!isnan(auxCameraNear)) {
		real oldNear = camera->GetZNear();
		real oldFar = camera->GetZFar();
		const_cast<Camera3*>(camera)->Adjust(auxCameraNear, auxCameraFar);
		myProjectionView = camera->GetProjectionViewMatrix();
		const_cast<Camera3*>(camera)->Adjust(oldNear, oldFar);
	} else
		myProjectionView = projectionView;

	DVector3 camPos = camera->gameObject->transform.GetWorldPosition();

	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	glDisable(GL_CULL_FACE);
	glDepthMask(GL_FALSE);

	shader.Bind();
	Internal::Storage::Rendering::quadMesh.Bind();
	for (auto pair : particleSystems) {
		pair.first.Bind(0);
		for (BillboardParticleSystem* system : pair.second) {
			Shader::LoadMatrix4x4(0, static_cast<FMatrix4x4>(myProjectionView * system->gameObject->transform.GetMatrix()));
			Shader::LoadFloat(1, 1.0F / EventsState::GetAspectRatio());
			Shader::LoadVector3(2, static_cast<FVector3>((camPos - system->gameObject->transform.GetWorldPosition())));
			Shader::LoadFloat(3, system->fadeStart);
			Shader::LoadFloat(4, system->fadeEnd);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, system->particleSSBO);
			glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, Internal::Storage::Rendering::quadMesh.GetDrawCount(), system->particleCount);
		}
	}

	glDepthMask(GL_TRUE);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glDisable(GL_BLEND);
}

void BillboardParticleRenderProcess::AddObject(BillboardParticleSystem* system) {
	// TODO: Verify that Texture2D is working with the hashing
	particleSystems[system->texture].push_back(system);
}

void BillboardParticleRenderProcess::RemoveObject(BillboardParticleSystem* system) {
	std::list<BillboardParticleSystem*>& ls = particleSystems[system->texture];
	ls.remove(system);
	if (ls.empty())
		particleSystems.erase(system->texture);
}

void BillboardParticleRenderProcess::AddObject(const RenderComponent3& object) { }
void BillboardParticleRenderProcess::RemoveObject(const RenderComponent3& object) { }
