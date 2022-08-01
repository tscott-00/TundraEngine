#pragma once

#include <list>

#include "general_common.h"
#include "shader.h"
#include "texture.h"
#include "game_object.h"
#include "mesh.h"
#include "fbo.h"
#include "color.h"
#include "debug.h"
#include "shader_generator.h"
#include "camera.h"

class ObjectRenderingProcess3;
class RenderingPostProcess3;
class DirectionalLightComponent3;

namespace Internal
{
	namespace Storage
	{
		struct RenderingObj
		{
			static std::unordered_map<std::string, std::vector<ObjectRenderingProcess3*>> processRegistry;
			static std::vector<ObjectRenderingProcess3*> processes[2];
			//static std::unordered_map<GLuint, std::list<TLight<T>*>> lights;
			static DirectionalLightComponent3* directionalLight;
			static float exposure;
		};

		struct Rendering
		{
			static const unsigned int ENVIRONMENT_MAP_RESOLUTION;
			static const unsigned int IRRADIANCE_MAP_RESOLUTION;
			static const unsigned int PREFILTERED_ENVIRONMENT_MAP_RESOLUTION;
			static std::unordered_map<std::string, RenderingPostProcess3*> postProcessRegistry;
			static std::vector<RenderingPostProcess3*> postProcessArray;
			static bool isRenderingScene;
			// TODO: Use this or seperate fbos for each stage?
			//static FBO renderBuffer; // color, color, normal+depth, material, light color
			static FBO renderBuffer; // color
			static FBO envMapFBO;
			static FBO multipurposeFBO;
			static FBO screenInfluencedFBO;
			static Texture2D screenInfluencedNormalAndIORTexture;
			static Texture2D screenInfluencedWaterFogTexture;
			static Texture2D screenInfluencedOutRayTexture;
			static Texture2D screenInfluencedReflectionTexture;
			static Texture2D screenInfluencedRefractionTexture;
			static Shader finalShader;
			static Shader irradianceMapComputation;
			static Shader prefilteredEnvironmentMapComputation;
			static Shader influencedRefractionShader;
			static Shader influencedReflectionShader;
			static Texture2D brdfLUT;
			static Cubemap defaultCubemap;
			static Mesh quadMesh;
			static Mesh skyboxMesh;
			//static Mesh pointLightVolume;
			static bool isDead;
		};
	}
}

class DirectionalLightComponent3 : public Component3
{
public:
	FColor color;

	DirectionalLightComponent3(const FColor& color);
	DirectionalLightComponent3(const DirectionalLightComponent3& other);

	void OnEnable() override;
	void OnDisable() override;

	can_copy(DirectionalLightComponent3)
};

class Material3
{
public:
	const std::vector<ObjectRenderingProcess3*>* process;

	Material3(const std::string& processName) : process(&Internal::Storage::RenderingObj::processRegistry[processName]) {}
};

class RenderComponent3;

class ObjectRenderingProcess3 //: public Suspendable
{
public:
	enum Type
	{
		OPAQUE,
		INFLUENCED,

		COUNT
	};
	const Type type;
	bool canRenderToEnvironmentMap;
	// Aux variables should only be used for things that do not rely on the depth buffer.
	real auxCameraNear;
	real auxCameraFar;

	ObjectRenderingProcess3(Type type);
	virtual ~ObjectRenderingProcess3();

	virtual void AddObject(const RenderComponent3& object) = 0;
	virtual void RemoveObject(const RenderComponent3& object) = 0;

	no_transfer_functions(ObjectRenderingProcess3);
};

struct ObjectCollection
{
	std::vector<Transform3*> objects;
	GLuint attributeSSBO; // 0 if it is not to be instanced!
	GLsizei attributeSSBOByteCapacity;

	ObjectCollection();
	ObjectCollection(ObjectCollection&& other);
	~ObjectCollection();

	void RefreshSSBO(const void* data, GLsizei dataBytes/*TODO: Add oversize back?, GLsizei oversizeInBytes*/);

	inline void Bind() const
	{
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, attributeSSBO);
	}

	ObjectCollection& operator=(ObjectCollection&& other);
};

class RenderComponent3 : public Component3
{
private:
	Mesh mesh;
	std::shared_ptr<Material3> material;
public:
	bool overrideCheck;

	RenderComponent3();
	RenderComponent3(const Mesh& mesh, const std::shared_ptr<Material3>& material, bool overrideCheck = false);
	RenderComponent3(const RenderComponent3& other);

	void OnEnable() override;
	void OnDisable() override;

	inline const Mesh& GetMesh() const
	{
		return mesh;
	}

	void SetMesh(const Mesh& mesh);

	inline const Material3* GetMaterial() const
	{
		return &*material;
	}

	void SetMaterial(const std::shared_ptr<Material3>& material);
	void SetMeshAndMaterial(const Mesh& mesh, const std::shared_ptr<Material3>& material);

	RenderComponent3& operator=(const RenderComponent3& other);

	can_copy(RenderComponent3);
};

class RenderingOpaqueProcess3 : public ObjectRenderingProcess3
{
public:
	RenderingOpaqueProcess3();

	virtual void Run(const RMatrix4x4& projectionView, const Camera3* camera) = 0;
};

class RenderingInfluencedProcess : public ObjectRenderingProcess3
{
public:
	GLint requestNum;

	RenderingInfluencedProcess(bool usesRefractions, bool usesReflections);

	virtual void RunDataPass(const RMatrix4x4& projectionView, const Camera3* camera) = 0;
	virtual void Run(const RMatrix4x4& projectionView, const Camera3* camera, const Texture2D& refractionTexture, const Texture2D& reflectionTexture) = 0;

	/*void AddObject(const TRenderComponent3<T>& object) override
	{
		objects[object.GetMesh()][object.GetMaterial()].push_back(&object.gameObject->transform);
	}

	void RemoveObject(const TRenderComponent3<T>& object) override
	{
		objects[object.GetMesh()][object.GetMaterial()].remove(&object.gameObject->transform);
	}
protected:
	std::unordered_map<const Mesh*, std::unordered_map<const TMaterial3<T>*, std::list<T*>>> objects;*/
};

class RenderingPostProcess3
{
public:
	virtual ~RenderingPostProcess3();

	virtual void Run() = 0;
};

// TODO: Move out of header file
class SkyboxRenderingProcess3 : public RenderingOpaqueProcess3
{
private:
	const Shader shader;
public:
	Cubemap cubemap;

	SkyboxRenderingProcess3(const Cubemap& cubemap);

	void Run(const RMatrix4x4& projectionView, const Camera3* camera) override;

	void AddObject(const RenderComponent3& object) override { }
	void RemoveObject(const RenderComponent3& object) override { }
};

namespace Internal
{
	namespace Rendering
	{
		// TODO: Remove vector thing. Only needs to be a single process
		void RegisterProcess(const std::string& name, const std::vector<ObjectRenderingProcess3*>& process);
		void RegisterProcess(const std::string& name, RenderingPostProcess3* process);

		void Init(int width, int height);
		void Clear();
	}
}

namespace Scene
{
	void Render(const Camera3* camera, const Cubemap& skybox);
	void Render(const Camera3* camera, const FBO& finalFbo);

	void ConductProbe(const Transform3& location, const Cubemap& environmentMap);
	void ComputeIrradiance(const Cubemap& environmentMap, const Cubemap& irradianceMap);
	void ComputePrefilteredEnvironmentMap(const Cubemap& environmentMap, const Cubemap& prefilteredEnvironmentMap);
	
	namespace Debug
	{
		//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		//glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
}