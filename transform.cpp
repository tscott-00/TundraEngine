#include "transform.h"

#include "events_state.h"

// TODO: Can optimize some of these things with more Transform2 functions (pos+scale only transformation, global scale, etc.)
// TODO: Are all the fabs needed?

MemoryPool<RMatrix4x4> Transform3::matrices;
MemoryPool<Transform2::SeperatedData> Transform2::seperatedDatas;

float Constraint::GetDomainPixelScale(const Transform2& transform) const
{
	FVector3 parentCenter(0.0F, 0.0F, 1.0F);
	FVector3 parentEdge(1.0F * (!component & 0b1), 1.0F * component, 1.0F);
	if (transform.parent != nullptr)
	{
		parentCenter = transform.parent->GetMatrix() * parentCenter;
		parentEdge = transform.parent->GetMatrix() * parentEdge;
	}

	return fabs(parentEdge[component] - parentCenter[component]) * (component == 0 ? EventsState::windowWidth : EventsState::windowHeight);
}

void PosConstraint::UpdateTransform(Transform2& transform, float pos) const
{
	transform.position[component] = pos;
}

float PosConstraint::GetScaleComponent(Transform2& transform) const
{
	return transform.scale[component];
}

void ScaleConstraint::UpdateTransform(Transform2& transform, float scale) const
{
	transform.scale[component] = scale;
}

float ScaleConstraint::GetPosComponent(Transform2& transform) const
{
	return transform.position[component];
}

void CenterPosConstraint::EnforceValidity(Transform2& transform) const
{
	if (isRelative)
		UpdateTransform(transform, value);
	else
		UpdateTransform(transform, 2.0F * value / GetDomainPixelScale(transform));
}

void LesserEdgePosConstraint::EnforceValidity(Transform2& transform) const
{
	if (isRelative)
		UpdateTransform(transform, -1.0F + (GetScaleComponent(transform) + value));
	else
		UpdateTransform(transform, -1.0F + (GetScaleComponent(transform) + 2.0F * value / GetDomainPixelScale(transform)));
}

void GreaterEdgePosConstraint::EnforceValidity(Transform2& transform) const
{
	if (isRelative)
		UpdateTransform(transform, 1.0F - (GetScaleComponent(transform) + value));
	else
		UpdateTransform(transform, 1.0F - (GetScaleComponent(transform) + 2.0F * value / GetDomainPixelScale(transform)));
}

// Does this work globally?
void DualEdgePosConstraint::EnforceValidity(Transform2& transform) const
{
	if (component == 0)
	{
		FVector3 parentEdge = FVector3(1.0F, 1.0F, 1.0F);
		FVector3 parentPosition = FVector3(0.0F, 0.0F, 1.0F);
		FVector3 lesserPosition(1.0F, 0.0F, 1.0F);
		FVector3 greaterPosition(-1.0F, 0.0F, 1.0F);

		if (transform.GetParent() != nullptr)
		{
			parentEdge = transform.GetParent()->GetMatrix() * parentEdge;
			parentPosition = transform.GetParent()->GetMatrix() * parentPosition;
		}

		if (lesserEdge != nullptr)
			lesserPosition = lesserEdge->GetMatrix() * lesserPosition;
		else
			lesserPosition.x = 2 * parentPosition.x - parentEdge.x;

		if (greaterEdge != nullptr)
			greaterPosition = greaterEdge->GetMatrix() * greaterPosition;
		else
			greaterPosition.x = parentEdge.x;

		UpdateTransform(transform, (lesserPosition.x + greaterPosition.x) * 0.5F - parentPosition.x);
	}
	else
	{
		FVector3 parentEdge = FVector3(1.0F, 1.0F, 1.0F);
		FVector3 parentPosition = FVector3(0.0F, 0.0F, 1.0F);
		FVector3 lesserPosition(0.0F, 1.0F, 1.0F);
		FVector3 greaterPosition(0.0F, -1.0F, 1.0F);

		if (transform.GetParent() != nullptr)
		{
			parentPosition = transform.GetParent()->GetMatrix() * parentPosition;
			parentEdge = transform.GetParent()->GetMatrix() * parentEdge;
		}

		if (lesserEdge != nullptr)
			lesserPosition = lesserEdge->GetMatrix() * lesserPosition;
		else
			lesserPosition.y = 2 * parentPosition.y - parentEdge.y;

		if (greaterEdge != nullptr)
			greaterPosition = greaterEdge->GetMatrix() * greaterPosition;
		else
			greaterPosition.y = parentEdge.y;

		UpdateTransform(transform, (lesserPosition.y + greaterPosition.y) * 0.5F - parentPosition.y);
	}
}

