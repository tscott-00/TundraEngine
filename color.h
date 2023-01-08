#pragma once

#include "general_common.h"

class FColor {
public:
	static const FColor WHITE;
	static const FColor BLACK;
	static const FColor RED;
	static const FColor GREEN;
	static const FColor BLUE;
	static const FColor PURPLE;
	static const FColor YELLOW;
	static const FColor CYAN;

	float r, g, b, a;

	FColor(float r, float g, float b, float a) {
		this->r = r;
		this->g = g;
		this->b = b;
		this->a = a;
	}

	FColor(float r, float g, float b) {
		this->r = r;
		this->g = g;
		this->b = b;
		a = 1.0F;
	}

	FColor(float v) {
		this->r = v;
		this->g = v;
		this->b = v;
		a = 1.0f;
	}

	FColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
		this->r = r / 255.0F;
		this->g = g / 255.0F;
		this->b = b / 255.0F;
		this->a = a / 255.0F;
	}

	FColor(uint8_t r, uint8_t g, uint8_t b) {
		this->r = r / 255.0F;
		this->g = g / 255.0F;
		this->b = b / 255.0F;
		a = 1.0F;
	}

	FColor(uint8_t v) {
		this->r = v / 255.0F;
		this->g = v / 255.0F;
		this->b = v / 255.0F;
		a = 1.0f;
	}

	FColor() {
		r = 1.0F;
		g = 1.0F;
		b = 1.0F;
		a = 1.0f;
	}

	FColor RGBPow(float power) {
		return FColor(powf(r, power), powf(g, power), powf(b, power), a);
	}

	uint32_t GetBin() {
		return static_cast<uint32_t>(roundf(r * 255.0F)) & (static_cast<uint32_t>(roundf(g * 255.0F)) << 8) & (static_cast<uint32_t>(roundf(b * 255.0F)) << 16) & (static_cast<uint32_t>(roundf(a * 255.0F)) << 24);
	}

	std::string GetColorSpecifierStr() const {
		return "color=" + std::to_string(r) + "," + std::to_string(g) + "," + std::to_string(b) + "," + std::to_string(a);
	}

	inline bool operator==(const FColor& other) const {
		return r == other.r && g == other.g && b == other.b && a == other.a;
	}

	inline bool operator!=(const FColor& other) const {
		return r != other.r || g != other.g || b != other.b || a != other.a;
	}
};
