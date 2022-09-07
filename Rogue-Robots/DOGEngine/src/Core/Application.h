#pragma once
#include "Window.h"
namespace DOG
{
	struct ApplicationSpecification
	{
		std::string Name;
		DirectX::XMFLOAT2 WindowDimensions;
		WindowMode InitialWindowMode;
	};

	class ApplicationManager
	{
	public:
		static [[nodiscard]] const bool ShouldRestartApplication() noexcept;
	private:
		static bool s_ShouldRestart;
		static bool s_InitialStartupDone;
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
		ApplicationSpecification m_Specification;
		bool m_IsRunning;
	};
}

[[nodiscard]] std::unique_ptr<DOG::Application> CreateApplication() noexcept;
