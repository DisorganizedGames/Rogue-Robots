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

#include "RenderGraph/RGResourceRepo.h"
#include "RenderGraph/RenderGraph.h"
#include "RenderGraph/RGBlackboard.h"



namespace DOG::gfx
{
	Renderer::Renderer(HWND hwnd, u32 clientWidth, u32 clientHeight, bool debug) :
		m_clientWidth(clientWidth),
		m_clientHeight(clientHeight)
	{
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

		m_rgResourceRepo = std::make_unique<RGResourceRepo>(m_rd, m_bin.get());
		m_rg = std::make_unique<RenderGraph>(m_rd, m_rgResourceRepo.get());
		m_rgBlackboard = std::make_unique<RGBlackboard>();

		



		const u32 maxConstantsPerFrame = 500;
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
			m_globalMaterialTable.get());



		m_texMan = std::make_unique<TextureManager>(m_rd, m_bin.get());



		// INITIALIZE RESOURCES =================



		// Create depth
		{
			TextureDesc d(MemoryType::Default, DXGI_FORMAT_D32_FLOAT, clientWidth, clientHeight, 1, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
			TextureViewDesc tvd(ViewType::DepthStencil, TextureViewDimension::Texture2D, DXGI_FORMAT_D32_FLOAT);
			m_depthTex = m_rd->CreateTexture(d);
			m_depthTarget = m_rd->CreateView(m_depthTex, tvd);
		}

		for (u8 i = 0; i < S_NUM_BACKBUFFERS; ++i)
		{
			// Grab textures
			m_scTextures[i] = m_sc->GetBuffer(i);

			// Create texture as target
			auto td = TextureViewDesc(ViewType::RenderTarget, TextureViewDimension::Texture2D, m_sc->GetBufferFormat());
			m_scViews[i] = m_rd->CreateView(m_scTextures[i], td);

			m_scPasses[i] = m_rd->CreateRenderPass(RenderPassBuilder()
				.AppendRT(m_scViews[i], RenderPassBeginAccessType::Clear, RenderPassEndingAccessType::Preserve)
				.AddDepth(m_depthTarget, RenderPassBeginAccessType::Clear, RenderPassEndingAccessType::Discard)
				.Build());
		}

		auto fullscreenTriVS = m_sclr->CompileFromFile("FullscreenTriVS.hlsl", ShaderType::Vertex);
		auto blitPS = m_sclr->CompileFromFile("BlitPS.hlsl", ShaderType::Pixel);
		m_pipe = m_rd->CreateGraphicsPipeline(GraphicsPipelineBuilder()
			.SetShader(fullscreenTriVS.get())
			.SetShader(blitPS.get())
			.AppendRTFormat(m_sc->GetBufferFormat())
			.SetDepthFormat(DepthFormat::D32)
			.SetDepthStencil(DepthStencilBuilder().SetDepthEnabled(true))
			.Build());

		auto meshVS = m_sclr->CompileFromFile("MainVS.hlsl", ShaderType::Vertex);
		auto meshPS = m_sclr->CompileFromFile("MainPS.hlsl", ShaderType::Pixel);
		m_meshPipe = m_rd->CreateGraphicsPipeline(GraphicsPipelineBuilder()
			.SetShader(meshVS.get())
			.SetShader(meshPS.get())
			.AppendRTFormat(m_sc->GetBufferFormat())
			.SetDepthFormat(DepthFormat::D32)
			.SetDepthStencil(DepthStencilBuilder().SetDepthEnabled(true))
			.Build());



		TestRenderGraph();

	}

