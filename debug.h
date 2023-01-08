#pragma once

#include <string>

namespace Console {
	enum QuitMode {
		DO_NOT_QUIT,
		UNSAFE_QUIT,
		FORCE_QUIT
	};

	// All can be output to real console, or to a general stream
	// Log() and Error() are recorded and can be written to a log later (./Logs/)
	// FatalError() shows error popup and saves everything to a file
	// Error() and FatalError save after every call, Log() saves when SaveLogs() is called or when Error() or FatalError() are called
	// Debug isn't recorded
	void Debug(const std::string& message);
	void Log(const std::string& message);
	void Error(const std::string& message);
	void FatalError(const std::string& message, QuitMode quitMode = DO_NOT_QUIT);

	void SaveLogs();
}
