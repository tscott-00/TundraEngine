#pragma once

#include <chrono>

class GameTime
{
public:
	float deltaTime;
	float totalTime;

	GameTime();

	void Update();
private:
	std::chrono::_V2::system_clock::time_point lastTime;
};

