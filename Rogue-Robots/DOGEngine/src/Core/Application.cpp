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
		ApplyGraphicsSettings();
		Window::SetWMHook(m_renderer->GetWMCallback());
		m_frontRenderer = std::make_unique<gfx::FrontRenderer>(m_renderer.get());

		AssetManager::Initialize(m_renderer.get());
		AudioManager::Initialize();
		PhysicsEngine::Initialize();
		LuaMain::Initialize();


		ImGuiMenuLayer::RegisterDebugWindow("ApplicationSetting", [this](bool& open) { ApplicationSettingDebugMenu(open); });
		ImGuiMenuLayer::RegisterDebugWindow("MiniProfiler", [](bool& open) { MiniProfiler::DrawResultWithImGui(open); }, true);
	}

	void Application::OnShutDown() noexcept
	{
		ImGuiMenuLayer::UnRegisterDebugWindow("MiniProfiler");
		ImGuiMenuLayer::UnRegisterDebugWindow("ApplicationSetting");
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
		m_specification.graphicsSettings.displayMode = m_renderer->GetMatchingDisplayMode(m_specification.graphicsSettings.displayMode);

		// Guard against bad values
		constexpr u32 maxTextureSize = 16384;
		constexpr u32 minTextureSize = 8; // 1 would be allowed but use 8 as a min value for.

		if (m_specification.graphicsSettings.renderResolution.x > maxTextureSize || m_specification.graphicsSettings.renderResolution.y > maxTextureSize
			|| m_specification.graphicsSettings.renderResolution.x < minTextureSize || m_specification.graphicsSettings.renderResolution.y < minTextureSize)
		{
			m_specification.graphicsSettings.renderResolution.x = m_specification.graphicsSettings.displayMode->Width;
			m_specification.graphicsSettings.renderResolution.y = m_specification.graphicsSettings.displayMode->Height;
		}

		Vector2u aspectRatio;
		if (m_specification.graphicsSettings.windowMode == WindowMode::Windowed)
		{
			std::tie(aspectRatio.x, aspectRatio.y) = Window::GetDimensions();
		}
		else
		{
			aspectRatio.x = m_specification.graphicsSettings.displayMode->Width;
			aspectRatio.y = m_specification.graphicsSettings.displayMode->Height;
		}

		u32 d = std::gcd(aspectRatio.x, aspectRatio.y);
		aspectRatio.x /= d;
		aspectRatio.y /= d;

		m_specification.graphicsSettings.renderResolution.x = m_specification.graphicsSettings.renderResolution.y * aspectRatio.x / aspectRatio.y;

		m_renderer->SetGraphicsSettings(m_specification.graphicsSettings);

		m_specification.graphicsSettings.windowMode = m_renderer->GetFullscreenState();
	}

	void Application::ApplicationSettingDebugMenu(bool& open)
	{
		if (ImGui::BeginMenu("View"))
		{
			if (ImGui::MenuItem("Application settings"))
			{
				open = true;
			}
			ImGui::EndMenu(); // "View"
		}

		if (open)
		{
			if (ImGui::Begin("Application settings", &open))
			{
				gfx::Monitor monitor = m_renderer->GetMonitor();

				ImGui::Text("Display settings");
				ImGui::Text(std::filesystem::path(monitor.output.DeviceName).string().c_str());

				int left = monitor.output.DesktopCoordinates.left;
				int right = monitor.output.DesktopCoordinates.right;
				int top = monitor.output.DesktopCoordinates.top;
				int bottom = monitor.output.DesktopCoordinates.bottom;

				std::string rectX = "left: " + std::to_string(left) + ", right: " + std::to_string(right);
				std::string rectY = "top: " + std::to_string(top) + ", bottom : " + std::to_string(bottom);
				ImGui::Text("Rect");
				ImGui::Text(rectX.c_str());
				ImGui::Text(rectY.c_str());


				auto&& modeElementToString = [&monitor](i64 index) -> std::string
				{
					std::string str = "resolution: " + std::to_string(monitor.modes[index].Width) + "x" + std::to_string(monitor.modes[index].Height);
					str += ", hz: " + std::to_string(static_cast<float>(monitor.modes[index].RefreshRate.Numerator) / monitor.modes[index].RefreshRate.Denominator);
					str += ", Format: " + std::to_string(monitor.modes[index].Format);
					str += ", scanline: " + std::to_string(monitor.modes[index].ScanlineOrdering);
					str += ", scaling: " + std::to_string(monitor.modes[index].Scaling);

					UINT c = std::gcd(monitor.modes[index].Width, monitor.modes[index].Height);
					UINT w = monitor.modes[index].Width / c;
					UINT h = monitor.modes[index].Height / c;
					str += ", aspect ratio: " + std::to_string(w) + "/" + std::to_string(h);
					
					return str;
				};

				static i64 selectedModeIndex = std::ssize(monitor.modes) - 1;
				selectedModeIndex = std::min(selectedModeIndex, static_cast<i64>(std::ssize(monitor.modes) - 1));


				static bool firstTime = true;
				if (firstTime)
				{
					selectedModeIndex = [&]()->i64 {
						for (int i = 1; i < monitor.modes.size() - 1; i++)
						{
							auto& other = monitor.modes[i];
							auto& current = *m_specification.graphicsSettings.displayMode;

							if (current.Format == other.Format && current.RefreshRate.Denominator == other.RefreshRate.Denominator
								&& current.RefreshRate.Numerator == other.RefreshRate.Numerator && current.Height == other.Height
								&& current.Width == other.Width && current.Scaling == other.Scaling && current.ScanlineOrdering == other.ScanlineOrdering)
							{
								return i;
							}
						}
						return selectedModeIndex;
					}();
				}

				if (ImGui::BeginCombo("modes", modeElementToString(selectedModeIndex).c_str()))
				{
					for (i64 i = std::ssize(monitor.modes) - 1; i >= 0; i--)
					{
						if (ImGui::Selectable(modeElementToString(i).c_str(), selectedModeIndex == i))
						{
							selectedModeIndex = i;
							m_specification.graphicsSettings.displayMode = monitor.modes[selectedModeIndex];
							m_renderer->SetGraphicsSettings(m_specification.graphicsSettings);
						}
					}
					ImGui::EndCombo();
				}

				static int selectedFullscreenStateIndex = 0;
				selectedFullscreenStateIndex = static_cast<int>(m_renderer->GetFullscreenState());
				std::array<const char*, 2> fullscreenCombo = { "Windowed", "Fullscreen" };
				if (ImGui::Combo("Fullscreen mode", &selectedFullscreenStateIndex, fullscreenCombo.data(), static_cast<int>(fullscreenCombo.size())))
				{
					m_specification.graphicsSettings.windowMode = static_cast<WindowMode>(selectedFullscreenStateIndex);
					m_renderer->SetGraphicsSettings(m_specification.graphicsSettings);
				}

				bool gfxChanged{ false };
				if (ImGui::Checkbox("Vsync", &m_specification.graphicsSettings.vSync))
					gfxChanged = true;

				ImGui::Separator();

				ImGui::Text("Graphics settings");



				
				static std::vector<std::string> res =
				{
					"144",
					"360",
					"720",
					"1080",
					"1440",
					"2160",
				};
				static int resIndex = 3;

				if (firstTime)
				{
					resIndex = [&]()->int {
						for (int i = 1; i < res.size() - 1; i++)
							if (m_specification.graphicsSettings.renderResolution.y == static_cast<u32>(std::stoi(res[i]))) return i;

						res.push_back(std::to_string(m_specification.graphicsSettings.renderResolution.y));
						return static_cast<int>(res.size() - 1);
					}();
					
				}


				ImGui::Text("resolution");
				ImGui::SameLine();

				Vector2u resolutionRatio = GetAspectRatio();
				auto&& resToString = [&](int index) -> std::string
				{
					std::string resX = std::to_string(std::stoi(res[index]) * resolutionRatio.x / resolutionRatio.y);
					return resX + "x" + res[index];
				};

				if (ImGui::BeginCombo("res", resToString(resIndex).c_str()))
				{
					for (int i = 0; i < std::size(res); i++)
					{
						if (ImGui::Selectable(resToString(i).c_str(), resIndex == i))
						{
							resIndex = i;
							m_specification.graphicsSettings.renderResolution.y = std::stoi(res[resIndex]);
							m_specification.graphicsSettings.renderResolution.x = m_specification.graphicsSettings.renderResolution.y * resolutionRatio.x / resolutionRatio.y;
							m_renderer->SetGraphicsSettings(m_specification.graphicsSettings);
						}
					}
					ImGui::EndCombo();
				}

				if (ImGui::Checkbox("Bloom", &m_specification.graphicsSettings.bloom))
					gfxChanged = true;

				if (ImGui::SliderFloat("BloomThreshold", &m_specification.graphicsSettings.bloomThreshold, 0.1f, 3))
					gfxChanged = true;

				if (ImGui::Checkbox("SSAO", &m_specification.graphicsSettings.ssao))
					gfxChanged = true;

				if (ImGui::Checkbox("Shadow Mapping", &m_specification.graphicsSettings.shadowMapping))
				{
					m_frontRenderer->ToggleShadowMapping(m_specification.graphicsSettings.shadowMapping);
					gfxChanged = true;
				}

				if (ImGui::SliderFloat("Gamma", &m_specification.graphicsSettings.gamma, 1.0f, 5.0f))
					gfxChanged = true;

				ImGui::SameLine();
				if (ImGui::Button("Default"))
				{
					m_specification.graphicsSettings.gamma = 2.22f;
					gfxChanged = true;
				}

				if (ImGui::Checkbox("Lit", &m_specification.graphicsSettings.lit))
					gfxChanged = true;

				if (gfxChanged)
					m_renderer->SetGraphicsSettings(m_specification.graphicsSettings);

				firstTime = false;

				//--------------
			}
			ImGui::End(); // "Application settings"
		}
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
