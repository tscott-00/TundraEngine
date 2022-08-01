#pragma once

#include <list>
#include <iostream>
#include <memory>

#include "general_common.h"
#include "game_math.h"

static const FMatrix3x3 FLOAT_MAT3_IDENTITY = FMatrix3x3(1.0F);
static const RMatrix4x4 REAL_MAT4_IDENTITY = RMatrix4x4(REAL_ONE);

template<class T>
class TGameObject;

// This is a base template class and should never be used directly
template<class T>
class TTransform
{
protected:
	TGameObject<T>* owner;
	T* parent;
	std::list<T*> children;
	bool isDirty;
	bool isEnabled;

	void MakeDirty()
	{
		if (!isDirty)
		{
			isDirty = true;
			for (auto child : children)
				child->MakeDirty();
		}
	}
public:
	TTransform() :
		owner(nullptr),
		parent(nullptr),
		isDirty(true),
		isEnabled(false)
	{ }

	~TTransform() noexcept
	{
		for (T* child : children)
			child->parent = nullptr;
		if (this->parent != nullptr)
			this->parent->children.remove(reinterpret_cast<T*>(this));
	}

	inline const std::list<T*> GetChildren() const // Must return copy because destructor removes and invalidates iterators and this is used in Destroy
	{
		return children;
	}

	inline void ClearChildren()
	{
		children.clear();
	}

	inline void SetOwner(TGameObject<T>* owner)
	{
		this->owner = owner;
	}

	inline TGameObject<T>* GetOwner() const
	{
		return owner;
	}

	inline void rtc_ForceDirty()
	{
		MakeDirty();
	}

	inline void SetParent(T* parent)
	{
		if (this->parent != nullptr)
			this->parent->children.remove(reinterpret_cast<T*>(this));
		if (parent != nullptr)
			parent->children.push_back(reinterpret_cast<T*>(this));
		this->parent = parent;
		MakeDirty();
	}

	inline void SetParent(T& parent)
	{
		if (this->parent != nullptr)
			this->parent->children.remove(reinterpret_cast<T*>(this));
		parent.children.push_back(reinterpret_cast<T*>(this));
		this->parent = &parent;
		MakeDirty();
	}

	inline T* GetParent()
	{
		return parent;
	}
};

// TODO: Init matrix in static pool
class Transform3 : public TTransform<Transform3>
{
	friend class TGameObject<Transform3>;
private:
	static MemoryPool<RMatrix4x4> matrices;

	bool ownsAllocation;

	FVector3 scale;
	FQuaternion rotation;
	RVector3 position;

	RMatrix4x4* matrix;
public:
	Transform3(const TVector3<real>& position = RVector3(REAL_ZERO, REAL_ZERO, REAL_ZERO), const FVector3& scale = FVector3(1.0F, 1.0F, 1.0F), const FQuaternion& rotation = FQuaternion(1.0F, 0.0F, 0.0F, 0.0F)) :
		position(position),
		scale(scale),
		rotation(rotation),
		ownsAllocation(true),
		matrix(matrices.alloc(REAL_ONE))
	{ }

	Transform3(const Transform3& other) :
		position(other.position),
		scale(other.scale),
		rotation(other.rotation),
		ownsAllocation(true),
		matrix(matrices.alloc(REAL_ONE))
	{ }

	~Transform3() noexcept
	{
		if (ownsAllocation)
			matrices.free(matrix);
	}

	Transform3& operator=(const Transform3& other) noexcept
	{
		MakeDirty();
		position = other.position;
		scale = other.scale;
		rotation = other.rotation;

		return *this;
	}

	void PairAllocation(RMatrix4x4* matrix)
	{
		assert(ownsAllocation);

		memcpy(matrix, this->matrix, sizeof(RMatrix4x4));
		matrices.free(this->matrix);
		this->matrix = matrix;
		ownsAllocation = false;
	}

	void UnpairAllocation()
	{
		assert(!ownsAllocation);

		matrix = matrices.alloc(*matrix);
		ownsAllocation = true;
	}

	inline const RVector3& GetPosition() const
	{
		return position;
	}

	inline void SetPosition(const RVector3& position)
	{
		this->position = position;
		MakeDirty();
	}

	inline void ChangePosition(const RVector3& delta)
	{
		this->position += delta;
		MakeDirty();
	}

	// TODO: Functions to set world vars?

	inline RVector3 GetWorldPosition() const
	{
		RVector4 pos(position, REAL_ONE);
		pos = GetLocalToWorldMatrix() * pos;
		return RVector3(pos);
	}

