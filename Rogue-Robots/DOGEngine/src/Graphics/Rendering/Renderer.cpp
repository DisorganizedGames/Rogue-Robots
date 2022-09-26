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
			m_globalMaterialTable.get(),
			m_bin.get());



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

		m_rgResMan = std::make_unique<RGResourceManager>(m_rd, m_bin.get());

		//TestRG();
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

	void Renderer::SubmitMesh(Mesh mesh, u32 submesh, MaterialHandle mat, const DirectX::SimpleMath::Matrix& world)
	{
		RenderSubmission sub{};
		sub.mesh = mesh;
		sub.submesh = submesh;
		sub.mat = mat;
		sub.world = world;
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

		// Write
		{
			GPUBarrier barrs[]
			{
				GPUBarrier::Transition(scTex, 0, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET),
				GPUBarrier::Transition(m_depthTex, 0, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE)
			};
			m_rd->Cmd_Barrier(m_cmdl, barrs);
		}

		m_rd->Cmd_BeginRenderPass(m_cmdl, scPass);

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
				.AppendConstant(m_globalMaterialTable->GetDescriptor())
				.AppendConstant(m_globalMaterialTable->GetMaterialIndex(sub.mat)
				);

			m_rd->Cmd_UpdateShaderArgs(m_cmdl, args);

			auto sm = m_globalMeshTable->GetSubmeshMD_CPU(sub.mesh, sub.submesh);
			m_rd->Cmd_DrawIndexed(m_cmdl, sm.indexCount, 1, sm.indexStart, 0, 0);

		}

		m_imgui->Render(m_rd, m_cmdl);

		m_rd->Cmd_EndRenderPass(m_cmdl);

		// Present
		{
			GPUBarrier barrs[]
			{
				GPUBarrier::Transition(scTex, 0, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT),
				GPUBarrier::Transition(m_depthTex, 0, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_COMMON)

			};
			m_rd->Cmd_Barrier(m_cmdl, barrs);
		}

		m_rd->SubmitCommandList(m_cmdl);


	
		{
			m_rg = std::move(std::make_unique<RenderGraph>(m_rd, m_rgResMan.get(), m_bin.get()));
			auto& rg = *m_rg;
		
			// GBuffer
			{
				struct PassData {};
				rg.AddPass<PassData>("GBuffer Pass",
					[&](PassData&, RenderGraph::PassBuilder& builder)
					{
						builder.DeclareTexture(RG_RESOURCE(GBufferPosition), RGTextureDesc::RenderTarget2D(DXGI_FORMAT_R8G8B8A8_UNORM, m_clientWidth, m_clientHeight));
						builder.DeclareTexture(RG_RESOURCE(GBufferNormal), RGTextureDesc::RenderTarget2D(DXGI_FORMAT_R8G8B8A8_UNORM, m_clientWidth, m_clientHeight));

						builder.WriteRenderTarget(RG_RESOURCE(GBufferPosition), RenderPassAccessType::Clear_Preserve,
							TextureViewDesc(ViewType::RenderTarget, TextureViewDimension::Texture2D, DXGI_FORMAT_R8G8B8A8_UNORM));
						builder.WriteRenderTarget(RG_RESOURCE(GBufferNormal), RenderPassAccessType::Clear_Preserve,
							TextureViewDesc(ViewType::RenderTarget, TextureViewDimension::Texture2D, DXGI_FORMAT_R8G8B8A8_UNORM));
					},
					[](const PassData&, RenderDevice* rd, CommandList cmdl, RenderGraph::PassResources& resources)
					{

					});
			}

			// Light pass to HDR
			{
				struct PassData {};
				rg.AddPass<PassData>("Light Pass to HDR",
					[&](PassData&, RenderGraph::PassBuilder& builder)
					{
						builder.DeclareTexture(RG_RESOURCE(LitHDR), RGTextureDesc::RenderTarget2D(DXGI_FORMAT_R16G16B16A16_FLOAT, m_clientWidth, m_clientHeight));

						builder.ReadResource(RG_RESOURCE(GBufferPosition), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
							TextureViewDesc(ViewType::ShaderResource, TextureViewDimension::Texture2D, DXGI_FORMAT_R8G8B8A8_UNORM));
						builder.ReadResource(RG_RESOURCE(GBufferNormal), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
							TextureViewDesc(ViewType::ShaderResource, TextureViewDimension::Texture2D, DXGI_FORMAT_R8G8B8A8_UNORM));
						builder.WriteRenderTarget(RG_RESOURCE(LitHDR), RenderPassAccessType::Clear_Preserve,
							TextureViewDesc(ViewType::RenderTarget, TextureViewDimension::Texture2D, DXGI_FORMAT_R16G16B16A16_FLOAT));
					},
					[](const PassData&, RenderDevice* rd, CommandList cmdl, RenderGraph::PassResources& resources)
					{
					});
			}

			// Do something arbitrary with GBuffer Positions
			{
				struct PassData {};
				rg.AddPass<PassData>("Read GBufferPosition",
					[&](PassData&, RenderGraph::PassBuilder& builder)
					{
						builder.DeclareTexture(RG_RESOURCE(ArbiOutput), RGTextureDesc::RenderTarget2D(DXGI_FORMAT_R8G8B8A8_UNORM, m_clientWidth, m_clientHeight));

						builder.ReadResource(RG_RESOURCE(GBufferPosition), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
							TextureViewDesc(ViewType::ShaderResource, TextureViewDimension::Texture2D, DXGI_FORMAT_R8G8B8A8_UNORM));
						builder.WriteRenderTarget(RG_RESOURCE(ArbiOutput), RenderPassAccessType::Clear_Preserve,
							TextureViewDesc(ViewType::RenderTarget, TextureViewDimension::Texture2D, DXGI_FORMAT_R8G8B8A8_UNORM));
					},
					[](const PassData&, RenderDevice* rd, CommandList cmdl, RenderGraph::PassResources& resources)
					{
					});
			}

			// Alias ArbiOutput
			{
				struct PassData {};
				rg.AddPass<PassData>("Alias ArbiOutput",
					[&](PassData&, RenderGraph::PassBuilder& builder)
					{
						builder.ProxyRead(RG_RESOURCE(ArbiReadDone));

						builder.WriteAliasedRenderTarget(RG_RESOURCE(ArbiOutput2), RG_RESOURCE(ArbiOutput), RenderPassAccessType::Clear_Preserve,
							TextureViewDesc(ViewType::RenderTarget, TextureViewDimension::Texture2D, DXGI_FORMAT_R8G8B8A8_UNORM));
					},
					[](const PassData&, RenderDevice* rd, CommandList cmdl, RenderGraph::PassResources& resources)
					{

					});
			}

			// Blit to LDR directly to backbuffer
			{
				struct PassData {};
				rg.AddPass<PassData>("Blit HDR to LDR Backbuffer",
					[&](PassData&, RenderGraph::PassBuilder& builder)
					{
						builder.ImportTexture(RG_RESOURCE(Backbuffer1), m_scTextures[0], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_PRESENT);

						builder.ProxyWrite(RG_RESOURCE(ArbiReadDone));
						builder.ReadResource(RG_RESOURCE(ArbiOutput), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
							TextureViewDesc(ViewType::ShaderResource, TextureViewDimension::Texture2D, DXGI_FORMAT_R16G16B16A16_FLOAT));
						builder.ReadResource(RG_RESOURCE(LitHDR), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
							TextureViewDesc(ViewType::ShaderResource, TextureViewDimension::Texture2D, DXGI_FORMAT_R16G16B16A16_FLOAT));
						builder.WriteRenderTarget(RG_RESOURCE(Backbuffer1), RenderPassAccessType::Clear_Preserve,
							TextureViewDesc(ViewType::RenderTarget, TextureViewDimension::Texture2D, DXGI_FORMAT_R8G8B8A8_UNORM));
					},
					[](const PassData&, RenderDevice* rd, CommandList cmdl, RenderGraph::PassResources& resources)
					{

					});
			}

			// Draw ImGUI on backbuffer
			{
				struct PassData {};
				rg.AddPass<PassData>("ImGUI Pass",
					[&](PassData&, RenderGraph::PassBuilder& builder)
					{
						builder.WriteAliasedRenderTarget(RG_RESOURCE(Backbuffer2), RG_RESOURCE(Backbuffer1), RenderPassAccessType::Preserve_Preserve,
							TextureViewDesc(ViewType::RenderTarget, TextureViewDimension::Texture2D, DXGI_FORMAT_R8G8B8A8_UNORM));
					},
					[](const PassData&, RenderDevice* rd, CommandList cmdl, RenderGraph::PassResources& resources)
					{

					});
			}

			// Draw 2D Renderer on backbuffer
			{
				struct PassData {};
				rg.AddPass<PassData>("2D Renderer Pass",
					[&](PassData&, RenderGraph::PassBuilder& builder)
					{
						builder.WriteAliasedRenderTarget(RG_RESOURCE(Backbuffer3), RG_RESOURCE(Backbuffer2), RenderPassAccessType::Preserve_Preserve,
							TextureViewDesc(ViewType::RenderTarget, TextureViewDimension::Texture2D, DXGI_FORMAT_R8G8B8A8_UNORM));
					},
					[](const PassData&, RenderDevice* rd, CommandList cmdl, RenderGraph::PassResources& resources)
					{

					});
			}

			rg.Build();
			rg.Execute();

		}


	
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

	void DOG::gfx::Renderer::TestRG()
	{
		TestRG_3();
	}

	void DOG::gfx::Renderer::TestRG_1()
	{
		RGResourceManager resMan(m_rd, m_bin.get());
		RenderGraph rg(m_rd, &resMan, m_bin.get());

		// Writing to same texture
		{
			struct PassData {};
			rg.AddPass<PassData>("Pass 1",
				[&](PassData&, RenderGraph::PassBuilder& builder)
				{
					//builder.DeclareTexture(RG_RESOURCE(out1), RGTextureDesc());
					builder.ImportTexture(RG_RESOURCE(out1), m_scTextures[0], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_PRESENT);
					builder.WriteRenderTarget(RG_RESOURCE(out1), RenderPassAccessType::Clear_Preserve,
						TextureViewDesc(ViewType::RenderTarget, TextureViewDimension::Texture2D, DXGI_FORMAT_R8G8B8A8_UNORM));
				},
				[](const PassData&, RenderDevice* rd, CommandList cmdl, RenderGraph::PassResources& resources)
				{
					resources.GetView(RG_RESOURCE(out1));
				});
		}

		{
			struct PassData {};
			rg.AddPass<PassData>("Pass 2",
				[&](PassData&, RenderGraph::PassBuilder& builder)
				{
					builder.WriteAliasedRenderTarget(RG_RESOURCE(out2), RG_RESOURCE(out1), RenderPassAccessType::Preserve_Preserve,
						TextureViewDesc(ViewType::RenderTarget, TextureViewDimension::Texture2D, DXGI_FORMAT_R8G8B8A8_UNORM));
				},
				[](const PassData&, RenderDevice* rd, CommandList cmdl, RenderGraph::PassResources& resources)
				{
				});
		}

		{
			struct PassData {};
			rg.AddPass<PassData>("Pass 3",
				[&](PassData&, RenderGraph::PassBuilder& builder)
				{
					builder.WriteAliasedRenderTarget(RG_RESOURCE(out3), RG_RESOURCE(out2), RenderPassAccessType::Preserve_Preserve,
						TextureViewDesc(ViewType::RenderTarget, TextureViewDimension::Texture2D, DXGI_FORMAT_R8G8B8A8_UNORM));
				},
				[](const PassData&, RenderDevice* rd, CommandList cmdl, RenderGraph::PassResources& resources)
				{

				});
		}

		rg.Build();
		rg.Execute();

		assert(false);
	}

	void DOG::gfx::Renderer::TestRG_2()
	{
		RGResourceManager resMan(m_rd, m_bin.get());
		RenderGraph rg(m_rd, &resMan, m_bin.get());

		// GBuffer
		{
			struct PassData {};
			rg.AddPass<PassData>("GBuffer Pass",
				[&](PassData&, RenderGraph::PassBuilder& builder)
				{
					builder.DeclareTexture(RG_RESOURCE(GBufferPosition), RGTextureDesc::RenderTarget2D(DXGI_FORMAT_R8G8B8A8_UNORM, m_clientWidth, m_clientHeight));
					builder.DeclareTexture(RG_RESOURCE(GBufferNormal), RGTextureDesc::RenderTarget2D(DXGI_FORMAT_R8G8B8A8_UNORM, m_clientWidth, m_clientHeight));

					builder.WriteRenderTarget(RG_RESOURCE(GBufferPosition), RenderPassAccessType::Clear_Preserve,
						TextureViewDesc(ViewType::RenderTarget, TextureViewDimension::Texture2D, DXGI_FORMAT_R8G8B8A8_UNORM));
					builder.WriteRenderTarget(RG_RESOURCE(GBufferNormal), RenderPassAccessType::Clear_Preserve,
						TextureViewDesc(ViewType::RenderTarget, TextureViewDimension::Texture2D, DXGI_FORMAT_R8G8B8A8_UNORM));
				},
				[](const PassData&, RenderDevice* rd, CommandList cmdl, RenderGraph::PassResources& resources)
				{
				
				});
		}

		// Light pass to HDR
		{
			struct PassData {};
			rg.AddPass<PassData>("Light Pass to HDR",
				[&](PassData&, RenderGraph::PassBuilder& builder)
				{
					builder.DeclareTexture(RG_RESOURCE(LitHDR), RGTextureDesc::RenderTarget2D(DXGI_FORMAT_R16G16B16A16_FLOAT, m_clientWidth, m_clientHeight));

					builder.ReadResource(RG_RESOURCE(GBufferPosition), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
						TextureViewDesc(ViewType::ShaderResource, TextureViewDimension::Texture2D, DXGI_FORMAT_R8G8B8A8_UNORM));
					builder.ReadResource(RG_RESOURCE(GBufferNormal), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
						TextureViewDesc(ViewType::ShaderResource, TextureViewDimension::Texture2D, DXGI_FORMAT_R8G8B8A8_UNORM));
					builder.WriteRenderTarget(RG_RESOURCE(LitHDR), RenderPassAccessType::Clear_Preserve,
						TextureViewDesc(ViewType::RenderTarget, TextureViewDimension::Texture2D, DXGI_FORMAT_R16G16B16A16_FLOAT));
				},
				[](const PassData&, RenderDevice* rd, CommandList cmdl, RenderGraph::PassResources& resources)
				{
				});
		}

		// Blit to LDR directly to backbuffer
		{
			struct PassData {};
			rg.AddPass<PassData>("Blit HDR to LDR Backbuffer",
				[&](PassData&, RenderGraph::PassBuilder& builder)
				{
					builder.ImportTexture(RG_RESOURCE(Backbuffer1), m_scTextures[0], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_PRESENT);

					builder.ReadResource(RG_RESOURCE(LitHDR), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
						TextureViewDesc(ViewType::ShaderResource, TextureViewDimension::Texture2D, DXGI_FORMAT_R16G16B16A16_FLOAT));
					builder.WriteRenderTarget(RG_RESOURCE(Backbuffer1), RenderPassAccessType::Clear_Preserve,
						TextureViewDesc(ViewType::RenderTarget, TextureViewDimension::Texture2D, DXGI_FORMAT_R8G8B8A8_UNORM));
				},
				[](const PassData&, RenderDevice* rd, CommandList cmdl, RenderGraph::PassResources& resources)
				{

				});
		}

		// Draw ImGUI on backbuffer
		{
			struct PassData {};
			rg.AddPass<PassData>("ImGUI Pass",
				[&](PassData&, RenderGraph::PassBuilder& builder)
				{
					builder.WriteAliasedRenderTarget(RG_RESOURCE(Backbuffer2), RG_RESOURCE(Backbuffer1), RenderPassAccessType::Preserve_Preserve,
						TextureViewDesc(ViewType::RenderTarget, TextureViewDimension::Texture2D, DXGI_FORMAT_R8G8B8A8_UNORM));
				},
				[](const PassData&, RenderDevice* rd, CommandList cmdl, RenderGraph::PassResources& resources)
				{

				});
		}

		// Draw 2D Renderer on backbuffer
		{
			struct PassData {};
			rg.AddPass<PassData>("2D Renderer Pass",
				[&](PassData&, RenderGraph::PassBuilder& builder)
				{
					builder.WriteAliasedRenderTarget(RG_RESOURCE(Backbuffer3), RG_RESOURCE(Backbuffer2), RenderPassAccessType::Preserve_Preserve,
						TextureViewDesc(ViewType::RenderTarget, TextureViewDimension::Texture2D, DXGI_FORMAT_R8G8B8A8_UNORM));
				},
				[](const PassData&, RenderDevice* rd, CommandList cmdl, RenderGraph::PassResources& resources)
				{

				});
		}

		rg.Build();
		rg.Execute();

		assert(false);
	}

	void DOG::gfx::Renderer::TestRG_3()
	{
		RGResourceManager resMan(m_rd, m_bin.get());
		RenderGraph rg(m_rd, &resMan, m_bin.get());

		// GBuffer
		{
			struct PassData {};
			rg.AddPass<PassData>("GBuffer Pass",
				[&](PassData&, RenderGraph::PassBuilder& builder)
				{
					builder.DeclareTexture(RG_RESOURCE(GBufferPosition), RGTextureDesc::RenderTarget2D(DXGI_FORMAT_R8G8B8A8_UNORM, m_clientWidth, m_clientHeight));
					builder.DeclareTexture(RG_RESOURCE(GBufferNormal), RGTextureDesc::RenderTarget2D(DXGI_FORMAT_R8G8B8A8_UNORM, m_clientWidth, m_clientHeight));

					builder.WriteRenderTarget(RG_RESOURCE(GBufferPosition), RenderPassAccessType::Clear_Preserve,
						TextureViewDesc(ViewType::RenderTarget, TextureViewDimension::Texture2D, DXGI_FORMAT_R8G8B8A8_UNORM));
					builder.WriteRenderTarget(RG_RESOURCE(GBufferNormal), RenderPassAccessType::Clear_Preserve,
						TextureViewDesc(ViewType::RenderTarget, TextureViewDimension::Texture2D, DXGI_FORMAT_R8G8B8A8_UNORM));
				},
				[](const PassData&, RenderDevice* rd, CommandList cmdl, RenderGraph::PassResources& resources)
				{

				});
		}

		// Light pass to HDR
		{
			struct PassData {};
			rg.AddPass<PassData>("Light Pass to HDR",
				[&](PassData&, RenderGraph::PassBuilder& builder)
				{
					builder.DeclareTexture(RG_RESOURCE(LitHDR), RGTextureDesc::RenderTarget2D(DXGI_FORMAT_R16G16B16A16_FLOAT, m_clientWidth, m_clientHeight));

					builder.ReadResource(RG_RESOURCE(GBufferPosition), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
						TextureViewDesc(ViewType::ShaderResource, TextureViewDimension::Texture2D, DXGI_FORMAT_R8G8B8A8_UNORM));
					builder.ReadResource(RG_RESOURCE(GBufferNormal), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
						TextureViewDesc(ViewType::ShaderResource, TextureViewDimension::Texture2D, DXGI_FORMAT_R8G8B8A8_UNORM));
					builder.WriteRenderTarget(RG_RESOURCE(LitHDR), RenderPassAccessType::Clear_Preserve,
						TextureViewDesc(ViewType::RenderTarget, TextureViewDimension::Texture2D, DXGI_FORMAT_R16G16B16A16_FLOAT));
				},
				[](const PassData&, RenderDevice* rd, CommandList cmdl, RenderGraph::PassResources& resources)
				{
				});
		}

		// Do something arbitrary with GBuffer Positions
		{
			struct PassData {};
			rg.AddPass<PassData>("Read GBufferPosition",
				[&](PassData&, RenderGraph::PassBuilder& builder)
				{
					builder.DeclareTexture(RG_RESOURCE(ArbiOutput), RGTextureDesc::RenderTarget2D(DXGI_FORMAT_R8G8B8A8_UNORM, m_clientWidth, m_clientHeight));

					builder.ReadResource(RG_RESOURCE(GBufferPosition), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
						TextureViewDesc(ViewType::ShaderResource, TextureViewDimension::Texture2D, DXGI_FORMAT_R8G8B8A8_UNORM));
					builder.WriteRenderTarget(RG_RESOURCE(ArbiOutput), RenderPassAccessType::Clear_Preserve,
						TextureViewDesc(ViewType::RenderTarget, TextureViewDimension::Texture2D, DXGI_FORMAT_R8G8B8A8_UNORM));
				},
				[](const PassData&, RenderDevice* rd, CommandList cmdl, RenderGraph::PassResources& resources)
				{
				});
		}

		// Alias ArbiOutput
		{
			struct PassData {};
			rg.AddPass<PassData>("Alias ArbiOutput",
				[&](PassData&, RenderGraph::PassBuilder& builder)
				{
					builder.ProxyRead(RG_RESOURCE(ArbiReadDone));

					builder.WriteAliasedRenderTarget(RG_RESOURCE(ArbiOutput2), RG_RESOURCE(ArbiOutput), RenderPassAccessType::Clear_Preserve,
						TextureViewDesc(ViewType::RenderTarget, TextureViewDimension::Texture2D, DXGI_FORMAT_R8G8B8A8_UNORM));
				},
				[](const PassData&, RenderDevice* rd, CommandList cmdl, RenderGraph::PassResources& resources)
				{
					
				});
		}

		// Blit to LDR directly to backbuffer
		{
			struct PassData {};
			rg.AddPass<PassData>("Blit HDR to LDR Backbuffer",
				[&](PassData&, RenderGraph::PassBuilder& builder)
				{
					builder.ImportTexture(RG_RESOURCE(Backbuffer1), m_scTextures[0], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_PRESENT);

					builder.ProxyWrite(RG_RESOURCE(ArbiReadDone));
					builder.ReadResource(RG_RESOURCE(ArbiOutput), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
						TextureViewDesc(ViewType::ShaderResource, TextureViewDimension::Texture2D, DXGI_FORMAT_R16G16B16A16_FLOAT));
					builder.ReadResource(RG_RESOURCE(LitHDR), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
						TextureViewDesc(ViewType::ShaderResource, TextureViewDimension::Texture2D, DXGI_FORMAT_R16G16B16A16_FLOAT));
					builder.WriteRenderTarget(RG_RESOURCE(Backbuffer1), RenderPassAccessType::Clear_Preserve,
						TextureViewDesc(ViewType::RenderTarget, TextureViewDimension::Texture2D, DXGI_FORMAT_R8G8B8A8_UNORM));
				},
				[](const PassData&, RenderDevice* rd, CommandList cmdl, RenderGraph::PassResources& resources)
				{

				});
		}

		// Draw ImGUI on backbuffer
		{
			struct PassData {};
			rg.AddPass<PassData>("ImGUI Pass",
				[&](PassData&, RenderGraph::PassBuilder& builder)
				{
					builder.WriteAliasedRenderTarget(RG_RESOURCE(Backbuffer2), RG_RESOURCE(Backbuffer1), RenderPassAccessType::Preserve_Preserve,
						TextureViewDesc(ViewType::RenderTarget, TextureViewDimension::Texture2D, DXGI_FORMAT_R8G8B8A8_UNORM));
				},
				[](const PassData&, RenderDevice* rd, CommandList cmdl, RenderGraph::PassResources& resources)
				{

				});
		}

		// Draw 2D Renderer on backbuffer
		{
			struct PassData {};
			rg.AddPass<PassData>("2D Renderer Pass",
				[&](PassData&, RenderGraph::PassBuilder& builder)
				{
					builder.WriteAliasedRenderTarget(RG_RESOURCE(Backbuffer3), RG_RESOURCE(Backbuffer2), RenderPassAccessType::Preserve_Preserve,
						TextureViewDesc(ViewType::RenderTarget, TextureViewDimension::Texture2D, DXGI_FORMAT_R8G8B8A8_UNORM));
				},
				[](const PassData&, RenderDevice* rd, CommandList cmdl, RenderGraph::PassResources& resources)
				{

				});
		}

		rg.Build();
		rg.Execute();

		assert(false);
	}

	void DOG::gfx::Renderer::TestRG_4()
	{
		// Test illegal cases with Aliasing (written in document)
	}

	LRESULT Renderer::WinProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		return m_imgui->WinProc(hwnd, uMsg, wParam, lParam);
	}

}
