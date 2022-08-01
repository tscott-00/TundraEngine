#include "rendering.h"

const unsigned int Internal::Storage::Rendering::ENVIRONMENT_MAP_RESOLUTION = 512;
const unsigned int Internal::Storage::Rendering::IRRADIANCE_MAP_RESOLUTION = 32;
const unsigned int Internal::Storage::Rendering::PREFILTERED_ENVIRONMENT_MAP_RESOLUTION = 128;
std::unordered_map<std::string, RenderingPostProcess3*> Internal::Storage::Rendering::postProcessRegistry;
std::vector<RenderingPostProcess3*> Internal::Storage::Rendering::postProcessArray;
bool Internal::Storage::Rendering::isRenderingScene = false;
FBO Internal::Storage::Rendering::renderBuffer;
FBO Internal::Storage::Rendering::envMapFBO;
FBO Internal::Storage::Rendering::multipurposeFBO;
FBO Internal::Storage::Rendering::screenInfluencedFBO;
Texture2D Internal::Storage::Rendering::screenInfluencedNormalAndIORTexture;
Texture2D Internal::Storage::Rendering::screenInfluencedWaterFogTexture;
Texture2D Internal::Storage::Rendering::screenInfluencedOutRayTexture;
Texture2D Internal::Storage::Rendering::screenInfluencedReflectionTexture;
Texture2D Internal::Storage::Rendering::screenInfluencedRefractionTexture;
Shader Internal::Storage::Rendering::finalShader;
Shader Internal::Storage::Rendering::irradianceMapComputation;
Shader Internal::Storage::Rendering::prefilteredEnvironmentMapComputation;
Shader Internal::Storage::Rendering::influencedRefractionShader;
Shader Internal::Storage::Rendering::influencedReflectionShader;
Texture2D Internal::Storage::Rendering::brdfLUT;
Cubemap Internal::Storage::Rendering::defaultCubemap;
Mesh Internal::Storage::Rendering::quadMesh;
Mesh Internal::Storage::Rendering::skyboxMesh;
bool Internal::Storage::Rendering::isDead = true;

// TODO: put all of the typed stuff into a macro
std::unordered_map<std::string, std::vector<ObjectRenderingProcess3*>> Internal::Storage::RenderingObj::processRegistry;
std::vector<ObjectRenderingProcess3*> Internal::Storage::RenderingObj::processes[2];
DirectionalLightComponent3* Internal::Storage::RenderingObj::directionalLight = nullptr;
float Internal::Storage::RenderingObj::exposure = 1.0F;

DirectionalLightComponent3::DirectionalLightComponent3(const FColor& color) :
	color(color)
{ }

DirectionalLightComponent3::DirectionalLightComponent3(const DirectionalLightComponent3& other) :
	color(other.color)
{ }

void DirectionalLightComponent3::OnEnable()
{
	if (Internal::Storage::RenderingObj::directionalLight == nullptr)
		Internal::Storage::RenderingObj::directionalLight = this;
	else
		Console::Error("Only one directional light can be enabled at any given time");
}

void DirectionalLightComponent3::OnDisable()
{
	if (Internal::Storage::RenderingObj::directionalLight == this)
		Internal::Storage::RenderingObj::directionalLight = nullptr;
}

ObjectRenderingProcess3::ObjectRenderingProcess3(Type type) :
	type(type), canRenderToEnvironmentMap(true), auxCameraNear(NAN), auxCameraFar(NAN)
{ }
ObjectRenderingProcess3::~ObjectRenderingProcess3() { }

ObjectCollection::ObjectCollection() :
	attributeSSBO(0),
	attributeSSBOByteCapacity(0)
{ }

ObjectCollection::ObjectCollection(ObjectCollection&& other) :
	objects(std::move(other.objects)),
	attributeSSBO(other.attributeSSBO),
	attributeSSBOByteCapacity(other.attributeSSBOByteCapacity)
{
	other.attributeSSBO = 0;
	other.attributeSSBOByteCapacity = 0;
}

ObjectCollection::~ObjectCollection()
{
	if (attributeSSBO != 0)
		glDeleteBuffers(1, &attributeSSBO);
}

