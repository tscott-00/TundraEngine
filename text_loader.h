#pragma once

#include <string>
#include <vector>

struct FloatData {
	FloatData(float* floats, unsigned int count, const std::string& tag);

	std::string print();

	float* floats;
	unsigned int count;
	std::string tag;
};

struct UIntData {
	UIntData(unsigned short* ints, unsigned int count, const std::string& tag);

	std::string print();

	unsigned short* ints;
	unsigned int count;
	std::string tag;
};

class TextData {
public:
	TextData(const std::string& fileName);
	~TextData();

	FloatData GetFloats(const std::string& tag) const;
	UIntData GetUInts(const std::string& tag) const;
private:
	std::vector<FloatData> floatData;
	std::vector<UIntData> uintData;
};
