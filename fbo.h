#pragma once

#include "general_common.h"

#include <GL/glew.h>

#include "texture.h"

class FBO
{
private:
	Texture2D* textures;
	Texture2D depthTexture;
	Texture2D depthStencilTexture;

	GLuint depthBuffer;
	GLuint depthStencilBuffer;
	GLuint fbo;
public:
	// TODO: Remove depth argument
	FBO(Texture2D* textures, unsigned int textureCount, unsigned int width, unsigned int height, bool hasDepthBuffer = false);
	FBO();

	void Reload(Texture2D* textures, unsigned int textureCount, unsigned int width, unsigned int height, bool hasDepthBuffer = false);
	void MakeLone(unsigned int width, unsigned int height, bool hasDepthBuffer = false);
	//void Reload(unsigned int textureCount, bool hasDepthBuffer, unsigned int width, unsigned int height, GLint internalFormat, GLint format);

	virtual ~FBO();

	void Bind() const;
	void Unbind() const;

	inline const Texture2D& GetTexture(unsigned int index) const
	{
		return textures[index];
	}

	inline const Texture2D& GetDepthTexture() const
	{
		return depthTexture;
	}

	inline const Texture2D& GetDepthStencilTexture() const
	{
		return depthStencilTexture;
	}

	void AttachDepthTexture();
	void AttachDepthStencilTexture();
	void AttachDepthBuffer();
	void AttachDepthStencilBuffer();

	inline void DrawBuffers(GLenum* buffers, int count)
	{
		glDrawBuffers(count, buffers);
	}

	inline void DrawBuffers(GLenum buffer)
	{
		glDrawBuffers(1, &buffer);
	}

	inline GLuint rtc_GetID() const
	{
		return fbo;
	}

	inline GLuint rtc_GetDepthBuffer() const
	{
		return depthBuffer;
	}

	inline const Texture2D& rtc_GetDepthTexture() const
	{
		return depthTexture;
	}

	inline GLuint rtc_GetDepthStencilBuffer() const
	{
		return depthStencilBuffer;
	}

	inline const Texture2D& rtc_GetDepthStencilTexture() const
	{
		return depthStencilTexture;
	}

	no_transfer_functions(FBO)
private:
	int textureCount;

	int width;
	int height;
};