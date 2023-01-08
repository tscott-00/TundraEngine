#pragma once

#include <string>
#include <SDL2/SDL.h>

class Window {
public:
	Window(const std::string& title, const std::string& iconDir = "");
	virtual ~Window();

	void Update();
private:
	SDL_Window* window;
	SDL_GLContext glContext;
};