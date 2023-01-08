#pragma once

#include "general_common.h"

#include <string>
#include <unordered_map>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <memory>

#include "game_math.h"

class Shader
{
public:
	struct Box
	{
		GLuint name;

		Box(GLuint name)
		{
			this->name = name;
		}

		~Box()
		{
			glDeleteProgram(name);
		}
	};
protected:
	static std::unordered_map<std::string, std::shared_ptr<Shader::Box>> registry;

	std::shared_ptr<Box> box;
public:
	Shader(const std::string& fileName);
	Shader(const std::string& vertexText, const std::string& fragmentText);
	Shader() { }

	Shader(const Shader& other);
	void operator=(const Shader& other);

	inline void Bind() const {
		glUseProgram(box->name);
	}

	inline GLint rtc_GetName() const {
		return box->name;
	}

	// TODO: Faster to use v functions?

	static inline void LoadMatrix4x4(int location, const FMatrix4x4& mat) {
		glUniformMatrix4fv(location, 1, GL_FALSE, &mat[0][0]);
	}

	static inline void LoadTransposeMatrix4x4(int location, const FMatrix4x4& mat) {
		glUniformMatrix4fv(location, 1, GL_TRUE, &mat[0][0]);
	}

	static inline void LoadMatrix3x3(int location, const FMatrix3x3& mat) {
		glUniformMatrix3fv(location, 1, GL_FALSE, &mat[0][0]);
	}

	static inline void LoadTransposeMatrix3x3(int location, const FMatrix3x3& mat) {
		glUniformMatrix3fv(location, 1, GL_TRUE, &mat[0][0]);
	}

	static inline void LoadVector4(unsigned int location, const FVector4& vec) {
		glUniform4fv(location, 1, &vec.x);
	}

	static inline void LoadVector3(unsigned int location, const FVector3& vec) {
		glUniform3fv(location, 1, &vec.x);
	}

	static inline void LoadVector2(unsigned int location, const FVector2& vec) {
		glUniform2fv(location, 1, &vec.x);
	}

	static inline void LoadVector2(unsigned int location, const IVector2& vec) {
		glUniform2iv(location, 1, &vec.x);
	}

	static inline void LoadVector4(unsigned int location, const float* vec) {
		glUniform4fv(location, 1, vec);
	}

	static inline void LoadVector3(unsigned int location, const float* vec) {
		glUniform3fv(location, 1, vec);
	}

	static inline void LoadVector2(unsigned int location, const float* vec) {
		glUniform2fv(location, 1, vec);
	}

	static inline void LoadInt(unsigned int location, int i) {
		glUniform1i(location, i);
	}

	static inline void LoadFloat(unsigned int location, float f) {
		glUniform1f(location, f);
	}

	static inline void LoadVector2Array(unsigned int location, const FVector2* vecs, unsigned int vecCount) {
		glUniform2fv(location, vecCount, &vecs[0].x);
	}
	
	static std::string LoadShader(const std::string& fileName);
private:
	void CheckShaderError(GLuint shader, GLuint flag, bool isProgram, const std::string& errorMessage);
	GLuint CreateShader(const std::string& shaderName, const std::string& text, GLenum shaderType);
};
