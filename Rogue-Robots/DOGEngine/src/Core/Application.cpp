#include "Application.h"
#include "../Input/Mouse.h"
#include "../Input/Keyboard.h"
namespace DOG
{
	bool ApplicationManager::s_shouldRestart{ false };
	bool ApplicationManager::s_initialStartupDone{ false };

	const bool ApplicationManager::ShouldRestartApplication() noexcept
	{
		bool toReturn = s_shouldRestart || !s_initialStartupDone;
		s_initialStartupDone = true;
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
			if (!Window::OnUpdate())
				m_IsRunning = false;

			Mouse::Reset();
		}
	}

	void Application::OnRestart() noexcept
	{
		//...
	}

	void Application::OnStartUp() noexcept
	{
		Window::Initialize(m_Specification);
	}

	void Application::OnShutDown() noexcept
	{
		//...
	}
}
