#pragma once

#include "general_common.h"
#include "updateable.h"
#include "texture.h"
#include "color.h"
#include "shader.h"
#include "mesh.h"

class SpecialState
{
public:
	SpecialState() { }

	virtual void CPUStart() { }
	virtual void CPUStop() { }
	virtual void GPUStart() { }
	virtual void GPUStop() { }
};

class Camera2 : public Component2, public SpecialState
{
public:
	Camera2() { }

	void CPUStart() override;
	void CPUStop() override;
	void GPUStart() override;
	void GPUStop() override;
	
	can_copy(Camera2)
};

#include "events_state.h"

// TODO: Needs to update some sort of input bounds (CPU)
class ScissorState : public SpecialState
{
protected:
	GLint x, y;
	GLsizei width, height;
public:
	ScissorState() :
		x(0),
		y(0),
		width(EventsState::windowWidth),
		height(EventsState::windowHeight)
	{ }

	ScissorState(GLint x, GLint y, GLsizei width, GLsizei height) :
		x(x),
		y(y),
		width(width),
		height(height)
	{ }

	void GPUStart() override
	{
		glEnable(GL_SCISSOR_TEST);
		glScissor(x, y, width, height);
	}

	void GPUStop() override
	{
		glDisable(GL_SCISSOR_TEST);
	}
};

class ScissorStateComponent : public Component2, public SpecialState
{
public:
	ScissorStateComponent() { }

	void GPUStart() override
	{
		FVector3 llCorner = gameObject->transform.GetMatrix() * FVector3(-1.0F, -1.0F, 1.0F);
		FVector3 urCorner = gameObject->transform.GetMatrix() * FVector3(1.0F, 1.0F, 1.0F);
		int x0 = static_cast<int>((llCorner.x + 1.0F) * 0.5F * EventsState::windowWidth);
		int y0 = static_cast<int>((llCorner.y + 1.0F) * 0.5F * EventsState::windowHeight);
		int x1 = static_cast<int>((urCorner.x + 1.0F) * 0.5F * EventsState::windowWidth);
		int y1 = static_cast<int>((urCorner.y + 1.0F) * 0.5F * EventsState::windowHeight);

		glEnable(GL_SCISSOR_TEST);
		glScissor(x0, y0, x1 - x0, y1 - y0);
	}

	void GPUStop() override
	{
		glDisable(GL_SCISSOR_TEST);
	}

	can_copy(ScissorStateComponent);
};

class RenderComponent2 : public Component2
{
public:
	static const FVector2 UNIT_UPPER_RIGHT, UNIT_UPPER_LEFT, UNIT_LOWER_LEFT, UNIT_LOWER_RIGHT;

	FVector4 atlasTexInfo;

	FColor color;
	unsigned short quadCount;
	// TODO: shared_ptr?
	SpecialState* specialState;

	RenderComponent2(const std::string& textureName, const FColor& color = FColor::WHITE, unsigned short quadCount = 1);
	
	RenderComponent2(const RenderComponent2& other) :
		atlasTexInfo(other.atlasTexInfo),
		color(other.color),
		quadCount(other.quadCount),
		specialState(other.specialState)
	{ }

	RenderComponent2* WithSpecialState(SpecialState* specialState)
	{
		this->specialState = specialState;

		return this;
	}

	void SetTexture(const std::string& textureName);

	virtual void LoadData(FVector2* vertices, FVector2* texCoords, FColor* colors);

	void OnEnable() override;
	void OnDisable() override;

	can_copy(RenderComponent2)
};

class IndependentRenderComponent2 : public Component2
{
public:
	Texture2D texture;
	FColor color;

	IndependentRenderComponent2(const Texture2D& texture, const FColor& color = FColor::WHITE) :
		texture(texture),
	    color(color) { }

	IndependentRenderComponent2(const IndependentRenderComponent2& other) :
		texture(other.texture),
		color(other.color) { }

	void OnEnable() override;
	void OnDisable() override;

	can_copy(IndependentRenderComponent2)
};

class UIComponent : public Component2
{
public:
	UIComponent() { }

	virtual bool ContainsPoint(float x, float y);

	// Order of events:
	// hover->click->release. Select and deselect are not guaranteed to occur in any particular order
	virtual void OnHover(const GameTime& deltaTime) { }
	virtual void OnClick(int button) { }
	virtual void OnRelease(int button) { }
	// Component is selected when unselected and clicked on with any of the three mouse buttons. Component is deselected when selected and a click occurs elsewhere.
	virtual void OnSelect() { }
	virtual void OnDeselect() { }

	void OnEnable() override;
	void OnDisable() override;
};

class MouseReleasingComponent2 : public Component2
{
public:
	MouseReleasingComponent2() { }
	MouseReleasingComponent2(MouseReleasingComponent2& other) { }

	void OnEnable() override;
	void OnDisable() override;
};

/*class UIFadeThenDestroy : public FUpdateableComponent2
{
private:
	float timeToFade;
	float timeElapsed;

	UINode* node;
	float startAlpha;
public:
	UIFadeThenDestroy(float timeToFade) :
		FUpdateableComponent2("Fade")
	{
		this->timeToFade = timeToFade;
		timeElapsed = 0.0F;
	}

	UIFadeThenDestroy* rtc_GetCopy() const override
	{
		return new UIFadeThenDestroy(*this);
	}

	void OnCreate() override
	{
		node = reinterpret_cast<UINode*>(gameObject);
		startAlpha = node->color.a;
	}

	void Update(const GameTime& time) override
	{
		timeElapsed += time.deltaTime;
		node->color.a = startAlpha * timeElapsed / timeToFade;
	}
private:
	UIFadeThenDestroy(const UIFadeThenDestroy& other) :
		FUpdateableComponent2(other)
	{
		this->timeToFade = other.timeToFade;
		timeElapsed = 0.0F;
	}
};*/

// TODO: This needs a better structure...
// everything should probably just be in an TGameObjectsState singleton
namespace Internal
{
	namespace Storage
	{
		struct GameObjects2
		{
			static Shader shader;
			static Shader independentShader;

			static FVector2* batchVertices;
			static FVector2* batchTexCoords;
			static FColor* batchColors;
			static Mesh batchMesh;
			static unsigned short batchCapacity;
			static Texture2DQuadAtlas texture;
			static FMatrix3x3 activeCameraMatrix;

			static UIComponent* underMouse;
			static UIComponent* selected;
		};
	}

	namespace GameObjects2
	{
		void Init(unsigned int initialAtlasSize, bool pixelArt = false);

		void Clear();

		void Update(const GameTime& time);

		void Render();
	}
}