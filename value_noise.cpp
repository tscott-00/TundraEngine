#include "value_noise.h"
#include <math.h>

ValueGenerator::ValueGenerator()
{
	generator = INT64_C(4237362089);
}

ValueGenerator::~ValueGenerator()
{
}

void ValueGenerator::operator=(int64_t seed)
{
	generator = seed;
}

#include <iostream>
#include <string>
double ValueGenerator::operator()(double x, double y)
{
	int32_t intX = (int)x;
	int32_t intY = (int)y;
	//std::cout << std::to_string(SmoothNoise(intX, intY)) << " " << std::to_string(SmoothNoise(intX + 1, intY)) << " " << std::to_string(x - intX) << " " << std::to_string(Interpolate(SmoothNoise(intX, intY), SmoothNoise(intX + 1, intY), x - intX)) << std::endl;
	return Interpolate(Interpolate(SmoothNoise(intX, intY), SmoothNoise(intX + 1, intY), x - intX),
					   Interpolate(SmoothNoise(intX, intY + 1), SmoothNoise(intX + 1, intY + 1), x - intX), y - intY);
}

double ValueGenerator::SmoothNoise(int32_t x, int32_t y)
{
	//return (generator(x - 1, y - 1) + generator(x + 1, y - 1) + generator(x - 1, y + 1) + generator(x + 1, y + 1)) * 0.075 +
	//	   (generator(x - 1, y) + generator(x, y - 1) + generator(x + 1, y) + generator(x, y + 1)) * 0.125 +
	//	   generator(x, y) * 0.2;
	//return (generator(x - 1, y - 1) + generator(x + 1, y - 1) + generator(x - 1, y + 1) + generator(x + 1, y + 1)) / 8.0 +
	//	(generator(x - 1, y) + generator(x, y - 1) + generator(x + 1, y) + generator(x, y + 1)) / 10.0 +
	//	generator(x, y) / 6.0;
	return generator(x, y);
}

//could optimize by only calculating blending factor for fractX once
double ValueGenerator::Interpolate(double a, double b, double blend)
{
	double cosBlend = (1.0 - cos(blend * 3.141592653589793)) * 0.5;
	return a * (1.0 - cosBlend) + b * cosBlend;
}