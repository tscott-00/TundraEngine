#include "debug.h"

#include <iostream>
#include "events_state.h"

namespace Console
{
	void Debug(const std::string& message)
	{
		std::cerr << message.c_str() << std::endl;
	}

	void Log(const std::string& message)
	{
		std::cerr << message.c_str() << std::endl;
	}

	void Error(const std::string& message)
	{
		std::cerr << message.c_str() << std::endl;
	}

	void FatalError(const std::string& message, QuitMode quitMode)
	{
		Error(message);

		switch (quitMode)
		{
		case DO_NOT_QUIT:
			break;
		case UNSAFE_QUIT:
		{
			EventsState::isProgramActive = false;
			Log("Attempting regular quit operations after fatal error...");
			break;
		}
		case FORCE_QUIT:
		{
			exit(1);
			break;
		}
		default:
		{
			Error("Unrecognized quit mode: " + quitMode);
			break;
		}
		}
		EventsState::isProgramActive = false;
	}

	//void SaveLogs();
}