	// TODO: Implement
	inline void SetWorldPosition(const RVector3& position)
	{
		this->position = position;
		MakeDirty();
	}

	inline void LookAtPosition(const RVector3& position)
	{
		FMatrix3x3 result;
		result[2] = -Normalize(static_cast<FVector3>(position - this->position));
		float radians = 90.0F / 180.0F * 3.141592653589793F;
		float s = sinf(radians);
		float c = cosf(radians);
		result[0][0] = result[2][0] * c - result[2][2] * s;
		result[0][1] = 0.0f;
		result[0][2] = result[2][0] * s + result[2][2] * c;
		result[1] = Cross(result[2], result[0]);
		
		rotation = FQuaternion(result);

		MakeDirty();
	}

	inline const FVector3& GetScale() const
	{
		return scale;
	}

	inline const FQuaternion& GetRotation() const
	{
		return rotation;
	}

	inline void SetScale(const FVector3& scale)
	{
		this->scale = scale;
		this->MakeDirty();
	}

	inline void SetRotation(const FQuaternion& rotation)
	{
		this->rotation = rotation;
		this->MakeDirty();
	}

	inline FVector3 GetWorldRight() const
	{
		FMatrix3x3 rotation = GetWorldRotationMatrix3x3();
		return rotation[0];
	}

	inline FVector3 GetWorldUp() const
	{
		FMatrix3x3 rotation = GetWorldRotationMatrix3x3();
		return rotation[1];
	}

	inline FVector3 GetWorldForward() const
	{
		FMatrix3x3 rotation = GetWorldRotationMatrix3x3();
		return -rotation[2];
	}

	FVector3 GetWorldScale() const
	{
		if (this->parent == nullptr)
			return scale;
		else
			return this->parent->GetWorldScale() * scale;
	}

	FQuaternion GetWorldRotation() const
	{
		if (this->parent == nullptr)
			return rotation;
		else
			return this->parent->GetWorldRotation() * rotation;
	}

	// TODO: Implement
	inline void SetWorldRotation(const FQuaternion& rotation)
	{
		this->rotation = rotation;
		MakeDirty();
	}

	inline FMatrix3x3 GetWorldRotationMatrix3x3() const
	{
		return CreateMatrix::Rotation3x3(GetWorldRotation());
	}

	inline FMatrix4x4 GetWorldRotationMatrix4x4() const
	{
		return CreateMatrix::Rotation4x4(GetWorldRotation());
	}

	inline const RMatrix4x4& GetLocalToWorldMatrix() const
	{
		if (parent != nullptr)
			return parent->GetMatrix();
		else
			return REAL_MAT4_IDENTITY;
	}

	const RMatrix4x4& GetMatrix()
	{
		if (isDirty)
		{
			RMatrix4x4& matrix = *this->matrix;
			if (parent != nullptr)
				matrix = parent->GetMatrix();
			else
				matrix = REAL_MAT4_IDENTITY;
			matrix *= CreateMatrix::Translation(position) * static_cast<RMatrix4x4>(CreateMatrix::Rotation4x4(rotation) * CreateMatrix::Scale(scale));
			isDirty = false;
		}

		return *matrix;
	}
	
	// TODO: bool if world position stays the same globally https://docs.unity3d.com/ScriptReference/Transform.SetParent.html
	// TODO: SetChildIndex https://docs.unity3d.com/ScriptReference/Transform.SetSiblingIndex.html
};

class Transform2;

class Constraint
{
protected:
	float GetDomainPixelScale(const Transform2& transform) const;
public:
	float value;
	bool isRelative;
	uint8_t component;

	Constraint(float value, bool isRelative) :
		value(value),
		isRelative(isRelative),
		component(0)
	{ }

	virtual void EnforceValidity(Transform2& transform) const = 0;
};

class PosConstraint : public Constraint
{
protected:
	void UpdateTransform(Transform2& transform, float pos) const;
	float GetScaleComponent(Transform2& transform) const;
public:
	PosConstraint(float value, bool isRelative = false) :
		Constraint(value, isRelative)
	{ }
};
#define pos_constraint(constraint) std::shared_ptr<PosConstraint>(new constraint)

class ScaleConstraint : public Constraint
{
protected:
	void UpdateTransform(Transform2& transform, float scale) const;
	float GetPosComponent(Transform2& transform) const;
public:
	ScaleConstraint(float value, bool isRelative = false) :
		Constraint(value, isRelative)
	{ }
};
#define scale_constraint(constraint) std::shared_ptr<ScaleConstraint>(new constraint)

