#include "camera.h"

void Camera3::Plane::SetCoefficients(real a, real b, real c, real d)
{
	real l = glm::length(RVector3(a, b, c));
	normal = RVector3(a / l, b / l, c / l);
	this->d = d / l;
}

Camera3::Camera3(real fov, real aspect, real zNear, real zFar) :
	fov(fov),
	aspect(aspect),
	zNear(zNear),
	zFar(zFar),
	projectionMatrix(CreateMatrix::Projection(fov, aspect, zNear, zFar)),
	viewRotationMatrix(1.0F)
{ }

Camera3::Camera3(const Camera3& other) :
	fov(other.fov),
	aspect(other.aspect),
	zNear(zNear),
	zFar(zFar),
	projectionMatrix(other.projectionMatrix),
	viewRotationMatrix(other.viewRotationMatrix)
{ }

void Camera3::OnCreate()
{
	RefreshFrustum();
}

void Camera3::Adjust(real fov, real aspect, real zNear, real zFar)
{
	this->fov = fov;
	this->aspect = aspect;
	this->zNear = zNear;
	this->zFar = zFar;
	projectionMatrix = CreateMatrix::Projection(fov, aspect, zNear, zFar);
}

void Camera3::Adjust(real zNear, real zFar)
{
	this->zNear = zNear;
	this->zFar = zFar;
	projectionMatrix = CreateMatrix::Projection(fov, aspect, zNear, zFar);
}

void Camera3::RefreshFrustum()
{
	RMatrix4x4 PVMatrix = GetProjectionViewMatrix();

	planes[Planes::LEFT].SetCoefficients(
		PVMatrix[0][0] + PVMatrix[0][3],
		PVMatrix[1][0] + PVMatrix[1][3],
		PVMatrix[2][0] + PVMatrix[2][3],
		PVMatrix[3][0] + PVMatrix[3][3]
	);

	planes[Planes::RIGHT].SetCoefficients(
		-PVMatrix[0][0] + PVMatrix[0][3],
		-PVMatrix[1][0] + PVMatrix[1][3],
		-PVMatrix[2][0] + PVMatrix[2][3],
		-PVMatrix[3][0] + PVMatrix[3][3]
	);

	planes[Planes::BOTTOM].SetCoefficients(
		PVMatrix[0][1] + PVMatrix[0][3],
		PVMatrix[1][1] + PVMatrix[1][3],
		PVMatrix[2][1] + PVMatrix[2][3],
		PVMatrix[3][1] + PVMatrix[3][3]
	);

	planes[Planes::TOP].SetCoefficients(
		-PVMatrix[0][1] + PVMatrix[0][3],
		-PVMatrix[1][1] + PVMatrix[1][3],
		-PVMatrix[2][1] + PVMatrix[2][3],
		-PVMatrix[3][1] + PVMatrix[3][3]
	);

	planes[Planes::NEAR].SetCoefficients(
		PVMatrix[0][2] + PVMatrix[0][3],
		PVMatrix[1][2] + PVMatrix[1][3],
		PVMatrix[2][2] + PVMatrix[2][3],
		PVMatrix[3][2] + PVMatrix[3][3]
	);

	planes[Planes::FAR].SetCoefficients(
		-PVMatrix[0][2] + PVMatrix[0][3],
		-PVMatrix[1][2] + PVMatrix[1][3],
		-PVMatrix[2][2] + PVMatrix[2][3],
		-PVMatrix[3][2] + PVMatrix[3][3]
	);
}