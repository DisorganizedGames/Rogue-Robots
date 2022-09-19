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

#include "../Graphics/Rendering/TextureManager.h"
#include "../Graphics/Rendering/GraphicsBuilder.h"
#include "../Core/TextureFileImporter.h"

#include <set>


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

		// Example of manual model loading
		// Arbitrary mesh works too, such replace MeshSpec and materials will custom solution.
		//auto builder = m_renderer->GetBuilder();
		//StaticModel model{};
		//{
		//	// Setup mesh spec
		//	auto res = AssimpImporter("Assets/Sponza_gltf/glTF/Sponza.gltf").GetResult();
		//	MeshTable::MeshSpecification loadSpec{};
		//	loadSpec.indices = res->mesh.indices;
		//	for (const auto& attr : res->mesh.vertexData)
		//		loadSpec.vertexDataPerAttribute[attr.first] = res->mesh.vertexData[attr.first];
		//	loadSpec.submeshData = res->submeshes;

		//	// Load materials
		//	auto loadTexture = [&](
		//		ImportedTextureFile& textureData,
		//		bool srgb) -> Texture
		//	{
		//		GraphicsBuilder::MippedTexture2DSpecification spec{};
		//		for (auto& mip : textureData.dataPerMip)
		//		{
		//			GraphicsBuilder::TextureSubresource subr{};
		//			subr.data = mip.data;
		//			subr.width = mip.width;
		//			subr.height = mip.height;
		//			spec.dataPerMip.push_back(subr);
		//		}
		//		spec.srgb = srgb;
		//		return builder->LoadTexture(spec);
		//	};

		//	auto loadToMat = [&](const std::string& path, bool srgb, bool genMips) -> std::optional<TextureView>
		//	{
		//		std::optional<Texture> tex;

		//		auto importedTex = TextureFileImporter(path, genMips).GetResult();
		//		if (importedTex)
		//			tex = loadTexture(*importedTex, srgb);
		//		else
		//			return {};

		//		return builder->CreateTextureView(*tex, TextureViewDesc(
		//			ViewType::ShaderResource,
		//			TextureViewDimension::Texture2D,
		//			srgb ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM));
		//	};

		//	std::vector<MaterialTable::MaterialSpecification> matSpecs;
		//	matSpecs.reserve(res->materials.size());
		//	for (const auto& mat : res->materials)
		//	{
		//		std::optional<Texture> albedoTex, metallicRoughnessTex, normalTex, emissiveTex;
		//		MaterialTable::MaterialSpecification matSpec{};

		//		const bool genMips = true;

		//		matSpec.albedo = loadToMat(mat.albedoPath, true, genMips);
		//		matSpec.metallicRoughness = loadToMat(mat.metallicRoughnessPath, false, genMips);
		//		matSpec.normal = loadToMat(mat.normalMapPath, false, genMips);
		//		matSpec.emissive = loadToMat(mat.emissivePath, true, genMips);
		//		matSpecs.push_back(matSpec);
		//	}

		//	model = builder->LoadCustomModel(loadSpec, matSpecs);
		//}


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

			for (auto const layer : m_layerStack)
			{
				layer->OnUpdate();
				layer->OnRender();
			}

			Mouse::Reset();

			//// ====== GPU
			m_renderer->BeginFrame_GPU();

			// Example submission
			//for (u32 i = 0; i < model.mesh.numSubmeshes; ++i)
			//	m_renderer->SubmitMesh(model.mesh.mesh, i, model.mats[i]);

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
