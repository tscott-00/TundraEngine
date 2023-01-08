#include "value_noise.h"
#include <math.h>

ValueGenerator::ValueGenerator() :
	generator(INT64_C(4237362089)) { }

void ValueGenerator::operator=(int64_t seed) {
	generator = seed;
}

double ValueGenerator::operator()(double x, double y) {
	int32_t intX = static_cast<int32_t>(x);
	int32_t intY = static_cast<int32_t>(y);
	return Interpolate(Interpolate(generator(intX, intY),     generator(intX + 1, intY),     x - intX),
					   Interpolate(generator(intX, intY + 1), generator(intX + 1, intY + 1), x - intX), y - intY);
}

//could optimize by only calculating blending factor for fractX once
double ValueGenerator::Interpolate(double a, double b, double blend) {
	double cosBlend = (1.0 - cos(blend * 3.141592653589793)) * 0.5;
	return a * (1.0 - cosBlend) + b * cosBlend;
}
