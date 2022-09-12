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
		: m_specification{spec}, m_isRunning{true}
	{
		OnStartUp();
	}

	Application::~Application() noexcept
	{
		OnShutDown();
	}

	void Application::Run() noexcept
	{
		while (m_isRunning)
		{
			if (!Window::OnUpdate())
				m_isRunning = false;

			for (auto const layer : m_layerStack)
			{
				layer->OnUpdate();
				layer->OnRender();
			}

			Mouse::Reset();
		}
	}

	void Application::OnRestart() noexcept
	{
		//...
	}

	void Application::OnStartUp() noexcept
	{
		Window::Initialize(m_specification);
	}

	void Application::OnShutDown() noexcept
	{
		//...
	}

	void Application::PushLayer(Layer* layer) noexcept
	{
		m_layerStack.PushLayer(layer);
	}

	void Application::PushOverlay(Layer* layer) noexcept
	{
		m_layerStack.PushOverlay(layer);
	}

	void Application::PopLayer(Layer* layer) noexcept
	{
		m_layerStack.PopLayer(layer);
	}

	void Application::PopOverlay(Layer* layer) noexcept
	{
		m_layerStack.PopOverlay(layer);
	}
}
