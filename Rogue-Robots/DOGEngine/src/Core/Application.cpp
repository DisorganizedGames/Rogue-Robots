#include "Application.h"
#include "Window.h"
#include "Time.h"
#include "AssetManager.h"

#include "../Input/Mouse.h"
#include "../Input/Keyboard.h"



#include "../Graphics/RHI/DX12/RenderBackend_DX12.h"
#include "../Graphics/RHI/DX12/RenderDevice_DX12.h"
#include "../Graphics/RHI/ShaderCompilerDXC.h"
#include "../Graphics/RHI/PipelineBuilder.h"

#include "../Graphics/Rendering/GPUGarbageBin.h"
#include "../Graphics/Rendering/UploadContext.h"
#include "../Graphics/Rendering/GPUTable.h"
#include "../Graphics/Rendering/GPUDynamicConstants.h"

#include "../EventSystem/EventBus.h"
#include "../EventSystem/LayerStack.h"
#include "../Graphics/Rendering/MaterialTable.h"
#include "../Graphics/Rendering/MeshTable.h"

#include "../Core/AssimpImporter.h"
#include "../Core/TextureFileImporter.h"
#include "../Graphics/Rendering/TextureManager.h"


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
		const u32 NUM_BUFFERS = 2;

		auto hwnd = Window::GetHandle();
		auto backend = std::make_unique<gfx::RenderBackend_DX12>(true);
		auto rd = backend->CreateDevice();
		auto sc = rd->CreateSwapchain(hwnd, NUM_BUFFERS);

		// Create depth
		Texture depthTex;
		TextureView depthTarget;
		{
			TextureDesc d(MemoryType::Default, DXGI_FORMAT_D32_FLOAT, Window::GetWidth(), Window::GetHeight(), 1, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
			TextureViewDesc tvd(ViewType::DepthStencil, TextureViewDimension::Texture2D, DXGI_FORMAT_D32_FLOAT);
			depthTex = rd->CreateTexture(d);
			depthTarget = rd->CreateView(depthTex, tvd);
		}

		std::array<Texture, NUM_BUFFERS> scTextures;
		std::array<TextureView, NUM_BUFFERS> scViews;
		std::array<RenderPass, NUM_BUFFERS> scPasses;
		for (u8 i = 0; i < NUM_BUFFERS; ++i)
		{
			// Grab textures
			scTextures[i] = sc->GetBuffer(i);

			// Create texture as target
			auto td = TextureViewDesc(ViewType::RenderTarget, TextureViewDimension::Texture2D, sc->GetBufferFormat());
			scViews[i] = rd->CreateView(scTextures[i], td);

			scPasses[i] = rd->CreateRenderPass(RenderPassBuilder()
				.AppendRT(scViews[i], RenderPassBeginAccessType::Clear, RenderPassEndingAccessType::Preserve)
				.AddDepth(depthTarget, RenderPassBeginAccessType::Clear, RenderPassEndingAccessType::Discard)
				.Build());
		}

		auto sclr = std::make_unique<ShaderCompilerDXC>();
		auto fullscreenTriVS = sclr->CompileFromFile("FullscreenTriVS.hlsl", ShaderType::Vertex);
		auto blitPS = sclr->CompileFromFile("BlitPS.hlsl", ShaderType::Pixel);
		Pipeline pipe = rd->CreateGraphicsPipeline(GraphicsPipelineBuilder()
			.SetShader(fullscreenTriVS.get())
			.SetShader(blitPS.get())
			.AppendRTFormat(sc->GetBufferFormat())
			.SetDepthFormat(DepthFormat::D32)
			.SetDepthStencil(DepthStencilBuilder().SetDepthEnabled(true))
			.Build());

		auto meshVS = sclr->CompileFromFile("MainVS.hlsl", ShaderType::Vertex);
		auto meshPS = sclr->CompileFromFile("MainPS.hlsl", ShaderType::Pixel);
		Pipeline meshPipe = rd->CreateGraphicsPipeline(GraphicsPipelineBuilder()
			.SetShader(meshVS.get())
			.SetShader(meshPS.get())
			.AppendRTFormat(sc->GetBufferFormat())
			.SetDepthFormat(DepthFormat::D32)
			.SetDepthStencil(DepthStencilBuilder().SetDepthEnabled(true))
			.Build());

		GPUGarbageBin bin(1);
		UploadContext upCtx(rd, 40'000'000, 1);
		UploadContext texUpCtx(rd, 400'000'000, 1);

		// Test mesh manager		

		// Startup
		MeshTable::MemorySpecification spec{};
		spec.maxSizePerAttribute[VertexAttribute::Position] = 4'000'000;
		spec.maxSizePerAttribute[VertexAttribute::UV] = 4'000'000;
		spec.maxSizePerAttribute[VertexAttribute::Normal] = 4'000'000;
		spec.maxSizePerAttribute[VertexAttribute::Tangent] = 4'000'000;
		spec.maxTotalSubmeshes = 500;
		spec.maxNumIndices = 1'000'000;
		MeshTable meshTab(rd, &bin, spec);

		MaterialTable::MemorySpecification memSpec{};
		memSpec.maxElements = 500;	// 500 distinct materials
		MaterialTable matTab(rd, &bin, memSpec);

		TextureManager texMan(rd, &bin);

		// Load ONE model
		MeshContainer sponza;
		std::vector<MaterialHandle> mats;
		{
			// Load mesh
			auto res = AssimpImporter("Assets/Sponza_gltf/glTF/Sponza.gltf").GetResult();
			MeshTable::MeshSpecification loadSpec{};
			loadSpec.indices = res->mesh.indices;
			for (const auto& attr : res->mesh.vertexData)
				loadSpec.vertexDataPerAttribute[attr.first] = res->mesh.vertexData[attr.first];
			loadSpec.submeshData = res->submeshes;
			sponza = meshTab.LoadMesh(loadSpec, upCtx);

			upCtx.SubmitCopies();


			// Load materials
			auto loadTexture = [](
				TextureManager& texMan, UploadContext& ctx, 
				const std::string& name, ImportedTextureFile& textureData, bool srgb) -> Texture
			{
				TextureManager::MippedTexture2DSpecification spec{};
				for (auto& mip : textureData.dataPerMip)
				{
					TextureManager::TextureSubresource subr{};
					subr.data = mip.data;
					subr.width = mip.width;
					subr.height = mip.height;
					spec.dataPerMip.push_back(subr);
				}
				spec.srgb = srgb;
				return texMan.LoadTexture(name, spec, ctx);
			};

			auto loadToMat = [&](const std::string& path, bool srgb, bool genMips) -> std::optional<TextureView>
			{
				std::optional<Texture> tex;
				if (!texMan.Exists(path))
				{
					const bool srgb = true;

					auto importedTex = TextureFileImporter(path, genMips).GetResult();
					if (importedTex)
						tex = loadTexture(texMan, texUpCtx, path, *importedTex, srgb);
				}
				else
					tex = texMan.GetTexture(path);

				if (tex)
				{
					return rd->CreateView(*tex, TextureViewDesc(
						ViewType::ShaderResource,
						TextureViewDimension::Texture2D,
						srgb ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM));
				}
				else
					return {};

			};

			for (const auto& mat : res->materials)
			{
				std::optional<Texture> albedoTex, metallicRoughnessTex, normalTex, emissiveTex;
				MaterialTable::MaterialSpecification matSpec{};

				const bool genMips = true;

				matSpec.albedo = loadToMat(mat.albedoPath, true, genMips);
				matSpec.metallicRoughness = loadToMat(mat.metallicRoughnessPath, false, genMips);
				matSpec.normal = loadToMat(mat.normalMapPath, false, genMips);
				matSpec.emissive = loadToMat(mat.emissivePath, true, genMips);

				mats.push_back(matTab.LoadMaterial(matSpec, upCtx));
			}


			upCtx.SubmitCopies();

			texUpCtx.SubmitCopies();
		}






		
		GPUDynamicConstants cMan(rd, &bin, 500);

		CommandList cmdl = rd->AllocateCommandList();
		u32 count = 0;

		while (m_isRunning)
		{
			Time::Start();
			Window::OnUpdate();

			for (auto const layer : m_layerStack)
			{
				layer->OnUpdate();
				layer->OnRender();
			}

			Mouse::Reset();



			// ====== GPU
			auto& scTex = scTextures[sc->GetNextDrawSurfaceIdx()];
			auto& scPass = scPasses[sc->GetNextDrawSurfaceIdx()];

			rd->Flush();

			bin.BeginFrame();
			rd->RecycleCommandList(cmdl);
			cmdl = rd->AllocateCommandList();


			// Write
			{
				GPUBarrier barrs[]
				{
					GPUBarrier::Transition(scTex, 0, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET),
					GPUBarrier::Transition(depthTex, 0, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE)
				};
				rd->Cmd_Barrier(cmdl, barrs);
			}

			rd->Cmd_BeginRenderPass(cmdl, scPass);

			rd->Cmd_SetViewports(cmdl, Viewports()
				.Append(0.f, 0.f, (f32)Window::GetWidth(), (f32)Window::GetHeight()));
			rd->Cmd_SetScissorRects(cmdl, ScissorRects()
				.Append(0, 0, Window::GetWidth(), Window::GetHeight()));

			// Fullscreen
			// Update and set constant
			//rd->Cmd_SetPipeline(cmdl, pipe);
			//{
			//	auto constant = cMan.Allocate();
			//	struct SomeData
			//	{
			//		f32 a, b, c, d;
			//	};
			//	SomeData dat{};
			//	dat.a = count == 0 ? 1.f : 0.f;
			//	dat.b = count == 1 ? 1.f : 0.f;
			//	dat.c = count == 2 ? 1.f : 0.f;
			//	dat.d = 1.f;
			//	std::memcpy(constant.memory, &dat, sizeof(SomeData));
			//	rd->Cmd_UpdateShaderArgs(cmdl, ShaderArgs()
			//		.AppendConstant(constant.globalDescriptor));
			//}
			//rd->Cmd_Draw(cmdl, 3, 1, 0, 0);

			
			rd->Cmd_SetPipeline(cmdl, meshPipe);
			rd->Cmd_SetIndexBuffer(cmdl, meshTab.GetIndexBuffer());
			for (u32 i = 0; i < sponza.numSubmeshes; ++i)
			{
				auto pfConstant = cMan.Allocate();
				struct PerFrameData
				{
					DirectX::XMMATRIX world, view, proj;
				} pfData{};
				pfData.world = DirectX::XMMatrixScaling(0.07f, 0.07f, 0.07f);
				pfData.view = DirectX::XMMatrixLookAtLH({ 5.f, 2.f, 0.f }, { -1.f, 1.f, 1.f }, { 0.f, 1.f, 0.f });
				// We are using REVERSE DEPTH!!!
				pfData.proj = DirectX::XMMatrixPerspectiveFovLH(80.f * 3.1415f / 180.f, (f32)Window::GetWidth() / Window::GetHeight(), 800.f, 0.1f);
				std::memcpy(pfConstant.memory, &pfData, sizeof(pfData));

				auto args = ShaderArgs()
					.AppendConstant(pfConstant.globalDescriptor)
					.AppendConstant(meshTab.GetSubmeshMD_GPU(sponza.mesh, i))
					.AppendConstant(meshTab.GetSubmeshDescriptor())
					.AppendConstant(meshTab.GetAttributeDescriptor(VertexAttribute::Position))
					.AppendConstant(meshTab.GetAttributeDescriptor(VertexAttribute::UV))
					.AppendConstant(meshTab.GetAttributeDescriptor(VertexAttribute::Normal))
					.AppendConstant(meshTab.GetAttributeDescriptor(VertexAttribute::Tangent))
					.AppendConstant(matTab.GetDescriptor())
					.AppendConstant(matTab.GetMaterialIndex(mats[i])
					);

				rd->Cmd_UpdateShaderArgs(cmdl, args);

				auto sm = meshTab.GetSubmeshMD_CPU(sponza.mesh, i);
				rd->Cmd_DrawIndexed(cmdl, sm.indexCount, 1, sm.indexStart, 0, 0);

			}
		
			rd->Cmd_EndRenderPass(cmdl);

			// Present
			{
				GPUBarrier barrs[]
				{
					GPUBarrier::Transition(scTex, 0, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT),
					GPUBarrier::Transition(depthTex, 0, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_COMMON)

				};
				rd->Cmd_Barrier(cmdl, barrs);
			}

			rd->SubmitCommandList(cmdl);

			sc->Present(false);

			count = (count + 1) % 3;

			bin.EndFrame();
			Time::End();
		}

		rd->Flush();
		bin.ForceClear();
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
		EventBus::Get().SetMainApplication(this);
		Window::Initialize(m_specification);
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
