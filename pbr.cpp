#include "pbr.h"

std::list<ReflectionProbeComponent3*> Internal::Storage::PBR::activeProbes;

PBRMaterial3::PBRMaterial3(const Texture2D& albedo, const Texture2D& normals, const Texture2D& materialData, bool pooledInstanced) :
	Material3("BasePBR"),
	albedo(albedo),
	normals(normals),
	materialData(materialData),
	pooledInstanced(pooledInstanced)
{ }

ReflectionProbeComponent3::ReflectionProbeComponent3() :
	environmentMap(Cubemap::Descriptor(Internal::Storage::Rendering::ENVIRONMENT_MAP_RESOLUTION, Internal::Storage::Rendering::ENVIRONMENT_MAP_RESOLUTION, GL_RGB16F, true, false)),
	irradianceMap(Cubemap::Descriptor(Internal::Storage::Rendering::IRRADIANCE_MAP_RESOLUTION, Internal::Storage::Rendering::IRRADIANCE_MAP_RESOLUTION, GL_RGB16F, true, false)),
	prefilteredEnvironmentMap(Cubemap::Descriptor(Internal::Storage::Rendering::PREFILTERED_ENVIRONMENT_MAP_RESOLUTION, Internal::Storage::Rendering::PREFILTERED_ENVIRONMENT_MAP_RESOLUTION, GL_RGB16F, true, true))
{ }

ReflectionProbeComponent3::ReflectionProbeComponent3(const ReflectionProbeComponent3& other) :
	environmentMap(other.environmentMap),
	irradianceMap(other.irradianceMap)
{ }

void ReflectionProbeComponent3::OnEnable()
{
	Internal::Storage::PBR::activeProbes.push_back(this);
}

void ReflectionProbeComponent3::OnDisable()
{
	Internal::Storage::PBR::activeProbes.remove(this);
}

void ReflectionProbeComponent3::Refresh()
{
	Scene::ConductProbe(gameObject->transform, environmentMap);
	Scene::ComputeIrradiance(environmentMap, irradianceMap);
	Scene::ComputePrefilteredEnvironmentMap(environmentMap, prefilteredEnvironmentMap);
}

PBROpaqueProcess3::PBROpaqueProcess3() :
	shader("Internal/Shaders/pbr")
{ }

void PBROpaqueProcess3::Run(const RMatrix4x4& projectionView, const Camera3* camera)
{
	shader.Bind();
	if (Internal::Storage::RenderingObj::directionalLight != nullptr)
	{
		Shader::LoadVector3(0, Internal::Storage::RenderingObj::directionalLight->gameObject->transform.GetWorldForward());
		Shader::LoadVector3(1, &Internal::Storage::RenderingObj::directionalLight->color.r);
	}
	else
	{
		Shader::LoadVector3(0, FVector3(0.0F, -1.0F, 0.0F));
		Shader::LoadVector3(1, FVector3(0.0F, 0.0F, 0.0F));
	}

	Internal::Storage::Rendering::brdfLUT.Bind(0);
	if (!Internal::Storage::PBR::activeProbes.empty())
	{
		Internal::Storage::PBR::activeProbes.front()->prefilteredEnvironmentMap.Bind(1);
		Internal::Storage::PBR::activeProbes.front()->irradianceMap.Bind(2);
	}
	else
	{
		Internal::Storage::Rendering::defaultCubemap.Bind(1);
		Internal::Storage::Rendering::defaultCubemap.Bind(2);
	}

	struct AttributeData
	{
		alignas(16) FMatrix4x4 p_v_t;
		alignas(16) FVector3 rotation0;
		alignas(16) FVector3 rotation1;
		alignas(16) FVector3 rotation2;
		alignas(16) FVector3 relative_camera_pos;
	};
	static std::vector<AttributeData> data;

	RVector3 camPos = camera->gameObject->transform.GetWorldPosition();
	for (auto& hashPair : objects)
	{
		hashPair.first.first.Bind();
		hashPair.first.second->LoadMaterial();
		// TODO: SSBO pooling if applicable (use dummy collection)!
		// TODO: If not pooling, have a static option to not reload unless adding a new object!
		//std::cerr << hashPair.second.objects.size() << " objects in group!" << std::endl;

		data.resize(hashPair.second.objects.size());
		for (int i = 0; i < hashPair.second.objects.size(); ++i)
		{
			data[i].p_v_t = static_cast<FMatrix4x4>(projectionView * hashPair.second.objects[i]->GetMatrix());
			FMatrix3x3 rotation = hashPair.second.objects[i]->GetWorldRotationMatrix3x3();
			data[i].rotation0 = rotation[0];
			data[i].rotation1 = rotation[1];
			data[i].rotation2 = rotation[2];
			data[i].relative_camera_pos = static_cast<FVector3>(camPos - hashPair.second.objects[i]->GetWorldPosition());
			/*Shader::LoadMatrix4x4(0, static_cast<FMatrix4x4>(projectionView * transform->GetMatrix()));
			Shader::LoadMatrix3x3(1, transform->GetWorldRotationMatrix3x3());
			Shader::LoadVector3(2, transform->GetWorldScale());
			Shader::LoadVector3(3, static_cast<FVector3>(camPos - transform->GetWorldPosition()));
			hashPair.first.first.DrawElements();*/
		}
		hashPair.second.RefreshSSBO(&data[0], hashPair.second.objects.size() * sizeof(AttributeData));
		hashPair.second.Bind();
		glDrawElementsInstanced(GL_TRIANGLES, hashPair.first.first.GetDrawCount(), GL_UNSIGNED_SHORT, 0, hashPair.second.objects.size());
	}
}

void PBROpaqueProcess3::AddObject(const RenderComponent3& object)
{
	objects[std::pair<Mesh, const PBRMaterial3*>(object.GetMesh(), reinterpret_cast<const PBRMaterial3*>(object.GetMaterial()))].objects.push_back(&object.gameObject->transform);
}

void PBROpaqueProcess3::RemoveObject(const RenderComponent3& object)
{
	/*
	// DEBUGGING:
	int instances = 0;
	for (auto* p : objects[object.GetMesh()][object.GetMaterial()])
	{
	if (p == &object.gameObject->transform)
	instances++;
	}
	if (instances != 1)
	std::cerr << "INST ERR " << instances << std::endl;*/

	auto hashPair = objects.find(std::pair<Mesh, const PBRMaterial3*>(object.GetMesh(), reinterpret_cast<const PBRMaterial3*>(object.GetMaterial())));
	RemoveVectorComponent(hashPair->second.objects, &object.gameObject->transform);
	if (hashPair->second.objects.empty())
		objects.erase(hashPair);
}

namespace Internal
{
	namespace PBR
	{
		void Init()
		{
			std::vector<ObjectRenderingProcess3*> basePBRProcess;
			basePBRProcess.push_back(new PBROpaqueProcess3());
			Rendering::RegisterProcess("BasePBR", basePBRProcess);
		}
	}
}