void ObjectCollection::RefreshSSBO(const void* data, GLsizei dataBytes)
{
	if (attributeSSBO == 0)
	{
		glGenBuffers(1, &attributeSSBO);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, attributeSSBO);
		glBufferData(GL_SHADER_STORAGE_BUFFER, dataBytes, data, GL_DYNAMIC_DRAW);
		attributeSSBOByteCapacity = dataBytes;
		//std::cerr << "creating new buffer of size " << dataBytes << std::endl;
	}
	else
	{
		if (dataBytes <= attributeSSBOByteCapacity)
		{
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, attributeSSBO);
			glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, dataBytes, data);
			//std::cerr << "sub new buffer of size " << dataBytes << std::endl;
		}
		else
		{
			glDeleteBuffers(1, &attributeSSBO);

			glGenBuffers(1, &attributeSSBO);
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, attributeSSBO);
			glBufferData(GL_SHADER_STORAGE_BUFFER, dataBytes, data, GL_DYNAMIC_DRAW);
			attributeSSBOByteCapacity = dataBytes;
			//std::cerr << "recreating a buffer of size " << dataBytes << std::endl;
		}
	}
}

ObjectCollection& ObjectCollection::operator=(ObjectCollection&& other)
{
	objects = std::move(other.objects);
	attributeSSBO = other.attributeSSBO;
	attributeSSBOByteCapacity = other.attributeSSBOByteCapacity;

	other.attributeSSBO = 0;
	other.attributeSSBOByteCapacity = 0;

	return *this;
}

RenderComponent3::RenderComponent3() :
	material(nullptr),
	overrideCheck(false)
{ }

RenderComponent3::RenderComponent3(const Mesh& mesh, const std::shared_ptr<Material3>& material, bool overrideCheck) :
	mesh(mesh),
	material(material),
	overrideCheck(overrideCheck)
{ }

RenderComponent3::RenderComponent3(const RenderComponent3& other) :
	mesh(other.mesh),
	material(other.material),
	overrideCheck(other.overrideCheck)
{ }

void RenderComponent3::OnEnable()
{
	if (!Internal::Storage::Rendering::isDead && ((overrideCheck || mesh.IsLoaded()) && material != nullptr))
		for (ObjectRenderingProcess3* process : *(material->process))
			process->AddObject(*this);
}

void RenderComponent3::OnDisable()
{
	if (!Internal::Storage::Rendering::isDead && ((overrideCheck || mesh.IsLoaded()) && material != nullptr))
		for (ObjectRenderingProcess3* process : *(material->process))
			process->RemoveObject(*this);
}

void RenderComponent3::SetMesh(const Mesh& mesh)
{
	bool enabled = this->gameObject->IsEnabled();
	if (enabled)
		OnDisable();
	this->mesh = mesh;
	if (enabled)
		OnEnable();
}

void RenderComponent3::SetMaterial(const std::shared_ptr<Material3>& material)
{
	bool enabled = this->gameObject->IsEnabled();
	if (enabled)
		OnDisable();
	this->material = material;
	if (enabled)
		OnEnable();
}

void RenderComponent3::SetMeshAndMaterial(const Mesh& mesh, const std::shared_ptr<Material3>& material)
{
	bool enabled = this->gameObject->IsEnabled();
	if (enabled)
		OnDisable();
	this->mesh = mesh;
	this->material = material;
	if (enabled)
		OnEnable();
}

RenderComponent3& RenderComponent3::operator=(const RenderComponent3& other)
{
	bool enabled = gameObject->IsEnabled();
	if (enabled)
		OnDisable();
	this->mesh = other.mesh;
	this->material = other.material;
	if (enabled)
		OnEnable();

	return *this;
}

RenderingOpaqueProcess3::RenderingOpaqueProcess3() :
	ObjectRenderingProcess3(ObjectRenderingProcess3::Type::OPAQUE)
{ }

RenderingInfluencedProcess::RenderingInfluencedProcess(bool usesRefractions, bool usesReflections) :
	ObjectRenderingProcess3(Type::INFLUENCED)
{
	requestNum = 0;
	if (usesRefractions)
		requestNum += 0b0000001;
	if (usesReflections)
		requestNum += 0b0000010;
}

RenderingPostProcess3::~RenderingPostProcess3() { }

SkyboxRenderingProcess3::SkyboxRenderingProcess3(const Cubemap& cubemap) :
	shader(GenerateShaderCode::SkyboxVertex(), GenerateShaderCode::SkyboxFragment()),
	cubemap(cubemap)
{ }

void SkyboxRenderingProcess3::Run(const RMatrix4x4& projectionView, const Camera3* camera)
{
	shader.Bind();
	Internal::Storage::Rendering::skyboxMesh.Bind();
	cubemap.Bind(0);
	Shader::LoadMatrix4x4(0, static_cast<FMatrix4x4>(camera->GetProjectionMatrix()));
	Shader::LoadMatrix4x4(1, camera->GetViewRotationMatrix());
	Internal::Storage::Rendering::skyboxMesh.DrawArrays();
}

