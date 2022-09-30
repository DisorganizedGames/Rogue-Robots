#include "Renderer.h"

#include "../RHI/DX12/RenderBackend_DX12.h"
#include "../RHI/DX12/ImGUIBackend_DX12.h"
#include "../RHI/DX12/RenderDevice_DX12.h"
#include "../RHI/ShaderCompilerDXC.h"
#include "../RHI/PipelineBuilder.h"

#include "GPUGarbageBin.h"
#include "UploadContext.h"
#include "GPUTable.h"
#include "GPUDynamicConstants.h"
#include "MaterialTable.h"
#include "MeshTable.h"
#include "TextureManager.h"
#include "GraphicsBuilder.h"

#include "../../Core/AssimpImporter.h"
#include "../../Core/TextureFileImporter.h"

#include "RenderGraph/RenderGraph.h"
#include "RenderGraph/RGResourceManager.h"

#include "Tracy/Tracy.hpp"

namespace DOG::gfx
{
	Renderer::Renderer(HWND hwnd, u32 clientWidth, u32 clientHeight, bool debug) :
		m_clientWidth(clientWidth),
		m_clientHeight(clientHeight)
	{
		m_boneJourno = std::make_unique<AnimationManager>();
		m_backend = std::make_unique<gfx::RenderBackend_DX12>(debug);
		m_rd = m_backend->CreateDevice();
		m_sc = m_rd->CreateSwapchain(hwnd, (u8)S_NUM_BACKBUFFERS);
		m_imgui = std::make_unique<gfx::ImGUIBackend_DX12>(m_rd, m_sc, S_MAX_FIF);
		m_sclr = std::make_unique<ShaderCompilerDXC>();

		m_wmCallback = [this](HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
		{
			return WinProc(hwnd, uMsg, wParam, lParam);
		};

		const u32 maxUploadSizeDefault = 40'000'000;
		const u32 maxUploadSizeTextures = 400'000'000;

		m_bin = std::make_unique<GPUGarbageBin>(S_MAX_FIF);
		m_uploadCtx = std::make_unique<UploadContext>(m_rd, maxUploadSizeDefault, S_MAX_FIF);
		m_texUploadCtx = std::make_unique<UploadContext>(m_rd, maxUploadSizeTextures, S_MAX_FIF);


		const u32 maxConstantsPerFrame = 500 * 32;
		m_dynConstants = std::make_unique<GPUDynamicConstants>(m_rd, m_bin.get(), maxConstantsPerFrame);
		m_cmdl = m_rd->AllocateCommandList();

		// Startup
		MeshTable::MemorySpecification spec{};
		const u32 maxBytesPerAttribute = 4'000'000;
		const u32 maxNumberOfIndices = 1'000'000;
		const u32 maxTotalSubmeshes = 500;

		spec.maxSizePerAttribute[VertexAttribute::Position] = maxBytesPerAttribute;
		spec.maxSizePerAttribute[VertexAttribute::UV] = maxBytesPerAttribute;
		spec.maxSizePerAttribute[VertexAttribute::Normal] = maxBytesPerAttribute;
		spec.maxSizePerAttribute[VertexAttribute::Tangent] = maxBytesPerAttribute;
		spec.maxSizePerAttribute[VertexAttribute::BlendData] = maxBytesPerAttribute;
		spec.maxTotalSubmeshes = maxTotalSubmeshes;
		spec.maxNumIndices = maxNumberOfIndices;
		m_globalMeshTable = std::make_unique<MeshTable>(m_rd, m_bin.get(), spec);

		MaterialTable::MemorySpecification memSpec{};
		memSpec.maxElements = 500;	// 500 distinct materials
		m_globalMaterialTable = std::make_unique<MaterialTable>(m_rd, m_bin.get(), memSpec);

		// Create builder for users to create graphical objects supported by the renderer
		m_builder = std::make_unique<GraphicsBuilder>(
			m_rd,
			m_uploadCtx.get(),
			m_texUploadCtx.get(),
			m_globalMeshTable.get(),
			m_globalMaterialTable.get(),
			m_bin.get());



		m_texMan = std::make_unique<TextureManager>(m_rd, m_bin.get());

		// INITIALIZE RESOURCES =================

		auto fullscreenTriVS = m_sclr->CompileFromFile("FullscreenTriVS.hlsl", ShaderType::Vertex);
		auto blitPS = m_sclr->CompileFromFile("BlitPS.hlsl", ShaderType::Pixel);
		m_pipe = m_rd->CreateGraphicsPipeline(GraphicsPipelineBuilder()
			.SetShader(fullscreenTriVS.get())
			.SetShader(blitPS.get())
			.AppendRTFormat(m_sc->GetBufferFormat())
			//.SetDepthFormat(DepthFormat::D32)
			//.SetDepthStencil(DepthStencilBuilder().SetDepthEnabled(true))
			.Build());

		auto meshVS = m_sclr->CompileFromFile("MainVS.hlsl", ShaderType::Vertex);
		auto meshPS = m_sclr->CompileFromFile("MainPS.hlsl", ShaderType::Pixel);
		m_meshPipe = m_rd->CreateGraphicsPipeline(GraphicsPipelineBuilder()
			.SetShader(meshVS.get())
			.SetShader(meshPS.get())
			.AppendRTFormat(DXGI_FORMAT_R16G16B16A16_FLOAT)
			.SetDepthFormat(DepthFormat::D32)
			.SetDepthStencil(DepthStencilBuilder().SetDepthEnabled(true))
			.Build());

		m_rgResMan = std::make_unique<RGResourceManager>(m_rd, m_bin.get());

		auto testCS = m_sclr->CompileFromFile("TestComputeCS.hlsl", ShaderType::Compute);
		m_testCompPipe = m_rd->CreateComputePipeline(ComputePipelineDesc(testCS.get()));

		

	}

	Renderer::~Renderer()
	{
		Flush();
	}

	void Renderer::SetMainRenderCamera(const DirectX::XMMATRIX& view, DirectX::XMMATRIX* proj)
	{
		m_viewMat = view;
		m_projMat = proj ? *proj : DirectX::XMMatrixPerspectiveFovLH(80.f * 3.1415f / 180.f, (f32)m_clientWidth / m_clientHeight, 800.f, 0.1f);
	}

	void Renderer::BeginGUI()
	{
		m_imgui->BeginFrame();
	}

	void Renderer::SubmitMesh(Mesh mesh, u32 submesh, MaterialHandle mat, const DirectX::SimpleMath::Matrix& world)
	{
		RenderSubmission sub{};
		sub.mesh = mesh;
		sub.submesh = submesh;
		sub.mat = mat;
		sub.world = world;
		m_submissions.push_back(sub);
	}

	void Renderer::Update(f32)
	{
		m_boneJourno->UpdateJoints();
	}

	void Renderer::Render(f32)
	{
		ZoneScopedN("Render");

		std::chrono::steady_clock::time_point m_start, m_end;
		std::chrono::duration<double> diff;

		m_rg = std::move(std::make_unique<RenderGraph>(m_rd, m_rgResMan.get(), m_bin.get()));
		auto& rg = *m_rg;

		m_start = m_end = std::chrono::high_resolution_clock::now();
		// Forward pass to HDR
		{
			struct PassData {};
			rg.AddPass<PassData>("Forward Pass",
				[&](PassData&, RenderGraph::PassBuilder& builder)
				{
					builder.DeclareTexture(RG_RESOURCE(MainDepth), RGTextureDesc::DepthWrite2D(DepthFormat::D32, m_clientWidth, m_clientHeight));
					builder.DeclareTexture(RG_RESOURCE(LitHDR), RGTextureDesc::RenderTarget2D(DXGI_FORMAT_R16G16B16A16_FLOAT, m_clientWidth, m_clientHeight)
						.AddFlag(D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS));

					builder.WriteRenderTarget(RG_RESOURCE(LitHDR), RenderPassAccessType::ClearPreserve,
						TextureViewDesc(ViewType::RenderTarget, TextureViewDimension::Texture2D, DXGI_FORMAT_R16G16B16A16_FLOAT));
					builder.ReadOrWriteDepth(RG_RESOURCE(MainDepth), RenderPassAccessType::ClearDiscard,
						TextureViewDesc(ViewType::DepthStencil, TextureViewDimension::Texture2D, DXGI_FORMAT_D32_FLOAT));
				},
				[&](const PassData&, RenderDevice* rd, CommandList cmdl, RenderGraph::PassResources&)
				{
					rd->Cmd_SetViewports(cmdl, Viewports()
						.Append(0.f, 0.f, (f32)m_clientWidth, (f32)m_clientHeight));
					rd->Cmd_SetScissorRects(cmdl, ScissorRects()
						.Append(0, 0, m_clientWidth, m_clientHeight));

					rd->Cmd_SetPipeline(cmdl, m_meshPipe);
					rd->Cmd_SetIndexBuffer(cmdl, m_globalMeshTable->GetIndexBuffer());
					for (const auto& sub : m_submissions)
					{
						auto pfConstant = m_dynConstants->Allocate(32);
						struct PerFrameData
						{
							DirectX::XMMATRIX world, view, proj;
							DirectX::XMFLOAT4 camPos;
							DirectX::XMFLOAT4X4 joints[130];
						} pfData{};

						for (size_t i = 0; i < m_boneJourno->m_vsJoints.size(); i++)
							pfData.joints[i] = m_boneJourno->m_vsJoints[i];

						DirectX::XMVECTOR tmp;
						auto invVm = DirectX::XMMatrixInverse(&tmp, m_viewMat);

						auto pos = invVm.r[3];
						DirectX::XMFLOAT3 posFloat3;
						DirectX::XMStoreFloat3(&posFloat3, pos);
						pfData.camPos = { posFloat3.x, posFloat3.y, posFloat3.z, 0.0f };

						pfData.world = sub.world;
						//pfData.view = DirectX::XMMatrixLookAtLH({ 5.f, 2.f, 0.f }, { -1.f, 1.f, 1.f }, { 0.f, 1.f, 0.f });
						pfData.view = m_viewMat;
						// We are using REVERSE DEPTH!!!
						//pfData.proj = DirectX::XMMatrixPerspectiveFovLH(80.f * 3.1415f / 180.f, (f32)m_clientWidth/m_clientHeight, 800.f, 0.1f);
						pfData.proj = m_projMat;
						std::memcpy(pfConstant.memory, &pfData, sizeof(pfData));

						auto args = ShaderArgs()
							.AppendConstant(pfConstant.globalDescriptor)
							.AppendConstant(m_globalMeshTable->GetSubmeshMD_GPU(sub.mesh, sub.submesh))
							.AppendConstant(m_globalMeshTable->GetSubmeshDescriptor())
							.AppendConstant(m_globalMeshTable->GetAttributeDescriptor(VertexAttribute::Position))
							.AppendConstant(m_globalMeshTable->GetAttributeDescriptor(VertexAttribute::UV))
							.AppendConstant(m_globalMeshTable->GetAttributeDescriptor(VertexAttribute::Normal))
							.AppendConstant(m_globalMeshTable->GetAttributeDescriptor(VertexAttribute::Tangent))
							.AppendConstant(m_globalMeshTable->GetAttributeDescriptor(VertexAttribute::BlendData))
							.AppendConstant(m_globalMaterialTable->GetDescriptor())
							.AppendConstant(m_globalMaterialTable->GetMaterialIndex(sub.mat)
							);

						rd->Cmd_UpdateShaderArgs(cmdl, QueueType::Graphics, args);

						auto sm = m_globalMeshTable->GetSubmeshMD_CPU(sub.mesh, sub.submesh);
						rd->Cmd_DrawIndexed(cmdl, sm.indexCount, 1, sm.indexStart, 0, 0);

					}
				});
		}

		// Blit HDR to LDR
		{
			struct PassData {};
			rg.AddPass<PassData>("Blit to HDR Pass",
				[&](PassData&, RenderGraph::PassBuilder& builder)
				{
					builder.ImportTexture(RG_RESOURCE(Backbuffer), m_sc->GetNextDrawSurface(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_PRESENT);

					builder.ReadResource(RG_RESOURCE(LitHDR), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
						TextureViewDesc(ViewType::ShaderResource, TextureViewDimension::Texture2D, DXGI_FORMAT_R16G16B16A16_FLOAT));
					builder.WriteRenderTarget(RG_RESOURCE(Backbuffer), RenderPassAccessType::ClearPreserve,
						TextureViewDesc(ViewType::RenderTarget, TextureViewDimension::Texture2D, DXGI_FORMAT_R8G8B8A8_UNORM));
				},
				[&](const PassData&, RenderDevice* rd, CommandList cmdl, RenderGraph::PassResources& resources)
				{
					rd->Cmd_SetPipeline(cmdl, m_pipe);
					rd->Cmd_UpdateShaderArgs(cmdl, QueueType::Graphics, ShaderArgs()
						.AppendConstant(resources.GetView(RG_RESOURCE(LitHDR))));
					rd->Cmd_Draw(cmdl, 3, 1, 0, 0);
				});

		}


		// Draw ImGUI on backbuffer
		{
			struct PassData {};
			rg.AddPass<PassData>("ImGUI Pass",
				[&](PassData&, RenderGraph::PassBuilder& builder)
				{
					builder.WriteRenderTarget(RG_RESOURCE(Backbuffer), RenderPassAccessType::PreservePreserve,
						TextureViewDesc(ViewType::RenderTarget, TextureViewDimension::Texture2D, DXGI_FORMAT_R8G8B8A8_UNORM));
				},
				[&](const PassData&, RenderDevice* rd, CommandList cmdl, RenderGraph::PassResources&)
				{
					m_imgui->Render(rd, cmdl);
				});
		}







		
		m_end = std::chrono::high_resolution_clock::now();
		diff = m_end - m_start;
		//std::cout << "Graph declaration time elapsed (ms): " << diff.count() * 1000.0 << "\n";

		m_start = m_end = std::chrono::high_resolution_clock::now();
		rg.Build();
		m_end = std::chrono::high_resolution_clock::now();
		diff = m_end - m_start;
		//std::cout << "Build time elapsed (ms): " << diff.count() * 1000.0 << "\n";

		m_start = m_end = std::chrono::high_resolution_clock::now();
		rg.Execute();
		m_end = std::chrono::high_resolution_clock::now();
		diff = m_end - m_start;
		//std::cout << "Exec time elapsed (ms): " << diff.count() * 1000.0 << "\n\n";

	}

	void Renderer::OnResize(u32 clientWidth, u32 clientHeight)
	{
		// If same client width/height --> Ignore
		if (clientWidth == m_clientWidth && m_clientHeight == clientHeight)
			return;

		m_clientWidth = clientWidth;
		m_clientHeight = clientHeight;

		// Recreate resources if needed..
	}

	void Renderer::BeginFrame_GPU()
	{
		m_rd->Flush();

		m_rgResMan->Tick();
		m_bin->BeginFrame();
		m_rd->RecycleCommandList(m_cmdl);
		m_cmdl = m_rd->AllocateCommandList();
	}

	void Renderer::EndFrame_GPU(bool vsync)
	{
		EndGUI();
		m_bin->EndFrame();
		m_submissions.clear();

		m_sc->Present(vsync);
	}

	void Renderer::Flush()
	{
		m_rd->Flush();
		m_bin->ForceClear();
	}

	void Renderer::EndGUI()
	{
		m_imgui->EndFrame();
	}

	LRESULT Renderer::WinProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		if (m_imgui)
			return m_imgui->WinProc(hwnd, uMsg, wParam, lParam);
		else
			return false;
	}

}
