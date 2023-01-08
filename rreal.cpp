#include "rreal.h"

#include <iostream>
#include <string>

RReal::RReal(int64_t seed) :
	seed(Init(seed)) { }

RReal::RReal() :
	seed(INT64_C(0)) { }

RReal::~RReal() { }

void RReal::operator=(int64_t seed) {
	this->seed = seed;
}

double RReal::operator()(int32_t x, int32_t y) {
	return GetDouble(x, y);
}

long double RReal::GetLongDouble(int32_t x, int32_t y) {
	return ((long double) GetDouble(x, y)) * ((long double) GetDouble(x + 1, y));
}

double RReal::GetDouble(int32_t x, int32_t y) {
	uint64_t seed0 = ((uint64_t)(Init(this->seed + (x * INT32_C(49632) + y * INT32_C(325176))) * MULTIPLIER + ADDEND) & MASK);
	uint64_t seed1 = ((uint64_t)(seed0 * MULTIPLIER + ADDEND) & MASK);
	uint32_t r0 = (uint32_t)(seed0 >> (INT64_C(48) - INT64_C(26)));
	uint32_t r1 = (uint32_t)(seed1 >> (INT64_C(48) - INT64_C(27)));
	return (((uint64_t)(r0) << INT64_C(27)) + r1) * DOUBLE_UNIT;
}

float RReal::GetFloat(int32_t x, int32_t y) {
	uint64_t seed0 = ((uint64_t)(Init(this->seed + (x * INT32_C(49632) + y * INT32_C(325176))) * MULTIPLIER + ADDEND) & MASK);
	uint32_t r0 = (uint32_t)(seed0 >> (INT64_C(48) - INT64_C(24)));
	return r0 * FLOAT_UNIT;
}

int8_t RReal::GetByte(int32_t x, int32_t y) {
	uint64_t seed0 = ((uint64_t)(Init(this->seed + (x * INT32_C(49632) + y * INT32_C(325176))) * MULTIPLIER + ADDEND) & MASK);
	return (uint8_t)(seed0 >> (INT64_C(48) - INT64_C(8)));
}

int16_t RReal::GetShort(int32_t x, int32_t y) {
	uint64_t seed0 = ((uint64_t)(Init(this->seed + (x * INT32_C(49632) + y * INT32_C(325176))) * MULTIPLIER + ADDEND) & MASK);
	return (uint8_t)(seed0 >> (INT64_C(48) - INT64_C(16)));
}

int32_t RReal::GetInt(int32_t x, int32_t y) {
	uint64_t seed0 = ((uint64_t)(Init(this->seed + (x * INT32_C(49632) + y * INT32_C(325176))) * MULTIPLIER + ADDEND) & MASK);
	return (uint32_t)(seed0 >> (INT64_C(48) - INT64_C(32)));
}

int64_t RReal::GetLong(int32_t x, int32_t y) {
	uint64_t seed0 = ((uint64_t)(Init(this->seed + (x * INT32_C(49632) + y * INT32_C(325176))) * MULTIPLIER + ADDEND) & MASK);
	uint64_t seed1 = ((uint64_t)(seed0 * MULTIPLIER + ADDEND) & MASK);
	uint32_t r0 = (uint32_t)(seed0 >> (INT64_C(48) - INT64_C(32)));
	uint32_t r1 = (uint32_t)(seed1 >> (INT64_C(48) - INT64_C(32)));
	return (((uint64_t)r0) << INT64_C(32)) + (uint64_t)r1;
}

inline int64_t RReal::Init(int64_t seed) const {
	return (seed ^ MULTIPLIER) & MASK;
}

const int64_t RReal::MULTIPLIER = INT64_C(0x5DEECE66D);
const int64_t RReal::ADDEND = INT64_C(0xB);
const int64_t RReal::MASK = (INT64_C(1) << INT64_C(48)) - INT64_C(1);
const double RReal::DOUBLE_UNIT = 1.0 / (INT64_C(1) << INT64_C(53));//1.0e-53;
const float RReal::FLOAT_UNIT = 1.0f / (INT32_C(1) << INT32_C(24));
