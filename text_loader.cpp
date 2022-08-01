#include "text_loader.h"
#include <fstream>
#include <iostream>

static inline std::vector<std::string> SplitString(const std::string &s, char delim);

TextData::TextData(const std::string& fileName)
{
	std::string fullFileName = "./Resources/" + fileName + ".tvd";
	std::ifstream file;
	file.open(fullFileName.c_str());

	std::string line;
	if (file.is_open())
	{
		while (file.good())
		{
			getline(file, line);

			if (line.length() < 2)
				continue;

			std::vector<std::string> parts = SplitString(line, ' ');
			if (parts[0] == "float")
			{
				unsigned int floatCount = static_cast<unsigned int>(parts.size() - 2);
				float* floats = new float[floatCount];
				for (unsigned int i = 0; i < floatCount; ++i)
					floats[i] = std::stof(parts[i + 2], nullptr);
				floatData.push_back(FloatData(floats, floatCount, parts[1]));
			}
			else if (parts[0] == "uint")
			{
				unsigned int intCount = static_cast<unsigned int>(parts.size() - 2);
				unsigned short* ints = new unsigned short[intCount];
				for (unsigned int i = 0; i < intCount; ++i)
					ints[i] = static_cast<unsigned short>(std::stoul(parts[i + 2], nullptr, 10));
				uintData.push_back(UIntData(ints, intCount, parts[1]));
			}
			else
			{
				std::cerr << "Unknown data type \"" << parts[0] << "\", skipping entry..." << std::endl;
			}
		}
	}
	else
	{
		std::cerr << "Unable to load mesh: " << fullFileName << std::endl;
	}
}

TextData::~TextData()
{
	for (unsigned int i = 0; i < floatData.size(); ++i)
		delete[] floatData[i].floats;
	for (unsigned int i = 0; i < uintData.size(); ++i)
		delete[] uintData[i].ints;
}

FloatData TextData::GetFloats(const std::string& tag) const
{
	for (unsigned int i = 0; i < floatData.size(); ++i)
		if (floatData[i].tag == tag)
			return floatData[i];
	//std::cerr << "No float array with tag " << tag << " found!" << std::endl;
	return FloatData(nullptr, 0, "nullptr");
}

UIntData TextData::GetUInts(const std::string& tag) const
{
	for (unsigned int i = 0; i < uintData.size(); ++i)
		if (uintData[i].tag == tag)
			return uintData[i];
	//std::cerr << "No int array with tag " << tag << " found!" << std::endl;
	return UIntData(nullptr, 0, "nullptr");
}

static inline std::vector<std::string> SplitString(const std::string &s, char delim)
{
	std::vector<std::string> elems;

	const char* cstr = s.c_str();
	size_t strLength = s.length();
	size_t start = 0;
	size_t end = 0;

	while (end <= strLength)
	{
		while (end <= strLength)
		{
			if (cstr[end] == delim)
				break;
			end++;
		}

		elems.push_back(s.substr(start, end - start));
		start = end + 1;
		end = start;
	}

	return elems;
}