void SpecificLesserEdgePosConstraint::EnforceValidity(Transform2& transform) const
{
	if (component == 0)
	{
		FVector3 otherEdge(1.0F, 0.0F, 1.0F);
		FVector3 parentEdge = FVector3(1.0F, 0.0F, 1.0F);
		FVector3 parentPosition = FVector3(0.0F, 0.0F, 1.0F);

		if (edge != nullptr)
			otherEdge = edge->GetMatrix() * otherEdge;
		else
			otherEdge.x = -1.0F; // TODO: Should be parent pos

		if (transform.GetParent() != nullptr)
		{
			parentPosition = transform.GetParent()->GetMatrix() * parentPosition;
			parentEdge = transform.GetParent()->GetMatrix() * parentEdge;
		}

		if (isRelative)
			UpdateTransform(transform, (otherEdge.x - ((parentEdge.x - parentPosition.x) * GetScaleComponent(transform) + parentPosition.x)) / fabs(parentEdge.x - parentPosition.x) + value);
		else
			UpdateTransform(transform, (otherEdge.x - ((parentEdge.x - parentPosition.x) * GetScaleComponent(transform) + parentPosition.x)) / fabs(parentEdge.x - parentPosition.x) + (2.0F * value / GetDomainPixelScale(transform)));
	}
	else
	{
		FVector3 otherEdge(0.0F, 1.0F, 1.0F);
		FVector3 parentEdge = FVector3(0.0F, 1.0F, 1.0F);
		FVector3 parentPosition = FVector3(0.0F, 0.0F, 1.0F);

		if (edge != nullptr)
			otherEdge = edge->GetMatrix() * otherEdge;
		else
			otherEdge.y = -1.0F;

		if (transform.GetParent() != nullptr)
		{
			parentPosition = transform.GetParent()->GetMatrix() * parentPosition;
			parentEdge = transform.GetParent()->GetMatrix() * parentEdge;
		}

		if (isRelative)
			UpdateTransform(transform, (otherEdge.y - ((parentEdge.y - parentPosition.y) * GetScaleComponent(transform) + parentPosition.y)) / fabs(parentEdge.y - parentPosition.y) + value);
		else
			UpdateTransform(transform, (otherEdge.y - ((parentEdge.y - parentPosition.y) * GetScaleComponent(transform) + parentPosition.y)) / fabs(parentEdge.y - parentPosition.y) + (2.0F * value / GetDomainPixelScale(transform)));
	}
}


void SpecificGreaterEdgePosConstraint::EnforceValidity(Transform2& transform) const
{
	if (component == 0)
	{
		FVector3 otherEdge(-1.0F, 0.0F, 1.0F);
		FVector3 parentEdge = FVector3(1.0F, 0.0F, 1.0F);
		FVector3 parentPosition = FVector3(0.0F, 0.0F, 1.0F);

		if (edge != nullptr)
			otherEdge = edge->GetMatrix() * otherEdge;
		else
			otherEdge.x = 1.0F;

		if (transform.GetParent() != nullptr)
		{
			parentPosition = transform.GetParent()->GetMatrix() * parentPosition;
			parentEdge = transform.GetParent()->GetMatrix() * parentEdge;
		}

		if (isRelative)
			UpdateTransform(transform, (otherEdge.x - ((parentEdge.x - parentPosition.x) * GetScaleComponent(transform) + parentPosition.x)) / fabs(parentEdge.x - parentPosition.x) - value);
		else
			UpdateTransform(transform, (otherEdge.x - ((parentEdge.x - parentPosition.x) * GetScaleComponent(transform) + parentPosition.x)) / fabs(parentEdge.x - parentPosition.x) - (2.0F * value / GetDomainPixelScale(transform)));
	}
	else
	{
		FVector3 otherEdge(0.0F, -1.0F, 1.0F);
		FVector3 parentEdge = FVector3(0.0F, 1.0F, 1.0F);
		FVector3 parentPosition = FVector3(0.0F, 0.0F, 1.0F);

		if (edge != nullptr)
			otherEdge = edge->GetMatrix() * otherEdge;
		else
			otherEdge.y = 1.0F;

		if (transform.GetParent() != nullptr)
		{
			parentPosition = transform.GetParent()->GetMatrix() * parentPosition;
			parentEdge = transform.GetParent()->GetMatrix() * parentEdge;
		}

		if (isRelative)
			UpdateTransform(transform, (otherEdge.y - ((parentEdge.y - parentPosition.y) * GetScaleComponent(transform) + parentPosition.y)) / fabs(parentEdge.y - parentPosition.y) - value);
		else
			UpdateTransform(transform, (otherEdge.y - ((parentEdge.y - parentPosition.y) * GetScaleComponent(transform) + parentPosition.y)) / fabs(parentEdge.y - parentPosition.y) - (2.0F * value / GetDomainPixelScale(transform)));
	}
}