namespace Internal
{
	namespace Rendering
	{
		void RegisterProcess(const std::string& name, const std::vector<ObjectRenderingProcess3*>& process)
		{
			Internal::Storage::RenderingObj::processRegistry[name] = process;
			for (ObjectRenderingProcess3* p : process)
				Internal::Storage::RenderingObj::processes[p->type].push_back(p);
		}

		void RegisterProcess(const std::string& name, RenderingPostProcess3* process)
		{
			Internal::Storage::Rendering::postProcessRegistry[name] = process;
			Internal::Storage::Rendering::postProcessArray.push_back(process);
		}

		void Init(int width, int height)
		{
			// color0, color1 can be xyz
			//std::cerr << glGetError << std::endl;
			/*Texture2D renderBufferTextures[5] = {
			Texture2D(Texture2D::Descriptor(Texture2D::Descriptor::Image(width, height, GL_RGB16))),
			Texture2D(Texture2D::Descriptor(Texture2D::Descriptor::Image(width, height, GL_RGB16))),
			Texture2D(Texture2D::Descriptor(Texture2D::Descriptor::Image(width, height, GL_RGBA16))),
			Texture2D(Texture2D::Descriptor(Texture2D::Descriptor::Image(width, height, GL_RG8))),
			Texture2D(Texture2D::Descriptor(Texture2D::Descriptor::Image(width, height, GL_RGB16F)))
			};
			Internal::Storage::Rendering::renderBuffer.Reload(&renderBufferTextures[0], 5, width, height, true);*/

			Texture2D renderBufferTextures[1] = {
				Texture2D(Texture2D::Descriptor(Texture2D::Descriptor::Image(width, height, GL_RGB16F)))
			};
			Internal::Storage::Rendering::renderBuffer.Reload(&renderBufferTextures[0], 1, width, height, false);
			Internal::Storage::Rendering::renderBuffer.Bind();
			Internal::Storage::Rendering::renderBuffer.AttachDepthStencilTexture();
			Internal::Storage::Rendering::renderBuffer.rtc_GetDepthStencilTexture().Bind();
			glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_STENCIL_TEXTURE_MODE, GL_DEPTH_COMPONENT);
			Internal::Storage::Rendering::renderBuffer.Unbind();

			Internal::Storage::Rendering::envMapFBO.MakeLone(Internal::Storage::Rendering::ENVIRONMENT_MAP_RESOLUTION, Internal::Storage::Rendering::ENVIRONMENT_MAP_RESOLUTION, true);
			Internal::Storage::Rendering::envMapFBO.Bind();
			Internal::Storage::Rendering::envMapFBO.DrawBuffers(GL_COLOR_ATTACHMENT0);
			Internal::Storage::Rendering::envMapFBO.Unbind();

			Internal::Storage::Rendering::multipurposeFBO.MakeLone(Internal::Storage::Rendering::IRRADIANCE_MAP_RESOLUTION, Internal::Storage::Rendering::IRRADIANCE_MAP_RESOLUTION, false);
			Internal::Storage::Rendering::multipurposeFBO.Bind();
			Internal::Storage::Rendering::multipurposeFBO.DrawBuffers(GL_COLOR_ATTACHMENT0);
			Internal::Storage::Rendering::multipurposeFBO.Unbind();

			// Can't be changed right now because it uses the depth texture from the screen.
			int screenInfluencedMapsWidth = width;
			int screenInfluencedMapsHeight = height;

			Internal::Storage::Rendering::screenInfluencedFBO.MakeLone(screenInfluencedMapsWidth, screenInfluencedMapsHeight);
			Internal::Storage::Rendering::screenInfluencedFBO.Bind();
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, Internal::Storage::Rendering::renderBuffer.rtc_GetDepthStencilTexture().rtc_GetID(), 0);
			Internal::Storage::Rendering::screenInfluencedFBO.Unbind();
			Internal::Storage::Rendering::screenInfluencedNormalAndIORTexture = Texture2D(Texture2D::Descriptor(Texture2D::Descriptor::Image(screenInfluencedMapsWidth, screenInfluencedMapsHeight, GL_RGBA8), false, false, GL_CLAMP_TO_EDGE));
			Internal::Storage::Rendering::screenInfluencedWaterFogTexture = Texture2D(Texture2D::Descriptor(Texture2D::Descriptor::Image(screenInfluencedMapsWidth, screenInfluencedMapsHeight, GL_RGBA8), false, false, GL_CLAMP_TO_EDGE));
			Internal::Storage::Rendering::screenInfluencedOutRayTexture = Texture2D(Texture2D::Descriptor(Texture2D::Descriptor::Image(screenInfluencedMapsWidth, screenInfluencedMapsHeight, GL_RGB8), false, false, GL_CLAMP_TO_EDGE));
			Internal::Storage::Rendering::screenInfluencedReflectionTexture = Texture2D(Texture2D::Descriptor(Texture2D::Descriptor::Image(screenInfluencedMapsWidth, screenInfluencedMapsHeight, GL_RGB16F), false, false, GL_CLAMP_TO_EDGE));
			Internal::Storage::Rendering::screenInfluencedRefractionTexture = Texture2D(Texture2D::Descriptor(Texture2D::Descriptor::Image(screenInfluencedMapsWidth, screenInfluencedMapsHeight, GL_RGB16F), false, false, GL_CLAMP_TO_EDGE));

