#include "game_time.h"

GameTime::GameTime()
{
	lastTime = std::chrono::high_resolution_clock::now();
	deltaTime = 0.0f;
}

void GameTime::Update()
{
	auto thisTime = std::chrono::high_resolution_clock::now();
	deltaTime = std::chrono::duration_cast<std::chrono::duration<float>>(thisTime - lastTime).count();
	lastTime = thisTime;
}