#define all_constraints(posConstraintX, posConstraintY, scaleConstraintX, scaleConstraintY) pos_constraint(posConstraintX), pos_constraint(posConstraintY), scale_constraint(scaleConstraintX), scale_constraint(scaleConstraintY)

// TODO: Need copy constructor
class Transform2 : public TTransform<Transform2>
{
	friend class TGameObject<Transform2>;

	friend class Constraint;
	friend class PosConstraint;
	friend class ScaleConstraint;
public:
	struct SeperatedData
	{
		FMatrix3x3 matrix;
		std::shared_ptr<PosConstraint> xConstraint, yConstraint;
		std::shared_ptr<ScaleConstraint> widthConstraint, heightConstraint;

		SeperatedData(std::shared_ptr<PosConstraint> xConstraint, std::shared_ptr<PosConstraint> yConstraint, std::shared_ptr<ScaleConstraint> widthConstraint, std::shared_ptr<ScaleConstraint> heightConstraint) :
			matrix(1.0F),
			xConstraint(xConstraint),
			yConstraint(yConstraint),
			widthConstraint(widthConstraint),
			heightConstraint(heightConstraint)
		{ }
	};
private:
	static MemoryPool<SeperatedData> seperatedDatas;

	bool ownsAllocation;

	FVector2 position;
	FVector2 scale;
	float rotation;
	FVector4 texInfo;

	SeperatedData* seperatedData;
public:
	Transform2(const FVector2& position = FVector2(0.0F, 0.0F), const FVector2& scale = FVector2(1.0F, 1.0F), float rotation = 0.0F, const FVector4& texInfo = FVector4(0.0F, 0.0F, 1.0F, 1.0F)) :
		position(position),
		scale(scale),
		rotation(rotation),
		texInfo(texInfo),
		ownsAllocation(true),
		seperatedData(seperatedDatas.alloc(nullptr, nullptr, nullptr, nullptr))
	{ }

	Transform2(std::shared_ptr<PosConstraint> xConstraint, std::shared_ptr<PosConstraint> yConstraint, std::shared_ptr<ScaleConstraint> widthConstraint, std::shared_ptr<ScaleConstraint> heightConstraint) :
		position(FVector2(0.0F, 0.0F)),
		scale(FVector2(1.0F, 1.0F)),
		rotation(0.0F),
		texInfo(FVector4(0.0F, 0.0F, 1.0F, 1.0F)),
		ownsAllocation(true),
		seperatedData(seperatedDatas.alloc(xConstraint, yConstraint, widthConstraint, heightConstraint))
	{
		if (yConstraint != nullptr)
			yConstraint->component = 1;
		if (heightConstraint != nullptr)
			heightConstraint->component = 1;
	}

	Transform2& WithConstraints(std::shared_ptr<PosConstraint> xConstraint, std::shared_ptr<PosConstraint> yConstraint, std::shared_ptr<ScaleConstraint> widthConstraint, std::shared_ptr<ScaleConstraint> heightConstraint)
	{
		if (yConstraint != nullptr)
			yConstraint->component = 1;
		if (heightConstraint != nullptr)
			heightConstraint->component = 1;

		seperatedData->xConstraint = xConstraint;
		seperatedData->yConstraint = yConstraint;
		seperatedData->widthConstraint = widthConstraint;
		seperatedData->heightConstraint = heightConstraint;

		MakeDirty();

		return *this;
	}

	Transform2& WithXConstraint(std::shared_ptr<PosConstraint> constraint)
	{
		seperatedData->xConstraint = constraint;

		return *this;
	}

	Transform2& WithXConstraint(std::shared_ptr<ScaleConstraint> constraint)
	{
		seperatedData->widthConstraint = constraint;

		return *this;
	}

	Transform2& WithYConstraint(std::shared_ptr<PosConstraint> constraint)
	{
		if (constraint != nullptr)
			constraint->component = 1;

		seperatedData->yConstraint = constraint;

		return *this;
	}

	Transform2& WithYConstraint(std::shared_ptr<ScaleConstraint> constraint)
	{
		if (constraint != nullptr)
			constraint->component = 1;

		seperatedData->heightConstraint = constraint;

		return *this;
	}