			std::string quadVertexCode = GenerateShaderCode::QuadVertex();
			Internal::Storage::Rendering::finalShader = Shader(quadVertexCode, GenerateShaderCode::FinalStageFragment());

			std::string cubemapProcessingVertexCode = GenerateShaderCode::CubemapProcessingVertex();
			Internal::Storage::Rendering::irradianceMapComputation = Shader(cubemapProcessingVertexCode, GenerateShaderCode::IrradianceMapComputationFragment());

			Internal::Storage::Rendering::prefilteredEnvironmentMapComputation = Shader(cubemapProcessingVertexCode, GenerateShaderCode::PrefilterdEnvironmentMapComputationFragment());

			Internal::Storage::Rendering::influencedRefractionShader = Shader("Internal/Shaders/refraction");
			Internal::Storage::Rendering::influencedReflectionShader = Shader("Internal/Shaders/reflection");

			// TODO: CCW?
			FVector2 quadPositions[] =
			{
				FVector2(-1.0F,1.0F),
				FVector2(1.0F,1.0F),
				FVector2(-1.0F,-1.0F),
				FVector2(1.0F,-1.0F)
			};
			FVector2 quadTexCoords[] =
			{
				FVector2(0.0F,1.0F),
				FVector2(1.0F,1.0F),
				FVector2(0.0F,0.0F),
				FVector2(1.0F,0.0F)
			};
			Internal::Storage::Rendering::quadMesh = Mesh(Mesh::Descriptor(sizeof(quadPositions) / sizeof(quadPositions[0])).AddArray(quadPositions).AddArray(quadTexCoords));

			FVector3 skyboxVertices[] =
			{
				FVector3(-1.0F,  1.0F, -1.0F),
				FVector3(-1.0F, -1.0F, -1.0F),
				FVector3(1.0F, -1.0F, -1.0F),
				FVector3(1.0F, -1.0F, -1.0F),
				FVector3(1.0F,  1.0F, -1.0F),
				FVector3(-1.0F,  1.0F, -1.0F),

				FVector3(-1.0F, -1.0F,  1.0F),
				FVector3(-1.0F, -1.0F, -1.0F),
				FVector3(-1.0F,  1.0F, -1.0F),
				FVector3(-1.0F,  1.0F, -1.0F),
				FVector3(-1.0F,  1.0F,  1.0F),
				FVector3(-1.0F, -1.0F,  1.0F),

				FVector3(1.0F, -1.0F, -1.0F),
				FVector3(1.0F, -1.0F,  1.0F),
				FVector3(1.0F,  1.0F,  1.0F),
				FVector3(1.0F,  1.0F,  1.0F),
				FVector3(1.0F,  1.0F, -1.0F),
				FVector3(1.0F, -1.0F, -1.0F),

				FVector3(-1.0F, -1.0F,  1.0F),
				FVector3(-1.0F,  1.0F,  1.0F),
				FVector3(1.0F,  1.0F,  1.0F),
				FVector3(1.0F,  1.0F,  1.0F),
				FVector3(1.0F, -1.0F,  1.0F),
				FVector3(-1.0F, -1.0F,  1.0F),

				FVector3(-1.0F,  1.0F, -1.0F),
				FVector3(1.0F,  1.0F, -1.0F),
				FVector3(1.0F,  1.0F,  1.0F),
				FVector3(1.0F,  1.0F,  1.0F),
				FVector3(-1.0F,  1.0F,  1.0F),
				FVector3(-1.0F,  1.0F, -1.0F),

				FVector3(-1.0F, -1.0F, -1.0F),
				FVector3(-1.0F, -1.0F,  1.0F),
				FVector3(1.0F, -1.0F, -1.0F),
				FVector3(1.0F, -1.0F, -1.0F),
				FVector3(-1.0F, -1.0F,  1.0F),
				FVector3(1.0F, -1.0F,  1.0F)
			};
			Internal::Storage::Rendering::skyboxMesh = Mesh(Mesh::Descriptor(sizeof(skyboxVertices) / sizeof(skyboxVertices[0])).AddArray(skyboxVertices));

