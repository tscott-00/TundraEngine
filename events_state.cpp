#include "events_state.h"

bool EventsState::isProgramActive = true;
const uint8_t* EventsState::internalKeyStates;
uint8_t* EventsState::keyStates;
std::vector<SDL_Scancode> EventsState::pressedKeys;
std::vector<SDL_Keycode > EventsState::pressedKeyCodes;
std::vector<SDL_Scancode> EventsState::releasedKeys;
bool EventsState::mouseButtonStates[3] = { false, false, false };
bool EventsState::pressedMouseButtons[3] = { false, false, false };
bool EventsState::releasedMouseButtons[3] = { false, false, false };
int EventsState::mouseX = 0;
int EventsState::mouseY = 0;
int EventsState::mouseDX = 0;
int EventsState::mouseDY = 0;
int EventsState::mouseDM = 0;
int EventsState::windowWidth = 0;
int EventsState::windowHeight = 0;
unsigned int EventsState::cursorFreeLockCount;

void EventsState::Init(int windowWidth, int windowHeight) {
	EventsState::windowWidth = windowWidth;
	EventsState::windowHeight = windowHeight;
	internalKeyStates = SDL_GetKeyboardState(nullptr);

	keyStates = new uint8_t[SDL_NUM_SCANCODES];
	for (unsigned int i = 0; i < SDL_NUM_SCANCODES; ++i)
		keyStates[i] = 0;
}

void EventsState::Clear() {
	delete[] keyStates;
}

void EventsState::Update() {
	mouseDX = 0;
	mouseDY = 0;
	mouseDM = 0;
	pressedMouseButtons[0] = false;
	pressedMouseButtons[1] = false;
	pressedMouseButtons[2] = false;
	releasedMouseButtons[0] = false;
	releasedMouseButtons[1] = false;
	releasedMouseButtons[2] = false;
	pressedKeys.clear();
	pressedKeyCodes.clear();
	releasedKeys.clear();
	SDL_Event e;
	// WARNING: button < 3 may not be necessary
	while (SDL_PollEvent(&e)) {
		switch (e.type) {
		case SDL_MOUSEMOTION:
			mouseDX += e.motion.xrel;
			mouseDY += e.motion.yrel;
			break;
		case SDL_MOUSEWHEEL:
			mouseDM += e.wheel.y;
			break;
		case SDL_MOUSEBUTTONDOWN:
		{
			uint8_t button = e.button.button - 1;
			if (button <= 2) { 
				mouseButtonStates[button] = true;
				pressedMouseButtons[button] = true;
			}
			break;
		}
		case SDL_MOUSEBUTTONUP:
		{
			uint8_t button = e.button.button - 1;
			if (button <= 2) {
				mouseButtonStates[button] = false;
				releasedMouseButtons[button] = true;
			}
			break;
		}
		case SDL_KEYDOWN:
			if (!keyStates[e.key.keysym.scancode])
				pressedKeys.push_back(e.key.keysym.scancode);
			pressedKeyCodes.push_back(e.key.keysym.sym);
			break;
		case SDL_KEYUP:
			releasedKeys.push_back(e.key.keysym.scancode);
			break;
		case SDL_QUIT:
			isProgramActive = false;
			break;
		default:
			break;
		}
	}
	SDL_GetMouseState(&mouseX, &mouseY);
	// TODO: Whole array does not need to be copied
	memcpy(keyStates, internalKeyStates, SDL_NUM_SCANCODES);
}
