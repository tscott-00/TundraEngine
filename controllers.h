#pragma once

/*#include "general_common.h"
#include "physics.h"
#include "events_state.h"

#include <BulletDynamics\Character\btKinematicCharacterController.h>

template<class T>
class TLandController3 : public TTempColObjContainer<T>, private Updateable
{
protected:
	btKinematicCharacterController* controller;
	btConvexShape* shape;
	btPairCachingGhostObject* ghost;

	float playerHeight;
	float runSpeed;
	float strafeSpeed;
	float jumpPower;

	TCamera3<T>* camera;
public:
	float pitch;
	float yaw;
	float additionalPitch;
	float additionalYaw;
	float walkSpeed;

	TLandController3(float playerHeight, float walkSpeed, float runSpeed, float strafeSpeed, float jumpPower, TCamera3<T>* camera)
	{
		this->walkSpeed = walkSpeed * 1.0F / 30.0F;
		this->runSpeed = runSpeed * 1.0F / 30.0F;
		this->strafeSpeed = strafeSpeed * 1.0F / 30.0F;
		this->jumpPower = jumpPower;
		shape = new btCapsuleShape(1.0, playerHeight);
		ghost = new btPairCachingGhostObject();
		this->obj = ghost;
		ghost->setCollisionShape(shape);
		ghost->setCollisionFlags(btCollisionObject::CF_CHARACTER_OBJECT);
		controller = new btKinematicCharacterController(ghost, shape, 0.35, btVector3(0.0, 1.0, 0.0));

		this->camera = camera;

		pitch = 0.0F;
		yaw = 0.0F;
		additionalPitch = 0.0F;
		additionalYaw = 0.0F;
	}

	virtual ~TLandController3()
	{
		delete shape;
		delete controller;
	}

	void OnCreate() override
	{
		ghost->setUserPointer(this->gameObject);
	}

	void OnEnable() override
	{
		this->EnableUpdates();
		btTransform transform;
		transform.setIdentity();

		FVector3 pos = this->gameObject->transform.CalculateWorldPosition();
		transform.setOrigin(btVector3(pos.x, pos.y, pos.z));

		ghost->setWorldTransform(transform);
		Internal::Storage::Physics::dynamicsWorld->addCollisionObject(ghost, btBroadphaseProxy::CharacterFilter, btBroadphaseProxy::StaticFilter | btBroadphaseProxy::DefaultFilter | 64);
		Internal::Storage::Physics::dynamicsWorld->addAction(controller);
		Internal::Storage::TPhysics<T>::movableObjects.Add(this);
	}

	void OnDisable() override
	{
		this->DisableUpdates();
		if (Internal::Storage::Physics::dynamicsWorld != nullptr)
		{
			Internal::Storage::TPhysics<T>::movableObjects.Remove(this);
			Internal::Storage::Physics::dynamicsWorld->removeAction(controller);
			Internal::Storage::Physics::dynamicsWorld->removeCollisionObject(ghost);
		}
	}

	void Update(const GameTime& time) override
	{
		if (SDL_GetRelativeMouseMode() == SDL_FALSE)
			return;

		float dp = 2.0F * time.deltaTime * additionalPitch;
		float dy = 2.0F * time.deltaTime * additionalYaw;
		if (fabs(dp) > fabs(additionalPitch))
			dp = additionalPitch;
		if (fabs(dy) > fabs(additionalYaw))
			dy = additionalYaw;
		additionalPitch -= dp;
		additionalYaw -= dy;
		pitch += dp;
		yaw += dy;


		pitch -= 0.3F * EventsState::mouseDY;
		yaw -= 0.3F * EventsState::mouseDX;
		camera->transform.SetRotation(glm::quat(FVector3(pitch / 180.0F * PI, yaw / 180.0F * PI, 0.0F)));
		//FMatrix4x4 viewRotationMatrix = camera->transform.CalculateWorldRotationMatrix4x4();
		FMatrix4x4 viewRotationMatrix = CreateMatrix::Rotation(yaw / 180.0F * PI, FVector3(0.0F, 1.0F, 0.0F)) *  CreateMatrix::Rotation(pitch / 180.0F * PI, FVector3(1.0F, 0.0F, 0.0F));
		camera->SetViewRotationMatrix(CreateMatrix::LookAt(FVector3(0.0F, 0.0F, 0.0F), Normalize(FVector3(-viewRotationMatrix[2])), Normalize(FVector3(viewRotationMatrix[1]))));

		FVector2 forwardWalkVec = Normalize(FVector2(-viewRotationMatrix[2].x, -viewRotationMatrix[2].z));
		FVector2 sideWalkVec = Normalize(FVector2(viewRotationMatrix[0].x, viewRotationMatrix[0].z));

		float forwardWalkSpeed = walkSpeed;
		if (EventsState::keyStates[SDL_SCANCODE_LSHIFT])
			forwardWalkSpeed = runSpeed;
		FVector3 change(0.0F, 0.0F, 0.0F);
		if (EventsState::keyStates[SDL_SCANCODE_W])
		{
			change.x += forwardWalkVec.x * forwardWalkSpeed;
			change.z += forwardWalkVec.y * forwardWalkSpeed;
		}
		if (EventsState::keyStates[SDL_SCANCODE_S])
		{
			change.x -= forwardWalkVec.x * forwardWalkSpeed;
			change.z -= forwardWalkVec.y * forwardWalkSpeed;
		}

		if (EventsState::keyStates[SDL_SCANCODE_A])
		{
			change.x -= sideWalkVec.x * strafeSpeed;
			change.z -= sideWalkVec.y * strafeSpeed;
		}
		if (EventsState::keyStates[SDL_SCANCODE_D])
		{
			change.x += sideWalkVec.x * strafeSpeed;
			change.z += sideWalkVec.y * strafeSpeed;
		}

		if (controller->canJump() && EventsState::keyStates[SDL_SCANCODE_SPACE])
		{
			controller->jump(btVector3(0.0, jumpPower, 0.0));
			//change.y += jumpPower * time.deltaTime;
		}

		//this->gameObject->transform.ChangePosition(change);
		controller->setWalkDirection(btVector3(change.x, change.y, change.z));
	}

	TLandController3* rtc_GetCopy() const override
	{
		return new TLandController3(*this);
	}
protected:
	TLandController3(const TLandController3<T>& other) :
		TTempColObjContainer<T>(other)
	{
		camera = other.camera;

		pitch = other.pitch;
		yaw = other.yaw;
	}
};*/