			Internal::Storage::Rendering::isDead = false;

			glDisable(GL_CULL_FACE);
			glDisable(GL_DEPTH_TEST);
			glDisable(GL_BLEND);

			//Internal::Storage::Rendering::brdfLUT = Texture2D(Texture2D::Descriptor(Texture2D::Descriptor::Image(Internal::Storage::Rendering::BRDF_LUT_RESOLUTION, Internal::Storage::Rendering::BRDF_LUT_RESOLUTION, GL_RGB16F), true, false, GL_CLAMP_TO_EDGE));
			Internal::Storage::Rendering::defaultCubemap = Cubemap(Cubemap::Descriptor(1, 1, GL_RGB16F, false, false));

			/*Internal::Storage::Rendering::multipurposeFBO.Bind();
			glViewport(0, 0, Internal::Storage::Rendering::BRDF_LUT_RESOLUTION, Internal::Storage::Rendering::BRDF_LUT_RESOLUTION);
			Shader brdfLUTShader(quadVertexCode, GenerateShaderCode::BrdfLUTFragment());
			brdfLUTShader.Bind();
			Internal::Storage::Rendering::quadMesh.Bind();
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, Internal::Storage::Rendering::brdfLUT.rtc_GetID(), 0);
			Internal::Storage::Rendering::quadMesh.DrawArrays(GL_TRIANGLE_STRIP);*/

			Internal::Storage::Rendering::multipurposeFBO.Bind();

			glViewport(0, 0, 1, 1);
			glClearColor(0.0F, 0.0F, 0.0F, 1.0F);
			for (unsigned int i = 0; i < 6; ++i)
			{
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, Internal::Storage::Rendering::defaultCubemap.rtc_GetID(), 0);
				glClear(GL_COLOR_BUFFER_BIT);
			}

			glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
		}

		void Clear()
		{
			for (unsigned int i = 0; i < ObjectRenderingProcess3::Type::COUNT; ++i)
			{
				for (ObjectRenderingProcess3* process : Internal::Storage::RenderingObj::processes[i])
					delete process;
				Internal::Storage::RenderingObj::processes[i].clear();
			}

			for (auto const& pair : Internal::Storage::Rendering::postProcessRegistry)
				delete pair.second;
			Internal::Storage::Rendering::postProcessRegistry.clear();
			Internal::Storage::Rendering::isDead = true;
		}
	}
}

namespace Scene
{
	void Render(const Camera3* camera, const Cubemap& skybox)
	{
		if (Internal::Storage::Rendering::isRenderingScene)
		{
			std::cerr << "Scene render must be complete before another pass is started..." << std::endl;
			return;
		}

		RMatrix4x4 projectionView = camera->GetProjectionViewMatrix();

		Internal::Storage::Rendering::isRenderingScene = true;
		// TODO: Excluded layers? (could have a id bit, 8 bits type, 16 bits specific, 8 bits layer?)
		
		FBO& fbo = Internal::Storage::Rendering::renderBuffer;
		fbo.Bind();

		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);
		glDisable(GL_BLEND);
		glClear(GL_DEPTH_BUFFER_BIT);

		for (ObjectRenderingProcess3* opaqueProcess : Internal::Storage::RenderingObj::processes[ObjectRenderingProcess3::Type::OPAQUE])
			dynamic_cast<RenderingOpaqueProcess3*>(opaqueProcess)->Run(projectionView, camera);