	Transform2& operator=(const Transform2& other)
	{
		MakeDirty();

		position = other.position;
		scale = other.scale;
		rotation = other.rotation;
		texInfo = other.texInfo;
		seperatedData->xConstraint = other.seperatedData->xConstraint;
		seperatedData->yConstraint = other.seperatedData->yConstraint;
		seperatedData->widthConstraint = other.seperatedData->widthConstraint;
		seperatedData->heightConstraint = other.seperatedData->heightConstraint;

		return *this;
	}

	void PairAllocation(SeperatedData* seperatedData)
	{
		assert(ownsAllocation);

		memcpy(seperatedData, this->seperatedData, sizeof(SeperatedData));
		seperatedDatas.free(this->seperatedData);
		this->seperatedData = seperatedData;
		ownsAllocation = false;
	}

	void UnpairAllocation()
	{
		assert(!ownsAllocation);

		SeperatedData* newSeperatedData = seperatedDatas.alloc(seperatedData->xConstraint, seperatedData->yConstraint, seperatedData->widthConstraint, seperatedData->heightConstraint);
		memcpy(&newSeperatedData->matrix, &seperatedData->matrix, sizeof(FMatrix3x3));
		seperatedData = newSeperatedData;
		ownsAllocation = true;
	}

	inline const FVector2& GetPosition() const
	{
		return position;
	}

	inline void SetPosition(const FVector2& position)
	{
		MakeDirty();
		this->position = position;
	}

	inline void ChangePosition(const FVector2& change)
	{
		MakeDirty();
		this->position += change;
	}

	inline const FVector2& GetScale() const
	{
		return scale;
	}

	inline void SetScale(const FVector2& scale)
	{
		MakeDirty();
		this->scale = scale;
	}

	inline float GetRotation() const
	{
		return rotation;
	}

	inline void SetRotation(float rotation)
	{
		MakeDirty();
		this->rotation = rotation;
	}

	inline void ChangeRotation(float change)
	{
		MakeDirty();
		this->rotation += change;
	}

	inline const FVector4& GetTexInfo() const
	{
		return texInfo;
	}

	inline void SetTexInfoBox(const FVector2& pos, const FVector2& scale)
	{
		this->texInfo.x = pos.x;
		this->texInfo.y = pos.y;
		this->texInfo.z = scale.x;
		this->texInfo.w = scale.y;
	}

	inline void SetTexInfoBounds(const FVector2& upperLeft, const FVector2& lowerRight)
	{
		this->texInfo.x = upperLeft.x;
		this->texInfo.y = upperLeft.y;
		this->texInfo.z = lowerRight.x - upperLeft.x;
		this->texInfo.w = lowerRight.y - upperLeft.y;
	}

	inline void SetTexInfo(const FVector4& texInfo)
	{
		this->texInfo = texInfo;
	}

	const FMatrix3x3& GetMatrix()
	{
		FMatrix3x3& matrix = seperatedData->matrix;

		if (isDirty)
		{
			if (parent != nullptr)
				matrix = parent->GetMatrix();
			else
				matrix = FLOAT_MAT3_IDENTITY;

			if (seperatedData->widthConstraint != nullptr)
				seperatedData->widthConstraint->EnforceValidity(*this);
			if (seperatedData->heightConstraint != nullptr)
				seperatedData->heightConstraint->EnforceValidity(*this);
			if (seperatedData->xConstraint != nullptr)
				seperatedData->xConstraint->EnforceValidity(*this);
			if (seperatedData->yConstraint != nullptr)
				seperatedData->yConstraint->EnforceValidity(*this);
			// Repeated enforcement is required in certain conditions where the size depends on the position (such as specific size constraints)
			if (seperatedData->widthConstraint != nullptr)
				seperatedData->widthConstraint->EnforceValidity(*this);
			if (seperatedData->heightConstraint != nullptr)
				seperatedData->heightConstraint->EnforceValidity(*this);

			FMatrix3x3 temp(1.0F);
			temp[2][0] = position.x;
			temp[2][1] = position.y;
			matrix *= temp;
			if (rotation != 0.0F)
			{
				temp = FLOAT_MAT3_IDENTITY;
				float c = cosf(rotation);
				float s = sinf(rotation);
				temp[0][0] = c;
				temp[0][1] = s;
				temp[1][0] = -s;
				temp[1][1] = c;
				matrix *= temp;
			}
			temp = FLOAT_MAT3_IDENTITY;
			temp[0][0] = scale.x;
			temp[1][1] = scale.y;
			matrix *= temp;
			isDirty = false;
		}

		return matrix;
	}
};