	Renderer::~Renderer()
	{
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

	void Renderer::SubmitMesh(Mesh mesh, u32 submesh, MaterialHandle mat)
	{
		RenderSubmission sub{};
		sub.mesh = mesh;
		sub.submesh = submesh;
		sub.mat = mat;
		m_submissions.push_back(sub);
	}

	void Renderer::Update(f32 )
	{
	}

	void Renderer::Render(f32 )
	{
		// ====== GPU
		auto& scTex = m_scTextures[m_sc->GetNextDrawSurfaceIdx()];
		auto& scPass = m_scPasses[m_sc->GetNextDrawSurfaceIdx()];

		// Replace original render graph and make a new one
		m_rg = std::move(std::make_unique<RenderGraph>(m_rd, m_rgResourceRepo.get()));

		auto bb = m_rgResourceRepo->ImportResource(scTex, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_PRESENT);
		auto depth = m_rgResourceRepo->ImportResource(m_depthTex, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COMMON);
		// Forward pass
		{
			struct PassData
			{
				RGResourceView bbOut;
				RGResourceView depthOut;
			};
			m_rg->AddPass<PassData>("Forward",
				[&](RenderGraph::PassBuilder& builder, PassData& passData)
				{
					passData.bbOut = builder.WriteTexture(bb, D3D12_RESOURCE_STATE_RENDER_TARGET,
						TextureViewDesc(
							ViewType::RenderTarget, TextureViewDimension::Texture2D, DXGI_FORMAT_R8G8B8A8_UNORM));

					passData.depthOut = builder.WriteTexture(depth, D3D12_RESOURCE_STATE_DEPTH_WRITE,
						TextureViewDesc(
							ViewType::DepthStencil, TextureViewDimension::Texture2D, DXGI_FORMAT_D32_FLOAT));
				},
				[this](RenderDevice* rd, CommandList cmdl, RenderGraph::PassResources& resources, const PassData& passData)
				{
					std::cout << "Doing pass forward\n";

					m_rd->Cmd_SetViewports(m_cmdl, Viewports()
						.Append(0.f, 0.f, (f32)m_clientWidth, (f32)m_clientHeight));
					m_rd->Cmd_SetScissorRects(m_cmdl, ScissorRects()
						.Append(0, 0, m_clientWidth, m_clientHeight));

					m_rd->Cmd_SetPipeline(m_cmdl, m_meshPipe);
					m_rd->Cmd_SetIndexBuffer(m_cmdl, m_globalMeshTable->GetIndexBuffer());
					for (const auto& sub : m_submissions)
					{
						auto pfConstant = m_dynConstants->Allocate();
						struct PerFrameData
						{
							DirectX::XMMATRIX world, view, proj;
							DirectX::XMFLOAT3 camPos;
						} pfData{};

						DirectX::XMVECTOR tmp;
						auto invVm = DirectX::XMMatrixInverse(&tmp, m_viewMat);

						auto pos = invVm.r[3];
						DirectX::XMFLOAT3 posFloat3;
						DirectX::XMStoreFloat3(&posFloat3, pos);
						pfData.camPos = posFloat3;

						pfData.world = DirectX::XMMatrixScaling(0.07f, 0.07f, 0.07f);
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
							.AppendConstant(m_globalMaterialTable->GetDescriptor())
							.AppendConstant(m_globalMaterialTable->GetMaterialIndex(sub.mat)
							);

						m_rd->Cmd_UpdateShaderArgs(m_cmdl, args);

						auto sm = m_globalMeshTable->GetSubmeshMD_CPU(sub.mesh, sub.submesh);
						m_rd->Cmd_DrawIndexed(m_cmdl, sm.indexCount, 1, sm.indexStart, 0, 0);

					}
				});
		}
		// GUI pass
		{
			struct PassData
			{
				RGResourceView bbIn;
				RGResourceView depthIn;
			};
			m_rg->AddPass<PassData>("GUI",
				[&](RenderGraph::PassBuilder& builder, PassData& passData)
				{
					passData.bbIn = builder.WriteTexture(bb, D3D12_RESOURCE_STATE_RENDER_TARGET,
						TextureViewDesc(
							ViewType::RenderTarget, TextureViewDimension::Texture2D, DXGI_FORMAT_R8G8B8A8_UNORM));
				},
				[this](RenderDevice* rd, CommandList cmdl, RenderGraph::PassResources& resources, const PassData& passData)
				{
					std::cout << "Doing GUI pass\n";
					m_imgui->Render(m_rd, m_cmdl);
				});

		}

		m_rg->Build();
		m_rg->Run();

	
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

		m_rgResourceRepo->Tick();
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

	bool Renderer::WinProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		return m_imgui->WinProc(hwnd, uMsg, wParam, lParam);
	}

	void DOG::gfx::Renderer::TestRenderGraph()
	{
		//RGResourceRepo rgRepo(m_rd, m_bin.get());
		//RenderGraph rg(m_rd, &rgRepo);
		//RGBlackboard blackboard;

		//RGTextureDesc desc{};
		//desc.format = DXGI_FORMAT_R8G8B8A8_UNORM;
		//desc.flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		//desc.initState = D3D12_RESOURCE_STATE_RENDER_TARGET;
		//auto outputPass1 = rgRepo.DeclareResource(desc);
		//{	
		//	struct PassData
		//	{
		//		RGResourceView output;
		//	};
		//	rg.AddPass<PassData>("Forward",
		//		[&](RenderGraph::PassBuilder& builder, PassData& passData)
		//		{
		//			passData.output = builder.WriteTexture(outputPass1, D3D12_RESOURCE_STATE_RENDER_TARGET,
		//				TextureViewDesc(
		//					ViewType::RenderTarget, TextureViewDimension::Texture2D, DXGI_FORMAT_R8G8B8A8_UNORM));
		//		},
		//		[](RenderDevice* rd, CommandList cmdl, RenderGraph::PassResources& resources, const PassData& passData)
		//		{
		//			
		//			std::cout << "Doing pass forward\n";
		//		});
		//}

		//auto bb = rgRepo.ImportResource(m_scTextures[0], D3D12_RESOURCE_STATE_PRESENT);
		//{

		//	struct PassData
		//	{
		//		RGResourceView input;
		//		RGResourceView output;
		//	};
		//	rg.AddPass<PassData>("Post-proc blit",
		//		[&](RenderGraph::PassBuilder& builder, PassData& passData)
		//		{
		//			passData.input = builder.ReadTexture(outputPass1, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		//				TextureViewDesc(
		//					ViewType::ShaderResource, TextureViewDimension::Texture2D, DXGI_FORMAT_R8G8B8A8_UNORM));

		//			passData.output = builder.WriteTexture(bb, D3D12_RESOURCE_STATE_RENDER_TARGET,
		//				TextureViewDesc(
		//					ViewType::RenderTarget, TextureViewDimension::Texture2D, DXGI_FORMAT_R8G8B8A8_UNORM));
		//		},
		//		[](RenderDevice* rd, CommandList cmdl, RenderGraph::PassResources& resources, const PassData& passData)
		//		{
		//			std::cout << "Doing pass Post-proc blit\n";
		//		});
		//}
		//{
		//	struct PassData
		//	{
		//		RGResourceView output;
		//	};
		//	rg.AddPass<PassData>("ImGUI pass",
		//		[&](RenderGraph::PassBuilder& builder, PassData& passData)
		//		{
		//			passData.output = builder.WriteTexture(bb, D3D12_RESOURCE_STATE_RENDER_TARGET,
		//				TextureViewDesc(
		//					ViewType::RenderTarget, TextureViewDimension::Texture2D, DXGI_FORMAT_R8G8B8A8_UNORM));
		//		},
		//		[](RenderDevice* rd, CommandList cmdl, RenderGraph::PassResources& resources, const PassData& passData)
		//		{
		//			std::cout << "Doing pass ImGUI\n";
		//		});
		//}
		//{
		//	struct PassData
		//	{
		//		RGResourceView output;
		//	};
		//	rg.AddPass<PassData>("2D GUI pass",
		//		[&](RenderGraph::PassBuilder& builder, PassData& passData)
		//		{
		//			passData.output = builder.WriteTexture(bb, D3D12_RESOURCE_STATE_RENDER_TARGET,
		//				TextureViewDesc(
		//					ViewType::RenderTarget, TextureViewDimension::Texture2D, DXGI_FORMAT_R8G8B8A8_UNORM));
		//		},
		//		[](RenderDevice* rd, CommandList cmdl, RenderGraph::PassResources& resources, const PassData& passData)
		//		{
		//			std::cout << "Doing pass 2D GUI\n";
		//		});
		//}


		//rg.Build();
		//rg.Run();

	}

}
