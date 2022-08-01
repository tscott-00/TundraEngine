#pragma once

#include "general_common.h"

class RReal
{
public:
	RReal(int64_t seed);
	RReal();
	~RReal();

	void operator=(int64_t seed);
	double operator()(int32_t x, int32_t y);

	// TODO: Make const
	long double GetLongDouble(int32_t x, int32_t y);
	double GetDouble(int32_t x, int32_t y);
	float GetFloat(int32_t x, int32_t y);
	int8_t GetByte(int32_t x, int32_t y);
	int16_t GetShort(int32_t x, int32_t y);
	int32_t GetInt(int32_t x, int32_t y);
	int64_t GetLong(int32_t x, int32_t y);
private:
	inline int64_t Init(int64_t seed) const;

	int64_t seed;

	static const int64_t MULTIPLIER;
	static const int64_t ADDEND;
	static const int64_t MASK;
	static const double DOUBLE_UNIT;
	static const float FLOAT_UNIT;
};