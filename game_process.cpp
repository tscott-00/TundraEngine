#include "game_process.h"
#include <list>

namespace {
	std::list<Internal::GameProcess*> processes[Internal::GameProcess::PRIORITIES_COUNT];
}

namespace Internal {
	GameProcess::GameProcess(void(*function)(), Priority priority) : function(function), priority(priority) {
		isEnabled = false;
	}

	GameProcess::~GameProcess() {
		if (isEnabled)
			processes[priority].remove(this);
	}

	void GameProcess::Enable() {
		if (!isEnabled)
			processes[priority].push_back(this);
	}

	void GameProcess::Disable() {
		if (isEnabled)
			processes[priority].remove(this);
	}

	void RunGameProcesses(GameProcess::Priority priorityLayer) {
		for (GameProcess* process : processes[priorityLayer])
			process->function();
	}
}
