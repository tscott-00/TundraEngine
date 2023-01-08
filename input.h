#pragma once

#include <SDL2/SDL.h>

namespace Input {
	const unsigned int LEFT_MOUSE_BUTTON = 0;
	const unsigned int MIDDLE_MOUSE_BUTTON = 1;
	const unsigned int RIGHT_MOUSE_BUTTON = 2;

	bool IsButtonDown(unsigned int button);
	bool WasButtonPressed(unsigned int button);
	bool WasButtonReleased(unsigned int button);

	// TODO: Need keycode IsKeyDown
	bool IsKeyDown(SDL_Scancode key);
	bool WasKeyPressed(SDL_Scancode key);
	bool WasKeyReleased(SDL_Scancode key);
}
