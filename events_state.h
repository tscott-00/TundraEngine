#pragma once

#include <SDL2/SDL.h>
#include <vector>

#include "game_math.h"

class EventsState {
public:
	// TODO: Most should be private with Get and some Set
	static bool isProgramActive;
	static const uint8_t* internalKeyStates;
	static uint8_t* keyStates;
	static std::vector<SDL_Scancode> pressedKeys;
	static std::vector<SDL_Keycode > pressedKeyCodes;
	static std::vector<SDL_Scancode> releasedKeys;
	static bool mouseButtonStates[3];
	static bool pressedMouseButtons[3];
	static bool releasedMouseButtons[3];
	static int mouseX, mouseY;
	static int mouseDX, mouseDY;
	static int mouseDM;
	static int windowWidth, windowHeight;
	static unsigned int cursorFreeLockCount;

	static inline bool IsCursorFree() {
		return cursorFreeLockCount > 0;
	}

	static inline bool IsCursorLocked() {
		return cursorFreeLockCount == 0;
	}

	static inline void AddCursorFreeLock() {
		if (cursorFreeLockCount == 0)
			SDL_SetRelativeMouseMode(SDL_FALSE);

		++cursorFreeLockCount;
	}

	static inline void FreeCursorFreeLock() {
		if (cursorFreeLockCount == 0)
			return;

		--cursorFreeLockCount;

		if (cursorFreeLockCount == 0)
			SDL_SetRelativeMouseMode(SDL_TRUE);
	}

	static inline float GetAspectRatio() {
		return static_cast<float>(windowWidth) / static_cast<float>(windowHeight);
	}

	static inline FVector2 GetGLSize(int pixelSizeX, int pixelSizeY) {
		return FVector2(pixelSizeX / static_cast<float>(windowHeight), pixelSizeY / static_cast<float>(windowHeight));
	}

	static void Init(int windowWidth, int windowHeight);

	static void Update();

	static void Clear();
};
