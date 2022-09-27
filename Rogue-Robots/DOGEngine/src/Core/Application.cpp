#include "Application.h"
#include "Window.h"
#include "Time.h"
#include "BoneJovi.h"
#include "AssetManager.h"
#include "../ECS/EntityManager.h"
#include "../Input/Mouse.h"
#include "../Input/Keyboard.h"

#include "../EventSystem/EventBus.h"
#include "../EventSystem/LayerStack.h"

#include "../Graphics/Rendering/Renderer.h"

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
		: m_specification{ spec }, m_isRunning{ true }, m_layerStack{ LayerStack::Get()}
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
		bool showDemoWindow = true;
		EntityManager::Get().Collect<ModelComponent, ModelAnimationComponent>().Do([&](ModelComponent& modelC, ModelAnimationComponent& animationC)
			{
				ModelAsset* model = static_cast<ModelAsset*>(AssetManager::Get().GetAsset(modelC));
				if(animationC.skeletonId == 0) // avoid unref. warning
					m_renderer->SetBones(model->animation);
			});

		while (m_isRunning)
		{
			Time::Start();
			Window::OnUpdate();

			// Early break if WM tells us to
			if (!m_isRunning)
				break;

			// All ImGUI calls must happen after this call
			m_renderer->BeginGUI();
			
			// Example ImGUI call
			ImGui::ShowDemoWindow(&showDemoWindow);

			AssetManager::Get().Update();

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

			EntityManager::Get().Bundle<TransformComponent, ModelComponent>().Do([&](TransformComponent& transformC, ModelComponent& modelC)
				{
					ModelAsset* model = AssetManager::Get().GetAsset<ModelAsset>(modelC);
					if (model)
					{
						for (u32 i = 0; i < model->gfxModel.mesh.numSubmeshes; ++i)
							m_renderer->SubmitMesh(model->gfxModel.mesh.mesh, i, model->gfxModel.mats[i], transformC);
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

		AssetManager::Initialize(m_renderer.get());
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
