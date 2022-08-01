#include "physics.h"

/*const float Internal::Storage::Physics::STEP_FREQUENCY = 1.0F / 30.0F;
btDefaultCollisionConfiguration* Internal::Storage::Physics::collisionConfiguration = nullptr;
btCollisionDispatcher* Internal::Storage::Physics::dispatcher = nullptr;
btBroadphaseInterface* Internal::Storage::Physics::overlappingPairCache = nullptr;
btGhostPairCallback* Internal::Storage::Physics::ghostCallback = nullptr;
btSequentialImpulseConstraintSolver* Internal::Storage::Physics::solver = nullptr;
btDiscreteDynamicsWorld* Internal::Storage::Physics::dynamicsWorld = nullptr;
RequestList<TempColObjContainer*> Internal::Storage::Physics::movableObjects;
float Internal::Storage::Physics::stepProgress = 0.0F;

namespace Internal
{
	namespace Physics
	{
		void Init()
		{
			Storage::Physics::collisionConfiguration = new btDefaultCollisionConfiguration();
			Storage::Physics::dispatcher = new btCollisionDispatcher(Storage::Physics::collisionConfiguration);
			Storage::Physics::overlappingPairCache = new btDbvtBroadphase();
			Storage::Physics::ghostCallback = new btGhostPairCallback();
			Storage::Physics::overlappingPairCache->getOverlappingPairCache()->setInternalGhostPairCallback(Storage::Physics::ghostCallback);
			Storage::Physics::solver = new btSequentialImpulseConstraintSolver();
			Storage::Physics::dynamicsWorld = new btDiscreteDynamicsWorld(Storage::Physics::dispatcher, Storage::Physics::overlappingPairCache, Storage::Physics::solver, Storage::Physics::collisionConfiguration);
			Storage::Physics::dynamicsWorld->setGravity(btVector3(0, -9.8, 0));
		}

		void Update(const GameTime& time)
		{
			Internal::Storage::Physics::stepProgress += time.deltaTime;
			while (Internal::Storage::Physics::stepProgress >= Internal::Storage::Physics::STEP_FREQUENCY)
			{
				Storage::Physics::dynamicsWorld->stepSimulation(Internal::Storage::Physics::STEP_FREQUENCY, 0, 0.0);
				Internal::Storage::Physics::stepProgress -= Internal::Storage::Physics::STEP_FREQUENCY;
			}

			Internal::Storage::Physics::movableObjects.ProcessRequests();
			for (TempColObjContainer* obj : Internal::Storage::Physics::movableObjects)
			{
				btTransform transform = obj->obj->getWorldTransform();
				btVector3 pos = transform.getOrigin();
				obj->gameObject->transform.SetWorldPosition(RVector3(pos.x(), pos.y(), pos.z()));
				btQuaternion rot = transform.getRotation();
				obj->gameObject->transform.SetWorldRotation(FQuaternion(static_cast<float>(rot.w()), static_cast<float>(rot.x()), static_cast<float>(rot.y()), static_cast<float>(rot.z())));
			}
		}

		void Debug()
		{
			for (int j = Storage::Physics::dynamicsWorld->getNumCollisionObjects() - 1; j >= 0; --j)
			{
				btCollisionObject* obj = Storage::Physics::dynamicsWorld->getCollisionObjectArray()[j];
				btRigidBody* body = btRigidBody::upcast(obj);
				btTransform trans;
				if (body && body->getMotionState())
					body->getMotionState()->getWorldTransform(trans);
				else
					trans = obj->getWorldTransform();
				printf("world pos object %d = %f,%f,%f\n", j, float(trans.getOrigin().getX()), float(trans.getOrigin().getY()), float(trans.getOrigin().getZ()));
			}
		}

		void Clear()
		{
			delete Storage::Physics::dynamicsWorld;
			delete Storage::Physics::solver;
			delete Storage::Physics::overlappingPairCache;
			delete Storage::Physics::ghostCallback;
			delete Storage::Physics::dispatcher;
			delete Storage::Physics::collisionConfiguration;

			Internal::Storage::Physics::dynamicsWorld = nullptr;
			Internal::Storage::Physics::solver = nullptr;
			Internal::Storage::Physics::overlappingPairCache = nullptr;
			Internal::Storage::Physics::ghostCallback = nullptr;
			Internal::Storage::Physics::dispatcher = nullptr;
			Internal::Storage::Physics::collisionConfiguration = nullptr;

			Storage::Physics::movableObjects.Clear();
		}
	}
}

namespace Physics
{
	void Raycast(const RVector3& startPosition, const RVector3& direction, real min, real max, RaycastResult3& result)
	{
		btVector3 start(startPosition.x + direction.x * min, startPosition.y + direction.y * min, startPosition.z + direction.z * min);
		btVector3 end(startPosition.x + direction.x * max, startPosition.y + direction.y * max, startPosition.z + direction.z * max);
		SpecialClosestRayResultCallback callback(start, end);
		//callback.m_collisionFilterMask = 64; (was for terrain filtering)
		Internal::Storage::Physics::dynamicsWorld->rayTest(start, end, callback);
		if (callback.hasHit())
		{
			result.hasHit = true;
			result.position = RVector3(callback.m_hitPointWorld.x(), callback.m_hitPointWorld.y(), callback.m_hitPointWorld.z());
			result.normal = RVector3(callback.m_hitNormalWorld.x(), callback.m_hitNormalWorld.y(), callback.m_hitNormalWorld.z());
			result.childShapeIndex = callback.m_childShapeIndex;
			if (callback.m_collisionObject != nullptr && callback.m_collisionObject->getUserPointer() != nullptr)
				result.gameObject = reinterpret_cast<GameObject3*>(callback.m_collisionObject->getUserPointer());
			else
				result.gameObject = nullptr;
		}
		else
			result.Reset();
	}
}

std::vector<GameObject3*> Collider3::GetColliding()
{
	std::vector<GameObject3*> colliding;

	int numManifolds = Internal::Storage::Physics::dispatcher->getNumManifolds();
	for (int i = 0; i < numManifolds; ++i)
	{
		btPersistentManifold* contactManifold = Internal::Storage::Physics::dispatcher->getManifoldByIndexInternal(i);
		const btCollisionObject* obA = reinterpret_cast<const btCollisionObject*>(contactManifold->getBody0());
		const btCollisionObject* obB = reinterpret_cast<const btCollisionObject*>(contactManifold->getBody1());

		if (obA->getUserPointer() == gameObject || obB->getUserPointer() == gameObject)
		{
			int numContacts = contactManifold->getNumContacts();
			for (int j = 0; j < numContacts; ++j)
			{
				btManifoldPoint& pt = contactManifold->getContactPoint(j);
				if (pt.getDistance() < 0.0f)
				{
					if (obA->getUserPointer() == gameObject)
						colliding.push_back(reinterpret_cast<GameObject3*>(obB->getUserPointer()));
					else
						colliding.push_back(reinterpret_cast<GameObject3*>(obA->getUserPointer()));
					break;
				}
			}
		}
	}

	return colliding;
}

void Collider3::OnCreate()
{
	body->setUserPointer(this->gameObject);
}

void Collider3::OnEnable()
{
	btTransform transform;
	transform.setIdentity();

	auto pos = this->gameObject->transform.GetWorldPosition();
	auto rot = this->gameObject->transform.GetWorldRotation();
	transform.setOrigin(btVector3(pos.x, pos.y, pos.z));
	transform.setRotation(btQuaternion(rot.x, rot.y, rot.z, rot.w));

	body->setWorldTransform(transform);

	if (!body->isStaticObject())
		Internal::Storage::Physics::movableObjects.Add(this);

	Internal::Storage::Physics::dynamicsWorld->addRigidBody(body);
}

void Collider3::OnDisable()
{
	if (Internal::Storage::Physics::dynamicsWorld != nullptr)
	{
		if (!body->isStaticObject())
			Internal::Storage::Physics::movableObjects.Remove(this);
		Internal::Storage::Physics::dynamicsWorld->removeRigidBody(body);
	}
}

Collider3::~Collider3()
{
	if (body->getMotionState() != nullptr)
		delete body->getMotionState();
	delete shape;
}

Collider3::Collider3(btCollisionShape* shape, real mass) :
	shape(shape),
	mass(mass)
{
	btVector3 localInertia(REAL_ZERO, REAL_ZERO, REAL_ZERO);
	if (mass != REAL_ZERO)
	{
		shape->calculateLocalInertia(mass, localInertia);
		body = new btRigidBody(btRigidBody::btRigidBodyConstructionInfo(mass, new btDefaultMotionState(), shape, localInertia));
	}
	else
	{
		body = new btRigidBody(btRigidBody::btRigidBodyConstructionInfo(mass, nullptr, shape, localInertia));
	}

	this->obj = body;
}

Collider3::Collider3(btCollisionShape* shape, bool isStatic) :
	shape(shape),
	mass(REAL_ZERO)
{
	if (isStatic)
		body = new btRigidBody(btRigidBody::btRigidBodyConstructionInfo(mass, nullptr, shape, btVector3(REAL_ZERO, REAL_ZERO, REAL_ZERO)));
	else
		body = new btRigidBody(btRigidBody::btRigidBodyConstructionInfo(mass, new btDefaultMotionState(), shape, btVector3(REAL_ZERO, REAL_ZERO, REAL_ZERO)));

	this->obj = body;
}

void Collider3::MakeStatic()
{
	if (!body->isStaticObject())
	{
		if (gameObject->IsEnabled())
			OnDisable();

		if (body->getMotionState() != nullptr)
			delete body->getMotionState();
		delete body;

		body = new btRigidBody(btRigidBody::btRigidBodyConstructionInfo(mass, nullptr, shape, btVector3(REAL_ZERO, REAL_ZERO, REAL_ZERO)));

		if (gameObject->IsEnabled())
			OnEnable();
	}
}

void Collider3::MakeDynamic()
{
	if (body->isStaticObject())
	{
		if (gameObject->IsEnabled())
			OnDisable();

		if (body->getMotionState() != nullptr)
			delete body->getMotionState();
		delete body;

		body = new btRigidBody(btRigidBody::btRigidBodyConstructionInfo(mass, new btDefaultMotionState(), shape, btVector3(REAL_ZERO, REAL_ZERO, REAL_ZERO)));

		if (gameObject->IsEnabled())
			OnEnable();
	}
}

CompoundCollider3::CompoundCollider3(bool isStatic) :
	Collider3(compoundShape = new btCompoundShape(false), isStatic),
	isStatic(isStatic)
{ }

RVector3 CompoundCollider3::RefreshShape()
{
	btVector3 inertia;
	btTransform principal;
	principal.setIdentity();
	compoundShape->calculatePrincipalAxisTransform(&masses[0], principal, inertia);
	body->setMassProps(mass, inertia);
	body->updateInertiaTensor();
	body->clearForces();

	//btMatrix3x3 inverseWorldRotation = body->getWorldTransform().getBasis().inverse();
	//for (int i = 0; i < compoundShape->getNumChildShapes(); ++i)
	//	compoundShape->getChildTransform(i).setOrigin(inverseWorldRotation * (body->getWorldTransform().getBasis() * compoundShape->getChildTransform(i).getOrigin() - principal.getOrigin()));

	for (int i = 0; i < compoundShape->getNumChildShapes(); ++i)
		compoundShape->getChildTransform(i).setOrigin(compoundShape->getChildTransform(i).getOrigin() - principal.getOrigin());
	compoundShape->recalculateLocalAabb();

	btTransform newTransform(body->getWorldTransform().getBasis(), body->getWorldTransform().getOrigin() + body->getWorldTransform().getBasis() * principal.getOrigin());
	body->setWorldTransform(newTransform);
	body->getMotionState()->setWorldTransform(newTransform);

	// TODO: Remove
	if (gameObject->IsEnabled())
	{
		Internal::Storage::Physics::dynamicsWorld->removeRigidBody(body);
		Internal::Storage::Physics::dynamicsWorld->addRigidBody(body);
	}

	return RVector3(principal.getOrigin().x(), principal.getOrigin().y(), principal.getOrigin().z());
}

RVector3 CompoundCollider3::AddShape(const DVector3& localPosition, const DQuaternion& localRotation, btCollisionShape* shape, double m_kg)
{
	btTransform transform;
	transform.setIdentity();
	transform.setOrigin(btVector3(localPosition.x, localPosition.y, localPosition.z));
	transform.setRotation(btQuaternion(localRotation.x, localRotation.y, localRotation.z, localRotation.w));
	compoundShape->addChildShape(transform, shape);
	mass += m_kg;
	masses.push_back(m_kg);

	if (!isStatic)
		return RefreshShape();
	else
		return RVector3(REAL_ZERO);
	// TODO: Recalc moment of inertia
}

void CompoundCollider3::SetStatic(bool isStatic)
{
	if (isStatic != this->isStatic)
	{
		if (isStatic)
			MakeStatic();
		else
		{
			MakeDynamic();
			RefreshShape();
		}

		this->isStatic = isStatic;
	}
}*/
