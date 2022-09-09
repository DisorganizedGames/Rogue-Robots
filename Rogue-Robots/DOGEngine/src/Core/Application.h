#pragma once
#include "Window.h"
namespace DOG
{
	struct ApplicationSpecification
	{
		std::string name;
		Vector2u windowDimensions;
		WindowMode initialWindowMode;
	};

	class ApplicationManager
	{
	public:
		static [[nodiscard]] const bool ShouldRestartApplication() noexcept;
	private:
		static bool s_shouldRestart;
		static bool s_initialStartupDone;
	};

	class Application
	{
	public:
		explicit Application(const ApplicationSpecification& spec) noexcept;
		virtual ~Application() noexcept;
		void Run() noexcept;
		void OnRestart() noexcept;
	private:
		void OnStartUp() noexcept;
		void OnShutDown() noexcept;
	private:
		ApplicationSpecification m_specification;
		bool m_isRunning;
	};
}

[[nodiscard]] std::unique_ptr<DOG::Application> CreateApplication() noexcept;
