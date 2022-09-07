#include "Application.h"

namespace DOG
{
	bool ApplicationManager::s_ShouldRestart{ false };
	bool ApplicationManager::s_InitialStartupDone{ false };

	const bool ApplicationManager::ShouldRestartApplication() noexcept
	{
		bool toReturn = s_ShouldRestart || !s_InitialStartupDone;
		s_InitialStartupDone = true;
		return toReturn;
	}

	Application::Application(const ApplicationSpecification& spec) noexcept
		: m_Specification{spec}, m_IsRunning{true}
	{
		OnStartUp();
	}

	Application::~Application() noexcept
	{
		OnShutDown();
	}

	void Application::Run() noexcept
	{
		while (m_IsRunning)
		{
			//...
		}
	}

	void Application::OnRestart() noexcept
	{
		//...
	}

	void Application::OnStartUp() noexcept
	{
		//...
	}

	void Application::OnShutDown() noexcept
	{
		//...
	}
}
