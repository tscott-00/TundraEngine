#include "shader.h"

#include <iostream>
#include <fstream>
#include <unordered_map>
#include <sys/stat.h>

#include "debug.h"

std::unordered_map<std::string, std::shared_ptr<Shader::Box>> Shader::registry;

Shader::Shader(const std::string& fileName) {
	auto entry = registry.find(fileName);
	if (entry != registry.end()) {
		box = entry->second;
		return;
	}
	GLint program = glCreateProgram();
	box = std::shared_ptr<Box>(new Box(program));
	registry[fileName] = box;

	struct stat buffer;
	std::string computeShaderFile = "./Resources/" + fileName + ".cs";
	if (stat(computeShaderFile.c_str(), &buffer) == 0) {
		GLuint compute = CreateShader(fileName, LoadShader(computeShaderFile), GL_COMPUTE_SHADER);

		glAttachShader(program, compute);

		glLinkProgram(program);
		CheckShaderError(program, GL_LINK_STATUS, true, "Error: Program linking failed");

		glValidateProgram(program);
		CheckShaderError(program, GL_VALIDATE_STATUS, true, "Error: Program is invalid");

		glDetachShader(program, compute);

		glDeleteShader(compute);
	} else {
		std::string vertexShaderFile = "./Resources/" + fileName + ".vs";
		std::string tesselationControlShaderFile = "./Resources/" + fileName + ".tcs";
		std::string tesselationEvaluationShaderFile = "./Resources/" + fileName + ".tes";
		std::string geometryShaderFile = "./Resources/" + fileName + ".gs";
		std::string fragmentShaderFile = "./Resources/" + fileName + ".fs";
		bool hasTesselationControlShader = stat(tesselationControlShaderFile.c_str(), &buffer) == 0;
		bool hasTesselationEvaluationShader = stat(tesselationEvaluationShaderFile.c_str(), &buffer) == 0;
		bool hasGeometryShader = stat(geometryShaderFile.c_str(), &buffer) == 0;

		GLuint vert = CreateShader(fileName, LoadShader(vertexShaderFile), GL_VERTEX_SHADER);
		GLuint tcs = 0;
		if (hasTesselationControlShader)
			tcs = CreateShader(fileName, LoadShader(tesselationControlShaderFile), GL_TESS_CONTROL_SHADER);
		GLuint tes = 0;
		if (hasTesselationEvaluationShader)
			tes = CreateShader(fileName, LoadShader(tesselationEvaluationShaderFile), GL_TESS_EVALUATION_SHADER);
		GLuint geom = 0;
		if (hasGeometryShader)
			geom = CreateShader(fileName, LoadShader(geometryShaderFile), GL_GEOMETRY_SHADER);
		GLuint frag = CreateShader(fileName, LoadShader(fragmentShaderFile), GL_FRAGMENT_SHADER);

		glAttachShader(program, vert);
		if (hasTesselationControlShader)
			glAttachShader(program, tcs);
		if (hasTesselationEvaluationShader)
			glAttachShader(program, tes);
		if (hasGeometryShader)
			glAttachShader(program, geom);
		glAttachShader(program, frag);

		glLinkProgram(program);
		CheckShaderError(program, GL_LINK_STATUS, true, "Error: Program linking failed");

		glValidateProgram(program);
		CheckShaderError(program, GL_VALIDATE_STATUS, true, "Error: Program is invalid");

		glDetachShader(program, vert);
		if (hasTesselationControlShader)
			glDetachShader(program, tcs);
		if (hasTesselationEvaluationShader)
			glDetachShader(program, tes);
		if (hasGeometryShader)
			glDetachShader(program, geom);
		glDetachShader(program, frag);

		glDeleteShader(vert);
		if (hasTesselationControlShader)
			glDeleteShader(tcs);
		if (hasTesselationEvaluationShader)
			glDeleteShader(tes);
		if (hasGeometryShader)
			glDeleteShader(geom);
		glDeleteShader(frag);
	}
}

