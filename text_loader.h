#pragma once

#include <string>
#include <vector>

struct FloatData
{
	FloatData(float* floats, unsigned int count, std::string tag)
	{
		this->floats = floats;
		this->count = count;
		this->tag = tag;
	}

	std::string print()
	{
		std::string str = "";
		for (unsigned int i = 0; i < count; i++)
			str += std::to_string(floats[i]) + " ";
		return str;
	}

	float* floats;
	unsigned int count;
	std::string tag;
};

struct UIntData
{
	UIntData(unsigned short* ints, unsigned int count, std::string tag)
	{
		this->ints = ints;
		this->count = count;
		this->tag = tag;
	}

	std::string print()
	{
		std::string str = "";
		for (unsigned int i = 0; i < count; i++)
			str += std::to_string(ints[i]) + " ";
		return str;
	}

	unsigned short* ints;
	unsigned int count;
	std::string tag;
};

class TextData
{
public:
	TextData(const std::string& fileName);
	~TextData();

	FloatData GetFloats(const std::string& tag) const;
	UIntData GetUInts(const std::string& tag) const;
private:
	std::vector<FloatData> floatData;
	std::vector<UIntData> uintData;
};