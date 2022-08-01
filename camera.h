#pragma once

#include "general_common.h"
#include "game_object.h"
#include "updateable.h"
#include <iostream>

class Camera3 : public Component3
{
protected:
	real fov;
	real aspect;
	real zNear, zFar;

	RMatrix4x4 projectionMatrix;
	FMatrix4x4 viewRotationMatrix;

	enum Planes
	{
		LEFT, RIGHT, BOTTOM, TOP, NEAR, FAR,

		COUNT
	};

	struct Plane
	{
		RVector3 normal;
		real d;

		void SetCoefficients(real a, real b, real c, real d);

		inline real GetDistance(const RVector3& pos) const
		{
			// d represents the position of the plane (I think)

			return d + glm::dot(normal, pos);
		}
	};

	Plane planes[Planes::COUNT];
public:
	Camera3(real fov, real aspect, real zNear, real zFar);
	Camera3(const Camera3& other);

	void OnCreate() override;

	void Adjust(real fov, real aspect, real zNear, real zFar);
	void Adjust(real zNear, real zFar);

	void RefreshFrustum();

	// Returns true if the right, left, up, and down frustum planes contain the sphere
	// Objects far behind will still not be rendered as the test turns false after the side frustums intersect
	inline bool FunnelFrustumContainsSphere(const RVector3& pos, real radius) const
	{
		for (int plane = 0; plane < Planes::NEAR; ++plane)
			if (planes[plane].GetDistance(pos) <= -radius)
				return false;

		return true;
	}

	// Returns true if the right, left, up, down, near, and far frustum planes contain the sphere
	inline bool FullFrustumContainsSphere(const RVector3& pos, real radius) const
	{
		for (int plane = 0; plane < Planes::COUNT; ++plane)
			if (planes[plane].GetDistance(pos) <= -radius)
				return false;

		return true;
	}

	inline RMatrix4x4 GetProjectionViewMatrix() const
	{
		return projectionMatrix * (static_cast<RMatrix4x4>(viewRotationMatrix) * CreateMatrix::Translation(-gameObject->transform.GetWorldPosition()));
	}

	inline RMatrix4x4 GetViewMatrix() const
	{
		return (static_cast<RMatrix4x4>(viewRotationMatrix)* CreateMatrix::Translation(-gameObject->transform.GetWorldPosition()));
	}

	/*inline FMatrix4x4 GetVPMatrix() const
	{
		return glm::inverse(projectionMatrix) * (viewRotationMatrix * CreateMatrix::Translation(-transform.CalculateWorldPosition()));
	}*/

	inline void SetViewRotationMatrix(const FMatrix4x4& viewRotationMatrix)
	{
		this->viewRotationMatrix = viewRotationMatrix;
	}

	inline const FMatrix4x4& GetViewRotationMatrix() const
	{
		return viewRotationMatrix;
	}

	inline const RMatrix4x4& GetProjectionMatrix() const
	{
		return projectionMatrix;
	}

	inline real GetZNear() const
	{
		return zNear;
	}

	inline real GetZFar() const
	{
		return zFar;
	}

	inline real GetFOV() const
	{
		return fov;
	}

	inline real GetAspectRatio() const
	{
		return aspect;
	}

	can_copy(Camera3)
};