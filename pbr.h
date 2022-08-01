#pragma once

#include "general_common.h"
#include "rendering.h"

// This system will not provide accurate results for scaled objects.
class PBRMaterial3 : public Material3
{
private:
	Texture2D albedo;
	Texture2D normals;
	Texture2D materialData;
	bool pooledInstanced;
public:
	PBRMaterial3(const Texture2D& albedo, const Texture2D& normals, const Texture2D& materialData, bool pooledInstanced = false);

	inline void LoadMaterial() const
	{
		albedo.Bind(3);
		normals.Bind(4);
		materialData.Bind(5);
	}

	no_transfer_functions(PBRMaterial3);
};

namespace std
{
	template <>
	struct hash<std::pair<Mesh, const PBRMaterial3*>>
	{
		std::size_t operator()(const std::pair<Mesh, const PBRMaterial3*>& k) const
		{
			return hash<Mesh>()(k.first) ^ hash<uintptr_t>()(reinterpret_cast<uintptr_t>(k.second));
		}
	};
}

class ReflectionProbeComponent3;

namespace Internal
{
	namespace Storage
	{
		struct PBR
		{
			static std::list<ReflectionProbeComponent3*> activeProbes;
		};
	}
}

class ReflectionProbeComponent3 : public Component3
{
public:
	// TODO: Can environment and prefiltered environment can both be the same cubemap?
	Cubemap environmentMap;
	Cubemap irradianceMap;
	Cubemap prefilteredEnvironmentMap;
public:
	ReflectionProbeComponent3();
	ReflectionProbeComponent3(const ReflectionProbeComponent3& other);

	void OnEnable() override;

	void OnDisable() override;

	void Refresh();

	can_copy(ReflectionProbeComponent3)
};

class PBROpaqueProcess3 : public RenderingOpaqueProcess3
{
private:
	std::unordered_map<std::pair<Mesh, const PBRMaterial3*>, ObjectCollection> objects;

	const Shader shader;
public:
	PBROpaqueProcess3();

	void Run(const RMatrix4x4& projectionView, const Camera3* camera) override;

	void AddObject(const RenderComponent3& object) override;
	void RemoveObject(const RenderComponent3& object) override;
};

namespace Internal
{
	namespace PBR
	{
		void Init();
	}
}