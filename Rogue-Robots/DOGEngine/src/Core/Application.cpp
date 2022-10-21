#include "Application.h"
#include "Window.h"
#include "Time.h"
#include "../Physics/PhysicsEngine.h"
#include "../Scripting//LuaMain.h"
#include "AnimationManager.h"
#include "AssetManager.h"
#include "LightManager.h"
#include "CustomMaterialManager.h"
#include "CustomMeshManager.h"
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
		//BulletPhysics::Initialize();
		//BulletPhysics::BulletTest();


		while (m_isRunning)
		{
			Time::Start();
			MiniProfiler::Update();
			MINIPROFILE
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

			// Update lights

			EntityManager::Get().Collect<DirtyComponent, PointLightComponent>().Do([](entity, DirtyComponent& dirty, PointLightComponent& light) {
				light.dirty |= dirty.IsDirty(DirtyComponent::positionChanged); });

			EntityManager::Get().Collect<DirtyComponent, SpotLightComponent>().Do([](entity, DirtyComponent& dirty, SpotLightComponent& light) {
				light.dirty |= dirty.IsDirty(DirtyComponent::positionChanged) || dirty.IsDirty(DirtyComponent::rotationChanged); });

			EntityManager::Get().Collect<TransformComponent, SpotLightComponent>().Do([&](entity, TransformComponent tr, SpotLightComponent& light)
				{
					if (light.dirty)
					{
						SpotLightDesc d{};
						d.position = tr.GetPosition();
						d.color = light.color;
						d.cutoffAngle = light.cutoffAngle;
						d.direction = light.direction;
						d.strength = light.strength;
						LightManager::Get().UpdateSpotLight(light.handle, d);
						light.dirty = false;
					}
				});

			EntityManager::Get().Collect<TransformComponent, PointLightComponent>().Do([&](entity, TransformComponent tr, PointLightComponent& light)
				{
					if (light.dirty)
					{
						PointLightDesc d{};
						d.position = tr.GetPosition();
						d.color = light.color;
						d.strength = light.strength;
						LightManager::Get().UpdatePointLight(light.handle, d);
						light.dirty = false;
					}
				});


			EntityManager::Get().Collect<TransformComponent, SubmeshRenderer>().Do([&](entity, TransformComponent& tr, SubmeshRenderer& sr)
				{
					// We are assuming that this is a totally normal submesh with no weird branches (i.e on ModularBlock or whatever)
					if (sr.dirty)
						CustomMaterialManager::Get().UpdateMaterial(sr.material, sr.materialDesc);
					m_renderer->SubmitMesh(sr.mesh, 0, sr.material, tr);
				});

			// We need to bucket in a better way..
			EntityManager::Get().Collect<TransformComponent, ModelComponent>().Do([&](entity e, TransformComponent& transformC, ModelComponent& modelC)
				{
					MINIPROFILE_NAMED("RenderSystem")
					ModelAsset* model = AssetManager::Get().GetAsset<ModelAsset>(modelC);
					if (model && model->gfxModel)
					{
						if (EntityManager::Get().HasComponent<ModularBlockComponent>(e))
						{
							if (EntityManager::Get().HasComponent<MeshColliderComponent>(e) &&
								EntityManager::Get().GetComponent<MeshColliderComponent>(e).drawMeshColliderOverride)
							{
								u32 meshColliderModelID = EntityManager::Get().GetComponent<MeshColliderComponent>(e).meshColliderModelID;
								ModelAsset * meshColliderModel = AssetManager::Get().GetAsset<ModelAsset>(meshColliderModelID);
								if (meshColliderModel && meshColliderModel->gfxModel)
								{
									for (u32 i = 0; i < meshColliderModel->gfxModel->mesh.numSubmeshes; ++i)
										m_renderer->SubmitMeshWireframeNoFaceCulling(meshColliderModel->gfxModel->mesh.mesh, i, meshColliderModel->gfxModel->mats[i], transformC);
								}
							}
							else
							{
								for (u32 i = 0; i < model->gfxModel->mesh.numSubmeshes; ++i)
									m_renderer->SubmitMeshNoFaceCulling(model->gfxModel->mesh.mesh, i, model->gfxModel->mats[i], transformC);
							}
						}
						else if (EntityManager::Get().HasComponent<AnimationComponent>(e))
						{
							for (u32 i = 0; i < model->gfxModel->mesh.numSubmeshes; ++i)
								m_renderer->SubmitAnimatedMesh(model->gfxModel->mesh.mesh, i, model->gfxModel->mats[i], transformC);
						}	
						else
						{
							if (EntityManager::Get().HasComponent<MeshColliderComponent>(e) &&
								EntityManager::Get().GetComponent<MeshColliderComponent>(e).drawMeshColliderOverride)
							{
								for (u32 i = 0; i < model->gfxModel->mesh.numSubmeshes; ++i)
									m_renderer->SubmitMeshWireframe(model->gfxModel->mesh.mesh, i, model->gfxModel->mats[i], transformC);
							}
							else
							{
								for (u32 i = 0; i < model->gfxModel->mesh.numSubmeshes; ++i)
									m_renderer->SubmitMesh(model->gfxModel->mesh.mesh, i, model->gfxModel->mats[i], transformC);
							}
						}
					}
				});

			if (CameraComponent::s_mainCamera)
			{
				auto mainCam = CameraComponent::s_mainCamera;
				auto& proj = (DirectX::XMMATRIX&)mainCam->projMatrix;
				m_renderer->SetMainRenderCamera(mainCam->viewMatrix, &proj);
			}
			
			m_renderer->Update(0.0f);
			m_renderer->Render(0.0f);

			m_renderer->EndFrame_GPU(m_specification.graphicsSettings.vSync);


			EntityManager::Get().Collect<DirtyComponent>().Do([](entity, DirtyComponent& dirty) { dirty.dirtyBitSet &= 0; });

			//Deferred deletions happen here!!!
			LuaMain::GetScriptManager()->RemoveScriptsFromDeferredEntities();
			PhysicsEngine::FreePhysicsFromDeferredEntities();
			AudioManager::StopAudioOnDeferredEntities();
			EntityManager::Get().DestroyDeferredEntities();

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
		LightManager::Initialize(m_renderer.get());
		CustomMeshManager::Initialize(m_renderer.get());
		CustomMaterialManager::Initialize(m_renderer.get());


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
		
		LightManager::Destroy();
		CustomMeshManager::Destroy();
		CustomMaterialManager::Destroy();
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
				ImGui::Checkbox("Vsync", &m_specification.graphicsSettings.vSync);
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
				{
					m_renderer->SetGraphicsSettings(m_specification.graphicsSettings);
				}

				if (ImGui::SliderFloat("BloomThreshold", &m_specification.graphicsSettings.bloomThreshold, 0.1f, 3))
				{
					m_renderer->SetGraphicsSettings(m_specification.graphicsSettings);
				}

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