void CenterScaleConstraint::EnforceValidity(Transform2& transform) const
{
	if (isRelative)
		UpdateTransform(transform, value * 0.5F);
	else
		UpdateTransform(transform, value / GetDomainPixelScale(transform));
}

void LesserEdgeScaleConstraint::EnforceValidity(Transform2& transform) const
{
	if (isRelative)
		UpdateTransform(transform, GetPosComponent(transform) + 1 - value);
	else
		UpdateTransform(transform, GetPosComponent(transform) + 1 - (2.0F * value / GetDomainPixelScale(transform)));
}

void GreaterEdgeScaleConstraint::EnforceValidity(Transform2& transform) const
{
	if (isRelative)
		UpdateTransform(transform, -GetPosComponent(transform) + 1 - value);
	else
		UpdateTransform(transform, -GetPosComponent(transform) + 1 - (2.0F * value / GetDomainPixelScale(transform)));
}

// TODO: Fabs is probably wrong for both this and the pos counterpart
void DualEdgeScaleConstraint::EnforceValidity(Transform2& transform) const
{
	if (component == 0)
	{
		FVector3 parentEdge = FVector3(1.0F, 1.0F, 1.0F);
		FVector3 parentPosition = FVector3(0.0F, 0.0F, 1.0F);
		FVector3 lesserPosition(1.0F, 0.0F, 1.0F);
		FVector3 greaterPosition(-1.0F, 0.0F, 1.0F);

		if (transform.GetParent() != nullptr)
		{
			parentPosition = transform.GetParent()->GetMatrix() * parentPosition;
			parentEdge = transform.GetParent()->GetMatrix() * parentEdge;
		}

		if (lesserEdge != nullptr)
			lesserPosition = lesserEdge->GetMatrix() * lesserPosition;
		else
			lesserPosition.x = 2 * parentPosition.x - parentEdge.x;

		if (greaterEdge != nullptr)
			greaterPosition = greaterEdge->GetMatrix() * greaterPosition;
		else
			greaterPosition.x = parentEdge.x;

		//std::cerr << lesserPosition.x << " " << greaterPosition.x << " " << 0.5F * fabs(greaterPosition.x - lesserPosition.x) / fabs(parentEdge.x - parentPosition.x) - (2.0F * value / GetDomainPixelScale(transform)) << std::endl;

		if (isRelative)
			UpdateTransform(transform, 0.5F * fabs(greaterPosition.x - lesserPosition.x) / fabs(parentEdge.x - parentPosition.x) - value);
		else
			UpdateTransform(transform, 0.5F * fabs(greaterPosition.x - lesserPosition.x) / fabs(parentEdge.x - parentPosition.x) - (2.0F * value / GetDomainPixelScale(transform)));
	}
	else
	{
		FVector3 parentEdge = FVector3(1.0F, 1.0F, 1.0F);
		FVector3 parentPosition = FVector3(0.0F, 0.0F, 1.0F);
		FVector3 lesserPosition(0.0F, 1.0F, 1.0F);
		FVector3 greaterPosition(0.0F, -1.0F, 1.0F);

		if (transform.GetParent() != nullptr)
		{
			parentPosition = transform.GetParent()->GetMatrix() * parentPosition;
			parentEdge = transform.GetParent()->GetMatrix() * parentEdge;
		}

		if (lesserEdge != nullptr)
			lesserPosition = lesserEdge->GetMatrix() * lesserPosition;
		else
			lesserPosition.y = 2 * parentPosition.y - parentEdge.y;

		if (greaterEdge != nullptr)
			greaterPosition = greaterEdge->GetMatrix() * greaterPosition;
		else
			greaterPosition.y = parentEdge.y;

		if (isRelative)
			UpdateTransform(transform, 0.5F * fabs(greaterPosition.y - lesserPosition.y) / fabs(parentEdge.y - parentPosition.y) - value);
		else
			UpdateTransform(transform, 0.5F * fabs(greaterPosition.y - lesserPosition.y) / fabs(parentEdge.y - parentPosition.y) - (2.0F * value / GetDomainPixelScale(transform)));
	}
}

