#include "fbo.h"

#include "events_state.h"
#include "debug.h"

FBO::FBO(Texture2D* textures, unsigned int textureCount, unsigned int width, unsigned int height, bool hasDepthBuffer)
{
	depthBuffer = 0;
	depthStencilBuffer = 0;
	fbo = 0;
	Reload(textures, textureCount, width, height, hasDepthBuffer);
}

FBO::FBO()
{
	textures = nullptr;
	textureCount = 0;
	depthBuffer = 0;
	depthStencilBuffer = 0;
	fbo = 0;
	width = EventsState::windowWidth;
	height = EventsState::windowHeight;
}

void FBO::Reload(Texture2D* textures, unsigned int textureCount, unsigned int width, unsigned int height, bool hasDepthBuffer)
{
	if (textures == nullptr)
	{
		if (fbo != 0)
		{
			if (textureCount > 0)
				delete[] this->textures;
			if (depthBuffer != 0)
				glDeleteRenderbuffers(1, &depthBuffer);
			if (depthStencilBuffer != 0)
				glDeleteRenderbuffers(1, &depthStencilBuffer);
			glDeleteFramebuffers(1, &fbo);

			this->textures = nullptr;
			this->textureCount = 0;
			depthBuffer = 0;
			depthStencilBuffer = 0;
			fbo = 0;
			this->width = EventsState::windowWidth;
			this->height = EventsState::windowHeight;
		}
		return;
	}

	if (fbo != 0)
	{
		if (textureCount > 0)
			delete[] this->textures;
		if (depthBuffer != 0)
			glDeleteRenderbuffers(1, &depthBuffer);
		if (depthStencilBuffer != 0)
			glDeleteRenderbuffers(1, &depthStencilBuffer);
		glDeleteFramebuffers(1, &fbo);
	}

	this->width = width;
	this->height = height;
	this->textureCount = textureCount;

	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	this->textures = new Texture2D[textureCount];
	GLenum* drawBuffers = new GLenum[textureCount];
	for (unsigned int i = 0; i < textureCount; ++i)
	{
		this->textures[i] = textures[i];
		this->textures[i].Bind();
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, this->textures[i].rtc_GetID(), 0);
		drawBuffers[i] = GL_COLOR_ATTACHMENT0 + i;
	}

	glDrawBuffers(textureCount, drawBuffers);
	delete[] drawBuffers;

	if (hasDepthBuffer)
	{
		glGenRenderbuffers(1, &depthBuffer);
		glBindRenderbuffer(GL_RENDERBUFFER, depthBuffer);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuffer);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void FBO::MakeLone(unsigned int width, unsigned int height, bool hasDepthBuffer)
{
	if (fbo != 0)
	{
		if (textureCount > 0)
			delete[] textures;
		if (depthBuffer != 0)
			glDeleteRenderbuffers(1, &depthBuffer);
		if (depthStencilBuffer != 0)
			glDeleteRenderbuffers(1, &depthStencilBuffer);
		glDeleteFramebuffers(1, &fbo);
	}

	this->width = width;
	this->height = height;
	textureCount = 0;
	depthStencilBuffer = 0;
	depthBuffer = 0;
	textures = nullptr;

	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	if (hasDepthBuffer)
	{
		glGenRenderbuffers(1, &depthBuffer);
		glBindRenderbuffer(GL_RENDERBUFFER, depthBuffer);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuffer);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

FBO::~FBO()
{
	if (fbo != 0)
	{
		if (textureCount > 0)
			delete[] textures;
		if (depthBuffer != 0)
			glDeleteRenderbuffers(1, &depthBuffer);
		if (depthStencilBuffer != 0)
			glDeleteRenderbuffers(1, &depthStencilBuffer);
		glDeleteFramebuffers(1, &fbo);
	}
}

void FBO::Bind() const
{
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glViewport(0, 0, width, height);
}

void FBO::Unbind() const
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, EventsState::windowWidth, EventsState::windowHeight);
}

void FBO::AttachDepthTexture()
{
	depthTexture = Texture2D(Texture2D::Descriptor(Texture2D::Descriptor::Image(width, height, GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT, GL_FLOAT), true, false, GL_CLAMP_TO_EDGE));
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture.rtc_GetID(), 0);
}

void FBO::AttachDepthStencilTexture()
{
	depthStencilTexture = Texture2D(Texture2D::Descriptor(Texture2D::Descriptor::Image(width, height, GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8), true, false, GL_CLAMP_TO_EDGE));
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, depthStencilTexture.rtc_GetID(), 0);
}

void FBO::AttachDepthBuffer()
{
	if (depthBuffer != 0)
		glDeleteRenderbuffers(1, &depthBuffer);

	glGenRenderbuffers(1, &depthBuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, depthBuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuffer);
}

void FBO::AttachDepthStencilBuffer()
{
	if (depthStencilBuffer != 0)
		glDeleteRenderbuffers(1, &depthStencilBuffer);

	glGenRenderbuffers(1, &depthStencilBuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, depthStencilBuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depthStencilBuffer);
}