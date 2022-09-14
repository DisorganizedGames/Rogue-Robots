#include "Application.h"
#include "Window.h"
#include "Time.h"

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
				.Build());
		}

		auto sclr = std::make_unique<ShaderCompilerDXC>();
		auto fullscreenTriVS = sclr->CompileFromFile("FullscreenTriVS.hlsl", ShaderType::Vertex);
		auto blitPS = sclr->CompileFromFile("BlitPS.hlsl", ShaderType::Pixel);

		Pipeline pipe = rd->CreateGraphicsPipeline(GraphicsPipelineBuilder()
			.SetShader(fullscreenTriVS.get())
			.SetShader(blitPS.get())
			.AppendRTFormat(sc->GetBufferFormat())
			.Build());

		GPUGarbageBin bin(1);
		UploadContext upCtx(rd, 16'000, 1);

		struct TestData
		{
			float a, b, c, d;
		};

		struct SomeHandle { u64 handle; friend class TypedHandlePool; };
		GPUTableDeviceLocal<SomeHandle> table(rd, &bin, sizeof(TestData), 100);

		TestData initData{};
		initData.a = 0.2f;
		initData.b = 0.4f;
		initData.c = 0.6f;
		initData.d = 1.0f;
		auto dataHandle = table.Allocate(1, &initData);
		table.SendCopyRequests(upCtx);
		upCtx.SubmitCopies();


		GPUDynamicConstants constantsMan(rd, &bin, 100);

		GPUTableHostVisible<SomeHandle> hvTable(rd, &bin, sizeof(TestData), 100);
		auto hvHandle = hvTable.Allocate(1);

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
			auto& scView = scViews[sc->GetNextDrawSurfaceIdx()];
			auto& scPass = scPasses[sc->GetNextDrawSurfaceIdx()];

			rd->Flush();

			bin.BeginFrame();
			rd->RecycleCommandList(cmdl);
			cmdl = rd->AllocateCommandList();

			TestData initData{};
			initData.a = count == 0 ? 1.f : 0.f;
			initData.b = count == 1 ? 1.f : 0.f;
			initData.c = count == 2 ? 1.f : 0.f;
			initData.d = 1.f;

			hvTable.UpdateDirect(hvHandle, &initData, sizeof(initData));

			//table.RequestUpdate(dataHandle, &initData, sizeof(initData));
			//table.SendCopyRequests(upCtx);
			//upCtx.SubmitCopies();

			auto dynConst = constantsMan.Allocate();
			std::memcpy(dynConst.memory, &initData, sizeof(initData));

			// Write
			{
				GPUBarrier barrs[]
				{
					GPUBarrier::Transition(scTex, 0, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET)
				};
				rd->Cmd_Barrier(cmdl, barrs);
			}

			rd->Cmd_BeginRenderPass(cmdl, scPass);

			rd->Cmd_SetPipeline(cmdl, pipe);
			rd->Cmd_SetViewports(cmdl, Viewports()
				.Append(0.f, 0.f, (f32)Window::GetWidth(), (f32)Window::GetHeight()));
			rd->Cmd_SetScissorRects(cmdl, ScissorRects()
				.Append(0, 0, Window::GetWidth(), Window::GetHeight()));

			auto directHandle = table.GetLocalOffset(dataHandle);
			rd->Cmd_UpdateShaderArgs(cmdl, ShaderArgs()
				.AppendConstant(table.GetGlobalDescriptor())
				.AppendConstant(directHandle)
				.AppendConstant(dynConst.globalDescriptor)
				.AppendConstant(hvTable.GetGlobalDescriptor())
				.AppendConstant(hvTable.GetLocalOffset(hvHandle)));

			rd->Cmd_Draw(cmdl, 3, 1, 0, 0);

			rd->Cmd_EndRenderPass(cmdl);

			// Present
			{
				GPUBarrier barrs[]
				{
					GPUBarrier::Transition(scTex, 0, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT)
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
