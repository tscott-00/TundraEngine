#pragma once

#include "rreal.h"

class ValueGenerator {
public:
	ValueGenerator();

	void operator=(int64_t seed);
	double operator()(double x, double y);
private:
	double Interpolate(double a, double b, double blend);

	RReal generator;
};
