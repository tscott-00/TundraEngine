#pragma once

#include "general_common.h"

#include <vector>
#include <iostream>
#include <string>

#include <GL/glew.h>
#include <SDL2/SDL.h>

#pragma region

#pragma endregion Schematics

#pragma region

#pragma endregion I/O

#pragma region

#pragma endregion User Input

class Window
{
public:
	Window(const std::string& title);
	virtual ~Window();

	void Update();
private:
	SDL_Window* window;
	SDL_GLContext glContext;

#if defined _DEBUG
	static void GLAPIENTRY MessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
		const GLchar* message,
		const void* userParam)
	{
		std::string typeStr;
		switch (type)
		{
		case GL_DEBUG_TYPE_ERROR:
		{
			typeStr = "Error";
			break;
		}
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
		{
			typeStr = "Deprecated";
			break;
		}
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
		{
			typeStr = "Undefined";
			break;
		}
		case GL_DEBUG_TYPE_PORTABILITY:
		{
			typeStr = "Portability";
			break;
		}
		case GL_DEBUG_TYPE_PERFORMANCE:
		{
			typeStr = "Performance";
			break;
		}
		case GL_DEBUG_TYPE_MARKER:
		{
			typeStr = "Marker";
			break;
		}
		case GL_DEBUG_TYPE_PUSH_GROUP:
		{
			typeStr = "Push";
			break;
		}
		case GL_DEBUG_TYPE_POP_GROUP:
		{
			typeStr = "Pop";
			break;
		}
		case GL_DEBUG_TYPE_OTHER:
		{
			typeStr = "Other";
			break;
		}
		default:
			typeStr = "Unkown";
			break;
		}

		std::string severityStr;
		switch (severity)
		{
		case GL_DEBUG_SEVERITY_LOW:
		{
			severityStr = "Low";
			break;
		}
		case GL_DEBUG_SEVERITY_MEDIUM:
		{
			severityStr = "Medium";
			break;
		}
		case GL_DEBUG_SEVERITY_HIGH:
		{
			severityStr = "High";
			break;
		}
		case GL_DEBUG_SEVERITY_NOTIFICATION:
		{
			severityStr = "Notification";
			break;
		}
		default:
			severityStr = "Unkown";
			break;
		}

		std::cerr << "OpenGL Message:\n-----------\nType:" << typeStr << "\nSeverity:" << severityStr << "\n\n\"" << message << "\"\n-----------\n\n";
		//printf("GL CALLBACK: %s\nType: 0x%x\nSeverity: 0x%x, Message: %s\n", (type == GL_TYPE_ERROR ? "** GL ERROR **" : ""), typeStr, severityStr, message);
	}
#endif
};