class CenterPosConstraint : public PosConstraint
{
public:
	CenterPosConstraint(float offset = 0.0F, bool isRelative = false) :
		PosConstraint(offset, isRelative)
	{ }

	void EnforceValidity(Transform2& transform) const override;
};

// TODO: Should work with transform pointers as well
class LesserEdgePosConstraint : public PosConstraint
{
public:
	LesserEdgePosConstraint(float offset = 0.0F, bool isRelative = false) :
		PosConstraint(offset, isRelative)
	{ }

	void EnforceValidity(Transform2& transform) const override;
};

class GreaterEdgePosConstraint : public PosConstraint
{
public:
	GreaterEdgePosConstraint(float offset = 0.0F, bool isRelative = false) :
		PosConstraint(offset, isRelative)
	{ }

	void EnforceValidity(Transform2& transform) const override;
};

// TODO: Doesn't work at all
class DualEdgePosConstraint : public PosConstraint
{
private:
	Transform2* lesserEdge;
	Transform2* greaterEdge;
public:
	DualEdgePosConstraint(Transform2* lesserEdge, Transform2* greaterEdge) :
		PosConstraint(0.0F, true),
		lesserEdge(lesserEdge),
		greaterEdge(greaterEdge)
	{ }

	void EnforceValidity(Transform2& transform) const override;
};

class SpecificLesserEdgePosConstraint : public PosConstraint
{
private:
	Transform2* edge;
public:
	SpecificLesserEdgePosConstraint(Transform2* edge, float offset = 0.0F, bool isRelative = false) :
		PosConstraint(offset, isRelative),
		edge(edge)
	{ }

	void EnforceValidity(Transform2& transform) const override;
};

class SpecificGreaterEdgePosConstraint : public PosConstraint
{
private:
	Transform2* edge;
public:
	SpecificGreaterEdgePosConstraint(Transform2* edge, float offset = 0.0F, bool isRelative = false) :
		PosConstraint(offset, isRelative),
		edge(edge)
	{ }

	void EnforceValidity(Transform2& transform) const override;
};

class CenterScaleConstraint : public ScaleConstraint
{
public:
	CenterScaleConstraint(float size, bool isRelative = false) :
		ScaleConstraint(size, isRelative)
	{ }

	void EnforceValidity(Transform2& transform) const override;
};

class LesserEdgeScaleConstraint : public ScaleConstraint
{
public:
	LesserEdgeScaleConstraint(float border = 0.0F, bool isRelative = false) :
		ScaleConstraint(border, isRelative)
	{ }

	void EnforceValidity(Transform2& transform) const override;
};

class GreaterEdgeScaleConstraint : public ScaleConstraint
{
public:
	GreaterEdgeScaleConstraint(float border = 0.0F, bool isRelative = false) :
		ScaleConstraint(border, isRelative)
	{ }

	void EnforceValidity(Transform2& transform) const override;
};

class DualEdgeScaleConstraint : public ScaleConstraint
{
private:
	Transform2* lesserEdge;
	Transform2* greaterEdge;
public:
	DualEdgeScaleConstraint(Transform2* lesserEdge, Transform2* greaterEdge, float border = 0.0F, bool isRelative = false) :
		ScaleConstraint(border, isRelative),
		lesserEdge(lesserEdge),
		greaterEdge(greaterEdge)
	{ }

	void EnforceValidity(Transform2& transform) const override;
};

class SpecificLesserEdgeScaleConstraint : public ScaleConstraint
{
private:
	Transform2* edge;
public:
	SpecificLesserEdgeScaleConstraint(Transform2* edge, float border = 0.0F, bool isRelative = false) :
		ScaleConstraint(border, isRelative),
		edge(edge)
	{ }

	void EnforceValidity(Transform2& transform) const override;
};

class SpecificGreaterEdgeScaleConstraint : public ScaleConstraint
{
private:
	Transform2* edge;
public:
	SpecificGreaterEdgeScaleConstraint(Transform2* edge, float border = 0.0F, bool isRelative = false) :
		ScaleConstraint(border, isRelative),
		edge(edge)
	{ }

	void EnforceValidity(Transform2& transform) const override;
};

class AspectRatioScaleConstraint : public ScaleConstraint
{
public:
	AspectRatioScaleConstraint(float aspectRatio) :
		ScaleConstraint(aspectRatio, true)
	{ }

	void EnforceValidity(Transform2& transform) const override;
};