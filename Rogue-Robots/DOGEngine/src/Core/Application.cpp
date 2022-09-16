#include "Application.h"
#include "Window.h"
#include "Time.h"
#include "AssetManager.h"

#include "../Input/Mouse.h"
#include "../Input/Keyboard.h"

#include "../EventSystem/EventBus.h"
#include "../EventSystem/LayerStack.h"

#include "../Graphics/Rendering/Renderer.h"

#include "../Core/DataPiper.h"

#include "ImGUI/imgui.h"

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
		: m_specification{ spec }, m_isRunning{ true }, m_layerStack{ LayerStack::Get() }
	{
		OnStartUp();
	}

	Application::~Application() noexcept
	{
		OnShutDown();
	}

	using namespace DOG::gfx;
	void Application::Run() noexcept
	{
		// Temporary read only data from runtime
		const piper::PipedData* runtimeData = piper::GetPipe();

		bool showDemoWindow = true;

		while (m_isRunning)
		{
			Time::Start();
			Window::OnUpdate();

			// All ImGUI calls must happen after this call
			m_renderer->BeginGUI();
		
			// Example ImGUI call
			ImGui::ShowDemoWindow(&showDemoWindow);

			for (auto const layer : m_layerStack)
			{
				layer->OnUpdate();
				layer->OnRender();
			}

			Mouse::Reset();

			//// ====== GPU
			m_renderer->BeginFrame_GPU();

			m_renderer->SetMainRenderCamera(runtimeData->viewMat);

			m_renderer->Update(0.0f);
			m_renderer->Render(0.0f);

			m_renderer->EndFrame_GPU(true);

			Time::End();
		}

		//rd->Flush();
		//bin.ForceClear();

		m_renderer->Flush();
	}

	void Application::OnRestart() noexcept
	{
		//...
	}

	void Application::OnEvent(IEvent& event) noexcept
	{
		if (!(event.GetEventCategory() == EventCategory::WindowEventCategory))
			return;

		switch (event.GetEventType())
		{
		case EventType::WindowClosedEvent:
			m_isRunning = false;
			break;
		}
	}

	void Application::OnStartUp() noexcept
	{
		std::filesystem::current_path(m_specification.workingDir);

		EventBus::Get().SetMainApplication(this);
		Window::Initialize(m_specification);
#ifdef _DEBUG
		m_renderer = std::make_unique<gfx::Renderer>(Window::GetHandle(), Window::GetWidth(), Window::GetHeight(), true);
#else
		m_renderer = std::make_unique<gfx::Renderer>(Window::GetHandle(), Window::GetWidth(), Window::GetHeight(), false);
#endif
		Window::SetWMHook(m_renderer->GetWMCallback());

		AssetManager::Initialize();
	}

	void Application::OnShutDown() noexcept
	{
		AssetManager::Destroy();
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
