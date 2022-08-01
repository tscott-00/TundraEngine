#pragma once
#include "rreal.h"

class ValueGenerator
{
public:
	ValueGenerator();
	~ValueGenerator();

	void operator=(int64_t seed);
	double operator()(double x, double y);
private:
	double SmoothNoise(int32_t x, int32_t y);
	double Interpolate(double a, double b, double blend);

	RReal generator;
};