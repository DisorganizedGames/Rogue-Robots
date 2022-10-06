#include "Application.h"
#include "Window.h"
#include "Time.h"
#include "../Physics/PhysicsEngine.h"
#include "AnimationManager.h"
#include "AssetManager.h"
#include "../ECS/EntityManager.h"
#include "../Input/Mouse.h"
#include "../Input/Keyboard.h"

#include "../EventSystem/EventBus.h"
#include "../EventSystem/LayerStack.h"
#include "../EventSystem/WindowEvents.h"

#include "../Graphics/Rendering/Renderer.h"

#include "ImGUI/imgui.h"

#include "../Audio/AudioManager.h"
#include "ImGuiMenuLayer.h"


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
		//BulletPhysics::Initialize();
		//BulletPhysics::BulletTest();

		while (m_isRunning)
		{
			Time::Start();
			Window::OnUpdate();

			// Early break if WM tells us to
			if (!m_isRunning)
				break;

			PhysicsEngine::UpdatePhysics((f32)Time::DeltaTime());

			// All ImGUI calls must happen after this call
			m_renderer->BeginGUI();

			AssetManager::Get().Update();
			AudioManager::AudioSystem();

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
			Mouse::Reset();

			//// ====== GPU
			m_renderer->BeginFrame_GPU();

			EntityManager::Get().Bundle<TransformComponent, ModelComponent>().Do([&](entity e, TransformComponent& transformC, ModelComponent& modelC)
				{
					ModelAsset* model = AssetManager::Get().GetAsset<ModelAsset>(modelC);
					if (model && model->gfxModel)
					{
						if (EntityManager::Get().HasComponent<ModularBlockComponent>(e))
						{
							for (u32 i = 0; i < model->gfxModel->mesh.numSubmeshes; ++i)
								m_renderer->SubmitMeshNoFaceCulling(model->gfxModel->mesh.mesh, i, model->gfxModel->mats[i], transformC);
						}
						else if (EntityManager::Get().HasComponent<AnimationComponent>(e))
						{
							for (u32 i = 0; i < model->gfxModel->mesh.numSubmeshes; ++i)
								m_renderer->SubmitAnimatedMesh(model->gfxModel->mesh.mesh, i, model->gfxModel->mats[i], transformC);
						}	
						else
						{
							for (u32 i = 0; i < model->gfxModel->mesh.numSubmeshes; ++i)
								m_renderer->SubmitMesh(model->gfxModel->mesh.mesh, i, model->gfxModel->mats[i], transformC);
						}
					}
				});

			auto mainCam = CameraComponent::s_mainCamera;
			auto& proj = (DirectX::XMMATRIX&)mainCam->projMatrix;
			m_renderer->SetMainRenderCamera(mainCam->viewMatrix, &proj);
			
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
		{
			m_isRunning = false;
			break;
		}
		case EventType::WindowResizedEvent:
		{
			auto& e = EVENT(WindowResizedEvent);
			if (m_renderer)
			{
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
					m_fullscreenStateOnFocusLoss = m_renderer->GetFullscreenState();
					m_renderer->SetFullscreenState(WindowMode::Windowed, *m_specification.displayMode);
				}
				else
				{
					m_renderer->SetFullscreenState(m_fullscreenStateOnFocusLoss, *m_specification.displayMode);
				}
			}
			break;
		}
		case EventType::WindowAltEnterEvent:
		{
			if (m_renderer)
			{
				if (m_renderer->GetFullscreenState() != WindowMode::FullScreen)
					m_renderer->SetFullscreenState(WindowMode::FullScreen, *m_specification.displayMode);
				else
					m_renderer->SetFullscreenState(WindowMode::Windowed, *m_specification.displayMode);
			}
			break;
		}
		case EventType::WindowHitBorderEvent:
		{
			std::cout << "WindowHitBorderEvent" << std::endl;
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
		Window::SetWMHook(m_renderer->GetWMCallback());

		AssetManager::Initialize(m_renderer.get());
		AudioManager::Initialize();
		PhysicsEngine::Initialize();

		ImGuiMenuLayer::RegisterDebugWindow("ApplicationSetting", [this](bool& open) { ApplicationSettingDebugMenu(open); });

		if (!m_specification.displayMode)
		{
			m_specification.displayMode = m_renderer->GetDefaultDisplayMode();
		}
	}

	void Application::OnShutDown() noexcept
	{
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
					return str;
				};

				static i64 selectedModeIndex = std::ssize(monitor.modes) - 1;
				selectedModeIndex = std::min(selectedModeIndex, static_cast<i64>(std::ssize(monitor.modes) - 1));
				if (ImGui::BeginCombo("modes", modeElementToString(selectedModeIndex).c_str()))
				{
					for (i64 i = std::ssize(monitor.modes) - 1; i >= 0; i--)
					{
						if (ImGui::Selectable(modeElementToString(i).c_str(), selectedModeIndex == i))
						{
							selectedModeIndex = i;
							m_specification.displayMode = monitor.modes[selectedModeIndex];
							if (m_renderer->GetFullscreenState() == WindowMode::FullScreen)
							{
								m_renderer->SetFullscreenState(WindowMode::FullScreen, monitor.modes[selectedModeIndex]);
							}
						}
					}
					ImGui::EndCombo();
				}

				static int selectedFullscreenStateIndex = 0;
				selectedFullscreenStateIndex = static_cast<int>(m_renderer->GetFullscreenState());
				std::array<const char*, 2> fullscreenCombo = { "Windowed", "Fullscreen" };
				if (ImGui::Combo("Fullscreen mode", &selectedFullscreenStateIndex, fullscreenCombo.data(), static_cast<int>(fullscreenCombo.size())))
				{
					m_renderer->SetFullscreenState(static_cast<WindowMode>(selectedFullscreenStateIndex), monitor.modes[selectedModeIndex]);
				}
				ImGui::Separator();
			}
			ImGui::End(); // "Application settings"
		}
	}
}
