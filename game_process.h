#pragma once
namespace Internal
{
	struct GameProcess
	{
		enum Priority
		{
			EARLY,
			LATE,

			PRIORITIES_COUNT
		} const priority;
		void(*function)();

		GameProcess(void(*function)(), Priority priority);
		virtual ~GameProcess();

		void Enable();
		void Disable();
	private:
		bool isEnabled;
	};

	void RunGameProcesses(GameProcess::Priority priorityLayer);
}