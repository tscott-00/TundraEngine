#pragma once

#include "general_common.h"
#include "updateable.h"
#include "texture.h"
#include "color.h"
#include "shader.h"
#include "mesh.h"

class SpecialState {
public:
	SpecialState();

	virtual void CPUStart();
	virtual void CPUStop();
	virtual void GPUStart();
	virtual void GPUStop();
};

class Camera2 : public Component2, public SpecialState {
public:
	Camera2();

	void CPUStart() override;
	void CPUStop() override;
	void GPUStart() override;
	void GPUStop() override;
	
	can_copy(Camera2)
};

#include "events_state.h"

// TODO: Needs to update some sort of input bounds (CPU)
class ScissorState : public SpecialState {
protected:
	GLint x, y;
	GLsizei width, height;
public:
	ScissorState();
	ScissorState(GLint x, GLint y, GLsizei width, GLsizei height);

	void GPUStart() override;
	void GPUStop() override;
};

class ScissorStateComponent : public Component2, public SpecialState {
public:
	ScissorStateComponent();

	void GPUStart() override;
	void GPUStop() override;

	can_copy(ScissorStateComponent);
};

class RenderComponent2 : public Component2 {
public:
	static const FVector2 UNIT_UPPER_RIGHT, UNIT_UPPER_LEFT, UNIT_LOWER_LEFT, UNIT_LOWER_RIGHT;

	FVector4 atlasTexInfo;
	FColor color;
	unsigned short quadCount;
	// TODO: shared_ptr?
	SpecialState* specialState;

	RenderComponent2(const std::string& textureName, const FColor& color = FColor::WHITE, unsigned short quadCount = 1);
	RenderComponent2(const RenderComponent2& other);

	RenderComponent2* WithSpecialState(SpecialState* specialState);

	void SetTexture(const std::string& textureName);

	virtual void LoadData(FVector2* vertices, FVector2* texCoords, FColor* colors);

	void OnEnable() override;
	void OnDisable() override;

	can_copy(RenderComponent2)
};

class IndependentRenderComponent2 : public Component2 {
public:
	Texture2D texture;
	FColor color;

	IndependentRenderComponent2(const Texture2D& texture, const FColor& color = FColor::WHITE);
	IndependentRenderComponent2(const IndependentRenderComponent2& other);

	void OnEnable() override;
	void OnDisable() override;

	can_copy(IndependentRenderComponent2)
};

class UIComponent : public Component2 {
public:
	UIComponent();

	virtual bool ContainsPoint(float x, float y);

	// Order of events:
	// hover->click->release. Select and deselect are not guaranteed to occur in any particular order
	virtual void OnHover(const GameTime& deltaTime);
	virtual void OnClick(int button);
	virtual void OnRelease(int button);
	// Component is selected when unselected and clicked on with any of the three mouse buttons. Component is deselected when selected and a click occurs elsewhere.
	virtual void OnSelect();
	virtual void OnDeselect();

	void OnEnable() override;
	void OnDisable() override;
};

class MouseReleasingComponent2 : public Component2 {
public:
	MouseReleasingComponent2();
	MouseReleasingComponent2(MouseReleasingComponent2& other);

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
namespace Internal {
	namespace Storage {
		struct GameObjects2 {
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

	namespace GameObjects2 {
		void Init(unsigned int initialAtlasSize, bool pixelArt = false);
		void Clear();
		void Update(const GameTime& time);
		void Render();
	}
}