Shader::Shader(const std::string& vertexText, const std::string& fragmentText) {
	GLint program = glCreateProgram();
	box = std::shared_ptr<Box>(new Box(program));

	GLuint vert = CreateShader("custom " + vertexText, vertexText, GL_VERTEX_SHADER);
	GLuint frag = CreateShader("custom " + fragmentText, fragmentText, GL_FRAGMENT_SHADER);

	glAttachShader(program, vert);
	glAttachShader(program, frag);

	glLinkProgram(program);
	CheckShaderError(program, GL_LINK_STATUS, true, "Error: Program linking failed");

	glValidateProgram(program);
	CheckShaderError(program, GL_VALIDATE_STATUS, true, "Error: Program is invalid");

	glDetachShader(program, vert);
	glDetachShader(program, frag);

	glDeleteShader(vert);
	glDeleteShader(frag);
}

Shader::Shader(const Shader& other) :
	box(other.box) { }

void Shader::operator=(const Shader& other) {
	box = other.box;
}

GLuint Shader::CreateShader(const std::string& shaderName, const std::string& text, GLenum shaderType) {
	GLuint shader = glCreateShader(shaderType);

	if (shader == 0)
		std::cerr << "Error: Shader creation failed" << std::endl;
	const GLchar* shaderSourceStrings[1];
	GLint shaderSourceStringLengths[1];
	shaderSourceStrings[0] = text.c_str();
	shaderSourceStringLengths[0] = static_cast<GLint>(text.length());

	glShaderSource(shader, 1, shaderSourceStrings, shaderSourceStringLengths);
	glCompileShader(shader);

	CheckShaderError(shader, GL_COMPILE_STATUS, false, "Error: Shader compilation failed for " + shaderName);

	return shader;
}

namespace {
	bool StartsWith(const std::string& str, const std::string& smallString) {
		if (str.size() < smallString.size())
			return false;

		for (size_t i = 0; i < smallString.size(); ++i)
			if (str[i] != smallString[i])
				return false;

		return true;
	}

	std::string GetAfterSpace(const std::string& str) {
		for (size_t i = 0; i < str.size(); ++i)
			if (str[i] == ' ')
				return str.substr(i + 1, str.size());

		return "NO INCLUDE FILE SPECIFIED";
	}

	void LoadLibrary(const std::string& fileName, std::vector<std::string>& loadedLibraries, std::string& output) {
		for (const std::string& other : loadedLibraries)
			if (other == fileName)
				return;
		loadedLibraries.push_back(fileName);

		std::ifstream file;
		file.open(fileName.c_str());

		std::string line;

		if (file.is_open()) {
			while (file.good()) {
				getline(file, line);
				if (StartsWith(line, "#include"))
					LoadLibrary("./resources/" + GetAfterSpace(line), loadedLibraries, output);
				else
					output.append(line + "\n");
			}
		} else {
			std::cerr << "Unable to load shader: " << fileName << std::endl;
		}
	}
}

std::string Shader::LoadShader(const std::string& fileName) {
	std::ifstream file;
	file.open(fileName.c_str());

	std::string output;
	std::string line;

	std::vector<std::string> loadedLibraries;
	if (file.is_open()) {
		while (file.good()) {
			getline(file, line);
			if (StartsWith(line, "#include"))
				LoadLibrary("./resources/" + GetAfterSpace(line), loadedLibraries, output);
			else
				output.append(line + "\n");
		}
	} else {
		std::cerr << "Unable to load shader: " << fileName << std::endl;
	}

	return output;
}

void Shader::CheckShaderError(GLuint shader, GLuint flag, bool isProgram, const std::string& errorMessage) {
	GLint success = 0;
	GLchar error[1024] = { 0 };

	if (isProgram)
		glGetProgramiv(shader, flag, &success);
	else
		glGetShaderiv(shader, flag, &success);

	if (success == GL_FALSE) {
		if (isProgram)
			glGetProgramInfoLog(shader, sizeof(error), nullptr, error);
		else
			glGetShaderInfoLog(shader, sizeof(error), nullptr, error);
		Console::Error(errorMessage + ": " + error);
	}
}
