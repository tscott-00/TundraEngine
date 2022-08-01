#pragma once

#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/ext.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>

#include "general_common.h"

// Inline some functions when replacing glm

template<class T>
using TVector2 = glm::vec<2, T>;
/*class TVector2 : public glm::vec<2, T>
{
public:
	TVector2() {}
	TVector2(const TVector2<T>& vec) : glm::vec<2, T>(vec) {}
	TVector2(T x, T y) : glm::vec<2, T>(x, y) {}
};*/

template<class T>
using TVector3 = glm::vec<3, T>;
/*class TVector3 : public glm::vec<3, T>
{
public:
	TVector3() {}
	TVector3(const TVector3<T>& vec) : glm::vec<3, T>(vec) {}
	TVector3(T x, T y, T z) : glm::vec<3, T>(x, y, z) {}
};*/

template<class T>
using TVector4 = glm::vec<4, T>;
/*class TVector4 : public glm::vec<4, T>
{
public:
	TVector4() {}
	TVector4(const TVector4<T>& vec) : glm::vec<4, T>(vec) {}
	TVector4(const TVector3<T>& vec, T w) : glm::vec<4, T>(vec, w) {}
	TVector4(T x, T y, T z, T w) : glm::vec<4, T>(x, y, z, w) {}
};*/

template<class T>
using TQuaternion = glm::tquat<T, glm::highp>;
/*class TQuaternion : public glm::tquat<T, glm::highp>
{
public:
	TQuaternion() {}
	TQuaternion(const TQuaternion<T>& vec) : glm::tquat<T, glm::highp>(vec) {}
	TQuaternion(T x, T y, T z, T w) : glm::tquat<T, glm::highp>(x, y, z, w) {}
};*/

template<class T>
using TMatrix2x2 = glm::mat<2, 2, T>;

template<class T>
using TMatrix3x3 = glm::mat<3, 3, T>;
/*class TMatrix3x3 : public glm::mat<3, 3, T>
{
public:
	TMatrix3x3() {}
	TMatrix3x3(const TMatrix3x3& mat) : glm::mat<3, 3, T>(mat) {}
	TMatrix3x3(T v) : glm::mat<3, 3, T>(v) {}
};*/

template<class T>
using TMatrix4x4 = glm::mat<4, 4, T>;
/*class TMatrix4x4 : public glm::mat<4, 4, T>
{
public:
	TMatrix4x4() {}
	TMatrix4x4(const TMatrix4x4& mat) : glm::mat<4, 4, T>(mat) {}
	TMatrix4x4(T v) : glm::mat<4, 4, T>(v) {}
};*/

typedef TVector2<float> FVector2;
typedef TVector2<double> DVector2;
typedef TVector2<int> IVector2;

typedef TVector3<float> FVector3;
typedef TVector3<double> DVector3;
typedef TVector3<int> IVector3;
typedef TVector3<real> RVector3;

typedef TVector4<float> FVector4;
typedef TVector4<double> DVector4;
typedef TVector4<int> IVector4;
typedef TVector4<real> RVector4;

typedef TQuaternion<float> FQuaternion;
typedef TQuaternion<double> DQuaternion;

typedef TMatrix2x2<float> FMatrix2x2;
typedef TMatrix2x2<double> DMatrix2x2;

typedef TMatrix3x3<float> FMatrix3x3;
typedef TMatrix3x3<double> DMatrix3x3;

typedef TMatrix4x4<float> FMatrix4x4;
typedef TMatrix4x4<double> DMatrix4x4;
typedef TMatrix4x4<real> RMatrix4x4;

template<class T>
TVector2<T> Normalize(const TVector2<T>& vector)
{
	return glm::normalize(vector);
}

template<class T>
TVector3<T> Normalize(const TVector3<T>& vector)
{
	return glm::normalize(vector);
}

template<class T>
TVector4<T> Normalize(const TVector4<T>& vector)
{
	return glm::normalize(vector);
}

template<class T>
TVector3<T> Cross(const TVector3<T>& vector0, const TVector3<T>& vector1)
{
	return glm::cross(vector0, vector1);
}

template<class T>
TVector2<T> Transform(const TVector2<T>& position, const TMatrix3x3<T>& matrix)
{
	// (position, 1.0)
	TVector3<T> temp(position.x, position.y, 1.0);
	temp = matrix * temp;
	// temp.SwizelXY
	return TVector2<T>(temp.x, temp.y);
}

template<class T>
TVector3<T> Transform(const TVector3<T>& position, const TMatrix4x4<T>& matrix)
{
	// (position, 1.0)
	TVector4<T> temp(position.x, position.y, position.z, 1.0);
	temp = matrix * temp;
	// temp.SwizelXYZ
	return TVector3<T>(temp.x, temp.y, temp.z);
}

namespace CreateMatrix
{
	template<class T>
	TMatrix4x4<T> Translation(const TVector3<T>& translation)
	{
		return glm::translate(translation);
	}

	template<class T>
	TMatrix2x2<T> Rotation2x2(T rotation)
	{
		TMatrix2x2<T> matrix;

		T c = cos(rotation);
		T s = sin(rotation);
		matrix[0][0] = c;
		matrix[0][1] = s;
		matrix[1][0] = -s;
		matrix[1][1] = c;

		return matrix;
	}
	
	template<class T>
	TMatrix3x3<T> Rotation3x3(const TQuaternion<T>& quat)
	{
		return glm::toMat3(quat);
	}

	template<class T>
	TMatrix4x4<T> Rotation4x4(const TQuaternion<T>& quat)
	{
		return glm::toMat4(quat);
	}

	template<class T>
	TMatrix4x4<T> Rotation(T angle, const TVector3<T>& axis)
	{
		return glm::rotate(angle, axis);
	}

	template<class T>
	TMatrix4x4<T> Scale(const TVector3<T>& scale)
	{
		return glm::scale(scale);
	}

	template<class T>
	TMatrix4x4<T> Projection(T fov, T aspect, T zNear, T zFar)
	{
		return glm::perspective(glm::radians(fov), aspect, zNear, zFar);
	}

	template<class T>
	TMatrix4x4<T> LookAt(const TVector3<T>& eye, const TVector3<T>& center, const TVector3<T>& up)
	{
		return glm::lookAt(eye, center, up);
	}
}