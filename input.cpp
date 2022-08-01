#include "input.h"
#include "events_state.h"

namespace Input
{
	bool IsButtonDown(unsigned int button)
	{
		return EventsState::mouseButtonStates[button];
	}

	bool WasButtonPressed(unsigned int button)
	{
		return EventsState::pressedMouseButtons[button];
	}

	bool WasButtonReleased(unsigned int button)
	{
		return EventsState::releasedMouseButtons[button];
	}

	bool IsKeyDown(SDL_Scancode key)
	{
		return EventsState::keyStates[key];
	}

	bool WasKeyPressed(SDL_Scancode key)
	{
		for (SDL_Scancode pressedKey : EventsState::pressedKeys)
			if (key == pressedKey)
				return true;
		
		return false;
	}

	bool WasKeyReleased(SDL_Scancode key)
	{
		for (SDL_Scancode releasedKey : EventsState::releasedKeys)
			if (key == releasedKey)
				return true;

		return false;
	}
}