void SpecificLesserEdgeScaleConstraint::EnforceValidity(Transform2& transform) const
{
	if (component == 0)
	{
		FVector3 edgePosition(1.0F, 0.0F, 1.0F);
		FVector3 parentEdge = FVector3(1.0F, 0.0F, 1.0F);
		FVector3 parentPosition = FVector3(0.0F, 0.0F, 1.0F);

		if (edge != nullptr)
			edgePosition = edge->GetMatrix() * edgePosition;
		else
			edgePosition.x = -1.0F;

		if (transform.GetParent() != nullptr)
		{
			parentPosition = transform.GetParent()->GetMatrix() * parentPosition;
			parentEdge = transform.GetParent()->GetMatrix() * parentEdge;
		}

		if (isRelative)
			UpdateTransform(transform, 0.5F * (edgePosition.x - parentEdge.x) / fabs(parentEdge.x - parentPosition.x) + value);
		else
			UpdateTransform(transform, 0.5F * (edgePosition.x - parentEdge.x) / fabs(parentEdge.x - parentPosition.x) + (2.0F * value / GetDomainPixelScale(transform)));
	}
	else
	{
		FVector3 edgePosition(0.0F, 1.0F, 1.0F);
		FVector3 parentEdge = FVector3(0.0F, 1.0F, 1.0F);
		FVector3 parentPosition = FVector3(0.0F, 0.0F, 1.0F);

		if (edge != nullptr)
			edgePosition = edge->GetMatrix() * edgePosition;
		else
			edgePosition.y = -1.0F;

		if (transform.GetParent() != nullptr)
		{
			parentPosition = transform.GetParent()->GetMatrix() * parentPosition;
			parentEdge = transform.GetParent()->GetMatrix() * parentEdge;
		}

		if (isRelative)
			UpdateTransform(transform, 0.5F * (edgePosition.y - parentEdge.y) / fabs(parentEdge.y - parentPosition.y) + value);
		else
			UpdateTransform(transform, 0.5F * (edgePosition.y - parentEdge.y) / fabs(parentEdge.y - parentPosition.y) + (2.0F * value / GetDomainPixelScale(transform)));
	}
}

void SpecificGreaterEdgeScaleConstraint::EnforceValidity(Transform2& transform) const
{
	if (component == 0)
	{
		FVector3 edgePosition(-1.0F, 0.0F, 1.0F);
		FVector3 parentEdge = FVector3(-1.0F, 0.0F, 1.0F);
		FVector3 parentPosition = FVector3(0.0F, 0.0F, 1.0F);

		if (edge != nullptr)
			edgePosition = edge->GetMatrix() * edgePosition;
		else
			edgePosition.x = 1.0F;

		if (transform.GetParent() != nullptr)
		{
			parentPosition = transform.GetParent()->GetMatrix() * parentPosition;
			parentEdge = transform.GetParent()->GetMatrix() * parentEdge;
		}

		if (isRelative)
			UpdateTransform(transform, 0.5F * (edgePosition.x - parentEdge.x) / fabs(parentEdge.x - parentPosition.x) - value);
		else
			UpdateTransform(transform, 0.5F * (edgePosition.x - parentEdge.x) / fabs(parentEdge.x - parentPosition.x) - (2.0F * value / GetDomainPixelScale(transform)));
	}
	else
	{
		FVector3 edgePosition(0.0F, -1.0F, 1.0F);
		FVector3 parentEdge = FVector3(0.0F, -1.0F, 1.0F);
		FVector3 parentPosition = FVector3(0.0F, 0.0F, 1.0F);

		if (edge != nullptr)
			edgePosition = edge->GetMatrix() * edgePosition;
		else
			edgePosition.y = 1.0F;

		if (transform.GetParent() != nullptr)
		{
			parentPosition = transform.GetParent()->GetMatrix() * parentPosition;
			parentEdge = transform.GetParent()->GetMatrix() * parentEdge;
		}

		if (isRelative)
			UpdateTransform(transform, 0.5F * (edgePosition.y - parentEdge.y) / fabs(parentEdge.y - parentPosition.y) - value);
		else
			UpdateTransform(transform, 0.5F * (edgePosition.y - parentEdge.y) / fabs(parentEdge.y - parentPosition.y) - (2.0F * value / GetDomainPixelScale(transform)));
	}
}

void AspectRatioScaleConstraint::EnforceValidity(Transform2& transform) const
{
	FVector3 parentOrigin(0.0F, 0.0F, 1.0F);
	FVector3 parentCorner(1.0F, 1.0F, 1.0F);
	if (transform.GetParent() != nullptr)
	{
		parentOrigin = transform.GetParent()->GetMatrix() * parentOrigin;
		parentCorner = transform.GetParent()->GetMatrix() * parentCorner;
	}

	float parentAspectRatio = fabs((parentCorner.x - parentOrigin.x) / (parentCorner.y - parentOrigin.y)) * EventsState::GetAspectRatio();

	if (component == 0)
		UpdateTransform(transform, transform.GetScale().y * value / parentAspectRatio);
	else
		UpdateTransform(transform, transform.GetScale().x / value * parentAspectRatio);
}