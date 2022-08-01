#pragma once

#include "general_common.h"

/*#ifdef TUNDRA_USE_DOUBLE_PRECISION
	#define BT_USE_DOUBLE_PRECISION
#endif
#include <Bullet\btBulletDynamicsCommon.h>
#include <Bullet\BulletCollision\CollisionShapes\btHeightfieldTerrainShape.h>
#include <Bullet\BulletCollision\CollisionDispatch\btGhostObject.h>

#include "mesh.h"
#include "game_object.h"
#include "game_time.h"

class TempColObjContainer;

namespace Internal
{
	namespace Storage
	{
		struct Physics
		{
			static const float STEP_FREQUENCY;

			static btDefaultCollisionConfiguration* collisionConfiguration;
			static btCollisionDispatcher* dispatcher;
			static btBroadphaseInterface* overlappingPairCache;
			static btGhostPairCallback* ghostCallback;
			static btSequentialImpulseConstraintSolver* solver;
			static btDiscreteDynamicsWorld* dynamicsWorld;

			static float stepProgress;

			static RequestList<TempColObjContainer*> movableObjects;
		};
	}

	namespace Physics
	{
		void Init();
		void Update(const GameTime& time);
		void Debug();
		void Clear();
	}
}

namespace Physics
{
	struct SpecialClosestRayResultCallback : public btCollisionWorld::ClosestRayResultCallback
	{
		int m_childShapeIndex;

		SpecialClosestRayResultCallback(const btVector3& rayFromWorld, const btVector3& rayToWorld) :
			ClosestRayResultCallback(rayFromWorld, rayToWorld),
			m_childShapeIndex(0)
		{ }

		virtual btScalar addSingleResult(btCollisionWorld::LocalRayResult& rayResult, bool normalInWorldSpace)
		{
			if (rayResult.m_localShapeInfo)
				m_childShapeIndex = rayResult.m_localShapeInfo->m_triangleIndex;
			else
				m_childShapeIndex = 0;

			return ClosestRayResultCallback::addSingleResult(rayResult, normalInWorldSpace);
		}
	};

	struct RaycastResult3
	{
		bool hasHit;
		RVector3 position;
		RVector3 normal;
		GameObject3* gameObject;
		int childShapeIndex;

		RaycastResult3()
		{
			Reset();
		}

		void Reset()
		{
			hasHit = false;
			position = RVector3(NAN, NAN, NAN);
			normal = RVector3(NAN, NAN, NAN);
			gameObject = nullptr;
		}
	};

	void Raycast(const RVector3& startPosition, const RVector3& direction, real min, real max, RaycastResult3& result);
}

class TempColObjContainer : public Component3
{
public:
	btCollisionObject* obj;

	TempColObjContainer() :
		obj(nullptr)
	{ }

	virtual ~TempColObjContainer()
	{
		delete obj;
	}

	cant_copy(TempColObjContainer)
};

class Collider3 : public TempColObjContainer
{
protected:
	btCollisionShape* shape;
	real mass;
public:
	btRigidBody* body;

	std::vector<GameObject3*> GetColliding();

	void RefreshStatic()
	{
		OnDisable();
		OnEnable();
	}

	void OnCreate() override;

	void OnEnable() override;
	void OnDisable() override;

	virtual ~Collider3();
protected:
	Collider3(btCollisionShape* shape, real mass);
	Collider3(btCollisionShape* shape, bool isStatic);

	void MakeStatic();
	void MakeDynamic();
};

class BoxCollider3 : public Collider3
{
public:
	BoxCollider3(const RVector3& boxSize, real mass) :
		Collider3(new btBoxShape(btVector3(boxSize.x, boxSize.y, boxSize.z)), mass)
	{ }

	// TODO: Is no longer valid, now that I don't support offsets
	BoxCollider3(const Mesh& mesh, real mass) :
		Collider3(new btBoxShape(btVector3((static_cast<real>(mesh.AABBMax.x) - static_cast<real>(mesh.AABBMin.x)) * static_cast<real>(0.5), (static_cast<real>(mesh.AABBMax.y) - static_cast<real>(mesh.AABBMin.y)) * static_cast<real>(0.5), (static_cast<real>(mesh.AABBMax.z) - static_cast<real>(mesh.AABBMin.z)) * static_cast<real>(0.5))), mass)
	{ }
	
	BoxCollider3* rtc_GetCopy() const override
	{
		btTransform origin;
		origin.setIdentity();
		btVector3 AABBMin;
		btVector3 AABBMax;
		this->shape->getAabb(origin, AABBMin, AABBMax);
		
		return new BoxCollider3(DVector3(AABBMax.x(), AABBMax.y(), AABBMax.z()), this->mass);
	}
};

class TerrainCollider3 : public Collider3
{
public:
	TerrainCollider3(real* heights, int gridSize, real minHeight, real maxHeight, real gridSquareScale) :
		Collider3(new btHeightfieldTerrainShape(gridSize, gridSize, heights, REAL_ONE, minHeight, maxHeight, 1, PHY_FLOAT, false), REAL_ZERO)
	{
		(reinterpret_cast<btHeightfieldTerrainShape*>(this->shape))->setLocalScaling(btVector3(gridSquareScale, REAL_ONE, gridSquareScale));
	}

	void OnEnable() override
	{
		btTransform transform;
		transform.setIdentity();

		auto pos = this->gameObject->transform.GetWorldPosition();
		transform.setOrigin(btVector3(pos.x, REAL_ZERO, pos.z));

		this->body->setWorldTransform(transform);

		Internal::Storage::Physics::dynamicsWorld->addRigidBody(this->body);
		//Internal::Storage::Physics::dynamicsWorld->addRigidBody(this->body, 64, btBroadphaseProxy::AllFilter);
	}

	cant_copy(TerrainCollider3);
};

class MeshCollider3 : public Collider3
{
public:
	std::vector<DVector3> verticies;
	std::vector<int> indicies;
	btTriangleIndexVertexArray* meshInterface;
	btVector3 position;

	// TODO: May have problems across triangles without this: https://pybullet.org/Bullet/phpBB3/viewtopic.php?f=9&t=4603
	MeshCollider3(std::vector<DVector3>&& inVerticies, std::vector<int>&& inIndicies, const DVector3& position) :
		Collider3(new btBvhTriangleMeshShape(meshInterface = new btTriangleIndexVertexArray(static_cast<int>(inIndicies.size() / 3), &inIndicies[0], static_cast<int>(3 * sizeof(int)), static_cast<int>(inVerticies.size()), &inVerticies[0].x, static_cast<int>(sizeof(DVector3))), true, true), REAL_ZERO),
		position(position.x, position.y, position.z)
	{
		verticies = std::move(inVerticies);
		indicies = std::move(inIndicies);
	}

	~MeshCollider3()
	{
		delete meshInterface;
	}

	void OnEnable() override
	{
		btTransform transform;
		transform.setIdentity();
		transform.setOrigin(position);

		this->body->setWorldTransform(transform);

		Internal::Storage::Physics::dynamicsWorld->addRigidBody(this->body);
		//Internal::Storage::Physics::dynamicsWorld->addRigidBody(this->body, 64, btBroadphaseProxy::AllFilter);
	}

	cant_copy(MeshCollider3);
};

class CompoundCollider3 : public Collider3
{
private:
	btCompoundShape* compoundShape;
	bool isStatic;
	std::vector<double> masses;

	RVector3 RefreshShape();
public:
	CompoundCollider3(bool isStatic);

	// Will NOT dellocate shapes. User is responsible. m_kg is ignored if isStatic == true. Returns the needed offset for the structure to align with the physics system
	RVector3 AddShape(const DVector3& localPosition, const DQuaternion& localRotation, btCollisionShape* shape, double m_kg);

	void SetStatic(bool isStatic);
	// TODO: Add remove func that will not attempt to calc principal when mass == 0

	//void AddBox(const DVector3& position, const DVector3& AABBSize, const DVector3& AABBOffset)
	//{
	//	AddShape(position, AABBOffset, new btBoxShape(btVector3(AABBSize.x, AABBSize.y, AABBSize.z)));
	//}

	//void ClearShapes()
	//{
	//	for (int i = 0; i < compoundShape->getNumChildShapes(); ++i)
	//		compoundShape->removeChildShapeByIndex(i);
	//}

	cant_copy(CompoundCollider3)
};

class GhostBoxCollider3 : public TempColObjContainer
{
private:
	btCollisionShape * shape;
	btGhostObject* ghost;
public:
	GhostBoxCollider3(const RVector3& boxSize)
	{
		shape = new btBoxShape(btVector3(boxSize.x, boxSize.y, boxSize.z));
		ghost = new btGhostObject();
		ghost->setCollisionShape(shape);
		this->obj = ghost;
	}

	GhostBoxCollider3(const Mesh& mesh)
	{
		shape = new btBoxShape(btVector3((static_cast<real>(mesh.AABBMax.x) - static_cast<real>(mesh.AABBMin.x)) * static_cast<real>(0.5), (static_cast<real>(mesh.AABBMax.y) - static_cast<real>(mesh.AABBMin.y)) * static_cast<real>(0.5), (static_cast<real>(mesh.AABBMax.z) - static_cast<real>(mesh.AABBMin.z)) * static_cast<real>(0.5)));
		ghost = new btGhostObject();
		ghost->setCollisionShape(shape);
		this->obj = ghost;
	}

	virtual ~GhostBoxCollider3()
	{
		delete shape;
	}

	void OnCreate() override
	{
		ghost->setUserPointer(this->gameObject);
	}

	void OnEnable() override
	{
		btTransform transform;
		transform.setIdentity();

		FVector3 pos = this->gameObject->transform.GetWorldPosition();
		transform.setOrigin(btVector3(pos.x, pos.y, pos.z));

		ghost->setWorldTransform(transform);
		Internal::Storage::Physics::dynamicsWorld->addCollisionObject(ghost);
	}

	void OnDisable() override
	{
		if (Internal::Storage::Physics::dynamicsWorld != NULL)
			Internal::Storage::Physics::dynamicsWorld->removeCollisionObject(ghost);
	}

	void UpdatePosition()
	{
		btTransform transform;
		transform.setIdentity();

		RVector3 pos = this->gameObject->transform.GetWorldPosition();
		transform.setOrigin(btVector3(pos.x, pos.y, pos.z));

		ghost->setWorldTransform(transform);
	}

	GhostBoxCollider3* rtc_GetCopy() const override
	{
		btTransform origin;
		origin.setIdentity();
		btVector3 aabbMin;
		btVector3 aabbMax;
		this->shape->getAabb(origin, aabbMin, aabbMax);

		return new GhostBoxCollider3(RVector3(aabbMax.x(), aabbMax.y(), aabbMax.z()));
	}
};*/
