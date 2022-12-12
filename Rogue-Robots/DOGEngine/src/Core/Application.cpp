#include "Application.h"
#include "Window.h"
#include "Time.h"
#include "../Physics/PhysicsEngine.h"
#include "../Scripting//LuaMain.h"
#include "AnimationManager.h"
#include "AssetManager.h"
#include "../ECS/EntityManager.h"		// to remove
#include "../Input/Mouse.h"
#include "../Input/Keyboard.h"

#include "../EventSystem/EventBus.h"
#include "../EventSystem/LayerStack.h"
#include "../EventSystem/WindowEvents.h"

#include "../Graphics/Rendering/Renderer.h"
#include "../Graphics/Rendering/FrontRenderer.h"

#include "ImGUI/imgui.h"

#include "../Audio/AudioManager.h"
#include "ImGuiMenuLayer.h"

#include "../common/MiniProfiler.h"

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
		while (m_isRunning)
		{
			Time::Start();
			MiniProfiler::Update();
			MINIPROFILE
			Mouse::Reset();
			Window::OnUpdate();

			// Early break if WM tells us to
			if (!m_isRunning)
				break;

			AssetManager::Get().Update();

			for (auto& system : EntityManager::Get())
			{
				system->EarlyUpdate();
			}

			PhysicsEngine::UpdatePhysics((f32)Time::DeltaTime());

			AudioManager::AudioSystem();

			m_frontRenderer->BeginFrameUICapture();
			for (auto const layer : m_layerStack)
			{
				layer->OnUpdate();
				layer->OnRender();
			}
#if defined _DEBUG
			for (auto const layer : m_layerStack)
			{
				layer->OnImGuiRender();
			}
#endif
			for (auto& system : EntityManager::Get())
			{
				system->Update();
			}

			m_frontRenderer->Update(Time::DeltaTime<TimeType::Seconds, f32>());
			m_frontRenderer->BeginGPUFrame();
			m_frontRenderer->Render(Time::DeltaTime<TimeType::Seconds, f32>());
			m_frontRenderer->EndGPUFrame();

			for (auto& system : EntityManager::Get())
			{
				system->LateUpdate();
			}

			EntityManager::Get().Collect<DirtyComponent>().Do([](entity, DirtyComponent& dirty) { dirty.dirtyBitSet &= 0; });	

			// Remove collision components from entities before next frame's collisions
			EntityManager::Get().Collect<HasEnteredCollisionComponent>().Do([](entity e, HasEnteredCollisionComponent& c)
				{
					if (c.entitiesCount > HasEnteredCollisionComponent::maxCount) std::cout << "HasCollidedComponent collided with more then" << HasEnteredCollisionComponent::maxCount << " other entities" << std::endl;
					EntityManager::Get().RemoveComponent<HasEnteredCollisionComponent>(e);
				});

			//Deferred deletions happen here!!!
			LuaMain::GetScriptManager()->RemoveScriptsFromDeferredEntities();
			m_frontRenderer->PerformDeferredDeletion();
			PhysicsEngine::FreePhysicsFromDeferredEntities();
			AudioManager::StopAudioOnDeferredEntities();
			EntityManager::Get().DestroyDeferredEntities();

			Time::End();
		}

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
		{
			m_isRunning = false;
			break;
		}
		case EventType::WindowPosChangingEvent:
		{
			if (!m_renderer) break;
			static WindowMode prevFullScreenState = WindowMode::Windowed;
			WindowMode currentFullscreenState = m_renderer->GetFullscreenState();
			if (currentFullscreenState != prevFullScreenState)
			{
				prevFullScreenState = currentFullscreenState;
				m_renderer->OnResize(0, 0); // forece the backbuffers to resize
				Window::PublishEvent<WindowPostPosChangingEvent>();
			}
			break;
		}
		case EventType::WindowResizedEvent:
		{
			auto& e = EVENT(WindowResizedEvent);
			if (m_renderer)
			{
				if (m_specification.graphicsSettings.windowMode == WindowMode::Windowed)
					m_specification.windowDimensions = e.dimensions;

				m_renderer->OnResize(e.dimensions.x, e.dimensions.y);

				// If mouse is confined we need to re confine it to the new window size
				if ((Window::GetCursorMode() & CursorMode::Confined) == CursorMode::Confined)
				{
					// This will make the free the cursor and then lock it to the new rectangle.
					Window::SetCursorMode(~Window::GetCursorMode());
					Window::SetCursorMode(~Window::GetCursorMode());
				}
				Window::PublishEvent<WindowPostResizedEvent>();
			}
			break;
		}
		case EventType::WindowActiveEvent:
		{
			if (m_renderer)
			{
				auto& e = EVENT(WindowActiveEvent);
				if (!e.active)
				{
					m_cursorModeOnFocusLoss = Window::GetCursorMode();
					Window::SetCursorMode(CursorMode::Visible);

					m_fullscreenStateOnFocusLoss = m_renderer->GetFullscreenState();
					m_specification.graphicsSettings.windowMode = WindowMode::Windowed;

					Keyboard::Reset();
				}
				else
				{
					Window::SetCursorMode(m_cursorModeOnFocusLoss);
					m_specification.graphicsSettings.windowMode = m_fullscreenStateOnFocusLoss;
				}

				m_renderer->SetGraphicsSettings(m_specification.graphicsSettings);
			}
			break;
		}
		case EventType::WindowAltEnterEvent:
		{
			if (m_renderer)
			{
				if (m_renderer->GetFullscreenState() != WindowMode::FullScreen)
					m_specification.graphicsSettings.windowMode = WindowMode::FullScreen;
				else
					m_specification.graphicsSettings.windowMode = WindowMode::Windowed;

				m_renderer->SetGraphicsSettings(m_specification.graphicsSettings);
			}
			break;
		}
		case EventType::WindowHitBorderEvent:
		{
			m_fullscreenStateOnFocusLoss = WindowMode::Windowed;
			break;
		}
	}
	}

	void Application::SetGraphicsSettings(const GraphicsSettings& settings) noexcept
	{
		m_specification.graphicsSettings = settings;
		ApplyGraphicsSettings();
	}

	GraphicsSettings Application::GetGraphicsSettings() const noexcept
	{
		return m_specification.graphicsSettings;
	}

	gfx::Monitor Application::GetMonitor() const noexcept
	{
		return m_renderer->GetMonitor();
	}

	void Application::OnStartUp() noexcept
	{
		std::filesystem::current_path(m_specification.workingDir);

		EventBus::Get().SetMainApplication(this);
		Window::Initialize(m_specification);
#ifdef _DEBUG
		m_renderer = std::make_unique<gfx::Renderer>(Window::GetHandle(), Window::GetWidth(), Window::GetHeight(), true, m_specification.graphicsSettings);
#else
		m_renderer = std::make_unique<gfx::Renderer>(Window::GetHandle(), Window::GetWidth(), Window::GetHeight(), false, m_specification.graphicsSettings);
#endif
		// If SetGraphicsSettings is called from inside the Renderers constructor we get strange result.
		m_renderer->SetGraphicsSettings(m_specification.graphicsSettings);
		assert(m_specification.graphicsSettings.windowMode == m_renderer->GetFullscreenState());

		Window::SetWMHook(m_renderer->GetWMCallback());
		m_frontRenderer = std::make_unique<gfx::FrontRenderer>(m_renderer.get());

		AssetManager::Initialize(m_renderer.get());
		AudioManager::Initialize();
		PhysicsEngine::Initialize();
		LuaMain::Initialize();


		ImGuiMenuLayer::RegisterDebugWindow("MiniProfiler", [](bool& open) { MiniProfiler::DrawResultWithImGui(open); }, true);
	}

	void Application::OnShutDown() noexcept
	{
		ImGuiMenuLayer::UnRegisterDebugWindow("MiniProfiler");
		AssetManager::Destroy();
		AudioManager::Destroy();
		
		::DestroyWindow(Window::GetHandle());
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

	const ApplicationSpecification& Application::GetApplicationSpecification() const noexcept
	{
		return m_specification;
	}

	void Application::ApplyGraphicsSettings() noexcept
	{
		m_renderer->VerifyAndSanitizeGraphicsSettings(m_specification.graphicsSettings, Window::GetWidth(), Window::GetHeight());

		m_frontRenderer->ToggleShadowMapping(m_specification.graphicsSettings.shadowMapping);

		m_renderer->SetGraphicsSettings(m_specification.graphicsSettings);

		m_specification.graphicsSettings.windowMode = m_renderer->GetFullscreenState();
	}

	Vector2u Application::GetAspectRatio()  const noexcept
	{
		Vector2u aspectRatio;
		if (m_renderer->GetFullscreenState() == WindowMode::Windowed)
		{
			std::tie(aspectRatio.x, aspectRatio.y) = Window::GetDimensions();
		}
		else
		{
			assert(m_specification.graphicsSettings.displayMode);
			aspectRatio.x = m_specification.graphicsSettings.displayMode->Width;
			aspectRatio.y = m_specification.graphicsSettings.displayMode->Height;
		}

		u32 d = std::gcd(aspectRatio.x, aspectRatio.y);
		aspectRatio.x /= d;
		aspectRatio.y /= d;
		return aspectRatio;
	}
	
}