		// TODO: This system doesn't make much sense after all.
		Internal::Storage::Rendering::screenInfluencedFBO.Bind();
		// Render data
		GLenum screenInfluencedFBOBuffers[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
		Internal::Storage::Rendering::screenInfluencedFBO.DrawBuffers(&screenInfluencedFBOBuffers[0], 3);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, Internal::Storage::Rendering::screenInfluencedNormalAndIORTexture.rtc_GetID(), 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, Internal::Storage::Rendering::screenInfluencedWaterFogTexture.rtc_GetID(), 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, Internal::Storage::Rendering::screenInfluencedOutRayTexture.rtc_GetID(), 0);
		glClearStencil(0);
		glClear(GL_STENCIL_BUFFER_BIT);
		glEnable(GL_STENCIL_TEST);
		glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
		glDepthMask(false);
		Internal::Storage::Rendering::renderBuffer.rtc_GetDepthStencilTexture().Bind(1); // TODO: Should be passing this
		for (ObjectRenderingProcess3* influencedProcess : Internal::Storage::RenderingObj::processes[ObjectRenderingProcess3::Type::INFLUENCED])
		{
			RenderingInfluencedProcess* cast = dynamic_cast<RenderingInfluencedProcess*>(influencedProcess);
			glStencilFunc(GL_ALWAYS, cast->requestNum, 0xFF);
			cast->RunDataPass(projectionView, camera);
		}
		// Compute refractions
		Internal::Storage::Rendering::screenInfluencedFBO.DrawBuffers(GL_COLOR_ATTACHMENT0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, Internal::Storage::Rendering::screenInfluencedRefractionTexture.rtc_GetID(), 0);
		glDepthMask(true);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);
		glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
		glStencilFunc(GL_EQUAL, 0b0000001, 0b0000001);
		Internal::Storage::Rendering::influencedRefractionShader.Bind();
		Internal::Storage::Rendering::quadMesh.Bind();
		Internal::Storage::Rendering::screenInfluencedNormalAndIORTexture.Bind(0);
		Internal::Storage::Rendering::screenInfluencedWaterFogTexture.Bind(1);
		Internal::Storage::Rendering::renderBuffer.GetTexture(0).Bind(2);
		Internal::Storage::Rendering::quadMesh.DrawArrays(GL_TRIANGLE_STRIP);

		// Compute reflections
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, Internal::Storage::Rendering::screenInfluencedReflectionTexture.rtc_GetID(), 0);
		glStencilFunc(GL_EQUAL, 0b0000010, 0b0000010);
		Internal::Storage::Rendering::influencedReflectionShader.Bind();
		Internal::Storage::Rendering::quadMesh.Bind();
		Internal::Storage::Rendering::screenInfluencedNormalAndIORTexture.Bind(0);
		Internal::Storage::Rendering::screenInfluencedOutRayTexture.Bind(1);
		skybox.Bind(2);
		Internal::Storage::Rendering::quadMesh.DrawArrays(GL_TRIANGLE_STRIP);

		// Render objects
		fbo.Bind();
		glEnable(GL_CULL_FACE);
		glEnable(GL_STENCIL_TEST);
		glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
		for (ObjectRenderingProcess3* influencedProcess : Internal::Storage::RenderingObj::processes[ObjectRenderingProcess3::Type::INFLUENCED])
		{
			RenderingInfluencedProcess* cast = dynamic_cast<RenderingInfluencedProcess*>(influencedProcess);
			glStencilFunc(GL_EQUAL, cast->requestNum, 0xFF);
			cast->Run(projectionView, camera, Internal::Storage::Rendering::screenInfluencedRefractionTexture, Internal::Storage::Rendering::screenInfluencedReflectionTexture);
		}

		glDisable(GL_STENCIL_TEST);
		glDisable(GL_CULL_FACE);
		fbo.Unbind();
		glEnable(GL_FRAMEBUFFER_SRGB);
		Internal::Storage::Rendering::finalShader.Bind();
		Shader::LoadFloat(0, Internal::Storage::RenderingObj::exposure);
		Internal::Storage::Rendering::quadMesh.Bind();
		fbo.GetTexture(0).Bind(0);
		Internal::Storage::Rendering::quadMesh.DrawArrays(GL_TRIANGLE_STRIP);
		glDisable(GL_FRAMEBUFFER_SRGB);

		Internal::Storage::Rendering::isRenderingScene = false;
	}

	// TODO: Incomplete
	void Render(const Camera3* camera, const FBO& finalFbo)
	{
		if (Internal::Storage::Rendering::isRenderingScene)
		{
			std::cerr << "Scene render must be complete before another pass is started..." << std::endl;
			return;
		}

		RMatrix4x4 projectionView = camera->GetProjectionViewMatrix();

		Internal::Storage::Rendering::isRenderingScene = true;
		// TODO: Excluded layers? (could have a id bit, 8 bits type, 16 bits specific, 8 bits layer?)

		FBO& fbo = Internal::Storage::Rendering::renderBuffer;
		fbo.Bind();

		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);
		glDisable(GL_BLEND);
		glClear(GL_DEPTH_BUFFER_BIT);

		for (ObjectRenderingProcess3* opaqueProcess : Internal::Storage::RenderingObj::processes[ObjectRenderingProcess3::Type::OPAQUE])
			dynamic_cast<RenderingOpaqueProcess3*>(opaqueProcess)->Run(projectionView, camera);

		glDisable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);
		fbo.Unbind();
		finalFbo.Bind();
		glEnable(GL_FRAMEBUFFER_SRGB);
		Internal::Storage::Rendering::finalShader.Bind();
		Shader::LoadFloat(0, Internal::Storage::RenderingObj::exposure);
		Internal::Storage::Rendering::quadMesh.Bind();
		fbo.GetTexture(0).Bind(0);
		Internal::Storage::Rendering::quadMesh.DrawArrays(GL_TRIANGLE_STRIP);
		glDisable(GL_FRAMEBUFFER_SRGB);
		finalFbo.Unbind();

		Internal::Storage::Rendering::isRenderingScene = false;
	}

	void ConductProbe(const Transform3& location, const Cubemap& environmentMap)
	{
		GameObject3* cameraObj = Instantiate<GameObject3>(Transform3(location.GetWorldPosition()));
		cameraObj->AddComponent<Camera3>(static_cast<real>(90.0), static_cast<real>(1.0), static_cast<real>(0.1), static_cast<real>(1000.0));
		Camera3* camera = cameraObj->GetComponent<Camera3>();

		RMatrix4x4 viewDirs[] =
		{
			CreateMatrix::LookAt(RVector3(REAL_ZERO, REAL_ZERO, REAL_ZERO), RVector3(REAL_ONE, REAL_ZERO, REAL_ZERO), RVector3(REAL_ZERO, -REAL_ONE, REAL_ZERO)),
			CreateMatrix::LookAt(RVector3(REAL_ZERO, REAL_ZERO, REAL_ZERO), RVector3(-REAL_ONE, REAL_ZERO, REAL_ZERO), RVector3(REAL_ZERO, -REAL_ONE, REAL_ZERO)),
			CreateMatrix::LookAt(RVector3(REAL_ZERO, REAL_ZERO, REAL_ZERO), RVector3(REAL_ZERO, REAL_ONE, REAL_ZERO), RVector3(REAL_ZERO, REAL_ZERO, REAL_ONE)),
			CreateMatrix::LookAt(RVector3(REAL_ZERO, REAL_ZERO, REAL_ZERO), RVector3(REAL_ZERO, -REAL_ONE, REAL_ZERO), RVector3(REAL_ZERO, REAL_ZERO, -REAL_ONE)),
			CreateMatrix::LookAt(RVector3(REAL_ZERO, REAL_ZERO, REAL_ZERO), RVector3(REAL_ZERO, REAL_ZERO, REAL_ONE), RVector3(REAL_ZERO, -REAL_ONE, REAL_ZERO)),
			CreateMatrix::LookAt(RVector3(REAL_ZERO, REAL_ZERO, REAL_ZERO), RVector3(REAL_ZERO, REAL_ZERO, -REAL_ONE), RVector3(REAL_ZERO, -REAL_ONE, REAL_ZERO)),
		};

		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);
		glDisable(GL_BLEND);

		Internal::Storage::Rendering::envMapFBO.Bind();

		//std::cerr << Internal::Storage::Rendering::envMapFBO.rtc_GetID() << std::endl;
		for (unsigned int i = 0; i < 6; ++i)
		{
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, environmentMap.rtc_GetID(), 0);

			glClear(GL_DEPTH_BUFFER_BIT);

			camera->SetViewRotationMatrix(viewDirs[i]);
			RMatrix4x4 projectionView = camera->GetProjectionViewMatrix();
			//std::cerr << glm::to_string(projectionView) << std::endl;

			for (ObjectRenderingProcess3* opaqueProcess : Internal::Storage::RenderingObj::processes[ObjectRenderingProcess3::Type::OPAQUE])
				if (opaqueProcess->canRenderToEnvironmentMap)
					dynamic_cast<RenderingOpaqueProcess3*>(opaqueProcess)->Run(projectionView, camera);

			glMemoryBarrier(GL_FRAMEBUFFER_BARRIER_BIT);
		}

		Destroy(cameraObj);

		Internal::Storage::Rendering::envMapFBO.Unbind();
	}

	void ComputeIrradiance(const Cubemap& environmentMap, const Cubemap& irradianceMap)
	{
		FMatrix4x4 projection = CreateMatrix::Projection(90.0F, 1.0F, 0.1F, 10.0F);
		FMatrix4x4 viewDirs[] =
		{
			CreateMatrix::LookAt(FVector3(0.0F, 0.0F, 0.0F), FVector3(1.0F, 0.0F, 0.0F), FVector3(0.0F, -1.0F, 0.0F)),
			CreateMatrix::LookAt(FVector3(0.0F, 0.0F, 0.0F), FVector3(-1.0F, 0.0F, 0.0F), FVector3(0.0F, -1.0F, 0.0F)),
			CreateMatrix::LookAt(FVector3(0.0F, 0.0F, 0.0F), FVector3(0.0F, 1.0F, 0.0F), FVector3(0.0F, 0.0F, 1.0F)),
			CreateMatrix::LookAt(FVector3(0.0F, 0.0F, 0.0F), FVector3(0.0F, -1.0F, 0.0F), FVector3(0.0F, 0.0F, -1.0F)),
			CreateMatrix::LookAt(FVector3(0.0F, 0.0F, 0.0F), FVector3(0.0F, 0.0F, 1.0F), FVector3(0.0F, -1.0F, 0.0F)),
			CreateMatrix::LookAt(FVector3(0.0F, 0.0F, 0.0F), FVector3(0.0F, 0.0F, -1.0F), FVector3(0.0F, -1.0F, 0.0F)),
		};

		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_BLEND);

		Internal::Storage::Rendering::multipurposeFBO.Bind();
		Internal::Storage::Rendering::irradianceMapComputation.Bind();
		Internal::Storage::Rendering::skyboxMesh.Bind();
		environmentMap.Bind(0);

		for (unsigned int i = 0; i < 6; ++i)
		{
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, irradianceMap.rtc_GetID(), 0);

			Shader::LoadMatrix4x4(0, projection * viewDirs[i]);
			Internal::Storage::Rendering::skyboxMesh.DrawArrays();

			glMemoryBarrier(GL_FRAMEBUFFER_BARRIER_BIT);
		}
	}

	void ComputePrefilteredEnvironmentMap(const Cubemap& environmentMap, const Cubemap& prefilteredEnvironmentMap)
	{
		FMatrix4x4 projection = CreateMatrix::Projection(90.0F, 1.0F, 0.1F, 1000.0F);
		FMatrix4x4 viewDirs[] =
		{
			CreateMatrix::LookAt(FVector3(0.0F, 0.0F, 0.0F), FVector3(1.0F, 0.0F, 0.0F), FVector3(0.0F, -1.0F, 0.0F)),
			CreateMatrix::LookAt(FVector3(0.0F, 0.0F, 0.0F), FVector3(-1.0F, 0.0F, 0.0F), FVector3(0.0F, -1.0F, 0.0F)),
			CreateMatrix::LookAt(FVector3(0.0F, 0.0F, 0.0F), FVector3(0.0F, 1.0F, 0.0F), FVector3(0.0F, 0.0F, 1.0F)),
			CreateMatrix::LookAt(FVector3(0.0F, 0.0F, 0.0F), FVector3(0.0F, -1.0F, 0.0F), FVector3(0.0F, 0.0F, -1.0F)),
			CreateMatrix::LookAt(FVector3(0.0F, 0.0F, 0.0F), FVector3(0.0F, 0.0F, 1.0F), FVector3(0.0F, -1.0F, 0.0F)),
			CreateMatrix::LookAt(FVector3(0.0F, 0.0F, 0.0F), FVector3(0.0F, 0.0F, -1.0F), FVector3(0.0F, -1.0F, 0.0F)),
		};

		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_BLEND);

		Internal::Storage::Rendering::multipurposeFBO.Bind();
		Internal::Storage::Rendering::prefilteredEnvironmentMapComputation.Bind();
		Internal::Storage::Rendering::skyboxMesh.Bind();
		environmentMap.Bind(0);

		unsigned int maxMipLevels = 5;
		for (unsigned int i = 0; i < 6; ++i)
		{
			Shader::LoadMatrix4x4(0, projection * viewDirs[i]);

			for (unsigned int mip = 0; mip < maxMipLevels; ++mip)
			{
				unsigned int size = static_cast<unsigned int>(128.0 * pow(0.5, mip));
				glViewport(0, 0, size, size);

				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, prefilteredEnvironmentMap.rtc_GetID(), mip);

				Shader::LoadFloat(1, static_cast<float>(mip) / static_cast<float>(maxMipLevels - 1));
				Internal::Storage::Rendering::skyboxMesh.DrawArrays();

				glMemoryBarrier(GL_FRAMEBUFFER_BARRIER_BIT);
			}
		}

		Internal::Storage::Rendering::multipurposeFBO.Unbind();
	}
}