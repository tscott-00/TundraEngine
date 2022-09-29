#pragma once

#include "ui.h"

class Font
{
public:
	struct Character
	{
		FVector4 texCoords;
		FVector2 quadSize;
		FVector2 offsetFromCursor;
		float cursorXAdvance;

		Character();
		Character(const FVector4& texCoords, const FVector2& quadSize, const FVector2& offsetFromCursor, float cursorXAdvance);
	};

	float cursorYAdvance;
	Character characters[INT8_MAX + 1];
	float kerningMatrix[(INT8_MAX + 1) * (INT8_MAX + 1)];

	std::string directory;

	Font(const std::string& directory);

	no_transfer_functions(Font)
};

class TextRenderComponent2 : public RenderComponent2
{
private:
	class MySpecialState : public SpecialState
	{
	private:
		TextRenderComponent2* component;
	public:
		MySpecialState(TextRenderComponent2* component);

		void GPUStart() override;
		void GPUStop() override;
	} specialState;

	Font* font;
	std::string str;

	FVector2* vertices;
	FVector2* texCoords;
	FColor* colors;

	int characterSize;
	Transform2 charParentTransform;
	int lastCursorPos;
	FColor lastCursorColor;

	float widthDist, edgeDist, borderWidthDist, borderEdgeDist;
	FColor borderColor;
public:
	// Last bounds taken by used text
	FVector2 lastBounds;
	// -1 for none or the index to display it before
	int cursorPos;
	FColor cursorColor;

	TextRenderComponent2(Font* font, int characterSize, const std::string& str = "");
	TextRenderComponent2(const TextRenderComponent2& other);

	void OnCreate() override;

	~TextRenderComponent2();

	inline int GetCharacterSize() const
	{
		return characterSize;
	}

	void SetCharacterSize(int characterSize);

	inline const std::string& GetText() const
	{
		return str;
	}

	void SetText(const std::string& str, bool force = false);

	TextRenderComponent2* WithBorder(const FColor& color);
	void SetBorder(float widthDist, float edgeDist, float borderWidthDist, float borderEdgeDist, const FColor& borderColor);
	void EnableBorder(const FColor& color);
	void DisableBorder();

	void LoadData(FVector2* vertices, FVector2* texCoords, FColor* colors) override;

	can_copy(TextRenderComponent2)
};