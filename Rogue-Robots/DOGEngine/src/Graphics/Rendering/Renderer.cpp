#include "Renderer.h"

#include "../RHI/DX12/RenderBackend_DX12.h"
#include "../RHI/DX12/ImGUIBackend_DX12.h"
#include "../RHI/DX12/RenderDevice_DX12.h"
#include "../RHI/DX12/Swapchain_DX12.h"
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
#include "RenderGraph/RGBlackboard.h"

#include "Tracy/Tracy.hpp"

// Passes
#include "RenderEffects/ImGUIEffect.h"
#include "RenderEffects/TestComputeEffect.h"

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

		// For internal per frame management
		const u32 maxUploadPerFrame = 512'000;
		m_perFrameUploadCtx = std::make_unique<UploadContext>(m_rd, maxUploadPerFrame, S_MAX_FIF);



		const u32 maxConstantsPerFrame = 1000;
		m_dynConstants = std::make_unique<GPUDynamicConstants>(m_rd, m_bin.get(), maxConstantsPerFrame);

		// multiple of curr loaded mixamo skeleton
		m_dynConstantsAnimated = std::make_unique<GPUDynamicConstants>(m_rd, m_bin.get(), 33 * 5);		
		m_cmdl = m_rd->AllocateCommandList();

		// Startup
		MeshTable::MemorySpecification spec{};
		const u32 maxBytesPerAttribute = 4'000'000;
		const u32 maxNumberOfIndices = 1'000'000;
		const u32 maxTotalSubmeshes = 500;
		const u32 maxMaterialArgs = 1000;

		spec.maxSizePerAttribute[VertexAttribute::Position] = maxBytesPerAttribute;
		spec.maxSizePerAttribute[VertexAttribute::UV] = maxBytesPerAttribute;
		spec.maxSizePerAttribute[VertexAttribute::Normal] = maxBytesPerAttribute;
		spec.maxSizePerAttribute[VertexAttribute::Tangent] = maxBytesPerAttribute;
		spec.maxSizePerAttribute[VertexAttribute::BlendData] = maxBytesPerAttribute;
		spec.maxTotalSubmeshes = maxTotalSubmeshes;
		spec.maxNumIndices = maxNumberOfIndices;
		m_globalMeshTable = std::make_unique<MeshTable>(m_rd, m_bin.get(), spec);

		MaterialTable::MemorySpecification memSpec{};
		memSpec.maxElements = maxMaterialArgs;
		m_globalMaterialTable = std::make_unique<MaterialTable>(m_rd, m_bin.get(), memSpec);

		// Create builder for users to create graphical objects supported by the renderer
		m_builder = std::make_unique<GraphicsBuilder>(
			m_rd,
			m_uploadCtx.get(),
			m_texUploadCtx.get(),
			m_globalMeshTable.get(),
			m_globalMaterialTable.get(),
			m_bin.get());


		// INITIALIZE RESOURCES ================= Globals for now..
		// Refer to TestComputeEffect for Effect local resource building
		auto fullscreenTriVS = m_sclr->CompileFromFile("FullscreenTriVS.hlsl", ShaderType::Vertex);
		auto blitPS = m_sclr->CompileFromFile("BlitPS.hlsl", ShaderType::Pixel);
		m_pipe = m_rd->CreateGraphicsPipeline(GraphicsPipelineBuilder()
			.SetShader(fullscreenTriVS.get())
			.SetShader(blitPS.get())
			.AppendRTFormat(m_sc->GetBufferFormat())
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

		m_meshPipeNoCull = m_rd->CreateGraphicsPipeline(GraphicsPipelineBuilder()
			.SetShader(meshVS.get())
			.SetShader(meshPS.get())
			.AppendRTFormat(DXGI_FORMAT_R16G16B16A16_FLOAT)
			.SetDepthFormat(DepthFormat::D32)
			.SetDepthStencil(DepthStencilBuilder().SetDepthEnabled(true))
			.SetRasterizer(RasterizerBuilder().SetCullMode(D3D12_CULL_MODE_NONE))
			.Build());

		m_rgResMan = std::make_unique<RGResourceManager>(m_rd, m_bin.get());






		// Setup per frame
		m_pfDataTable = std::make_unique<GPUTableDeviceLocal<PfDataHandle>>(m_rd, m_bin.get(), (u32)sizeof(PerFrameData), S_MAX_FIF + 1, false);
		m_pfHandle = m_pfDataTable->Allocate(1, &m_pfData);


		// Setup persistent renderer data
		m_globalData.meshTableSubmeshMD = m_globalMeshTable->GetSubmeshDescriptor();
		m_globalData.meshTablePos = m_globalMeshTable->GetAttributeDescriptor(VertexAttribute::Position);
		m_globalData.meshTableUV = m_globalMeshTable->GetAttributeDescriptor(VertexAttribute::UV);
		m_globalData.meshTableNor = m_globalMeshTable->GetAttributeDescriptor(VertexAttribute::Normal);
		m_globalData.meshTableTan = m_globalMeshTable->GetAttributeDescriptor(VertexAttribute::Tangent);
		m_globalData.meshTableBlend = m_globalMeshTable->GetAttributeDescriptor(VertexAttribute::BlendData);
		m_globalData.perFrameTable = m_pfDataTable->GetGlobalDescriptor();
		m_globalData.materialTable = m_globalMaterialTable->GetDescriptor();

		m_globalDataTable = std::make_unique<GPUTableDeviceLocal<GlobalDataHandle>>(m_rd, m_bin.get(), (u32)sizeof(GlobalData), 1, false);
		m_gdHandle = m_globalDataTable->Allocate(1, &m_globalData);
		m_globalDataTable->SendCopyRequests(*m_uploadCtx);


		// Set default global pass data
		m_globalEffectData.bbScissor = ScissorRects().Append(0, 0, m_clientWidth, m_clientHeight);
		m_globalEffectData.bbVP = Viewports().Append(0.f, 0.f, (f32)m_clientWidth, (f32)m_clientHeight);
		// render vps/scissors subject to change
		m_globalEffectData.defRenderScissors = ScissorRects().Append(0, 0, m_clientWidth, m_clientHeight);
		m_globalEffectData.defRenderVPs= Viewports().Append(0.f, 0.f, (f32)m_clientWidth, (f32)m_clientHeight);
		m_globalEffectData.globalDataDescriptor = m_globalDataTable->GetGlobalDescriptor();
		m_globalEffectData.meshTable = m_globalMeshTable.get();

		// Setup blackboard for potential Effect-intercom
		m_rgBlackboard = std::make_unique<RGBlackboard>();


		// Define Passes
		/*
			Remember that the connections between passes are simply resource names.

			You can see how the graph looks like by uncommenting the GENERATE_GRAPHVIZ define in RenderGraph.h.
			you can then find rendergraph.txt in the Assets folder and simply copy paste it to Graphviz Online.
			The graph is to be traversed in topological order, so you can then verify that everything is executed
			in the order you expect them to.
			
			Remember that you do not have to immediately create an Effect class to play around.
			You can simply define passes as lambdas just like Forward Pass in the Render function to get acquainted
			(i.e try defining a few passes with only read/write declarations to see if the generated graph is as expected!)
			
		*/
		m_imGUIEffect = std::make_unique<ImGUIEffect>(m_globalEffectData, *m_rgBlackboard, m_imgui.get());
		m_testComputeEffect = std::make_unique<TestComputeEffect>(m_globalEffectData, *m_rgBlackboard, m_rd, m_sclr.get());
	}

	Renderer::~Renderer()
	{
		Flush();
		m_sc->SetFullscreenState(false, {}); // safeguard to prevent crash if game has not exited fullscreen before exit
	}

	Monitor Renderer::GetMonitor() const
	{
		return m_rd->GetMonitor();
	}

	DXGI_MODE_DESC Renderer::GetDefaultDisplayMode() const
	{
		// Abstract DXGI_MODE_DESC should be created later, for now use dxgi
		return static_cast<Swapchain_DX12*>(m_sc)->GetDefaultDisplayModeDesc();
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

	void Renderer::SubmitMesh(Mesh mesh, u32 submesh, MaterialHandle material, const DirectX::SimpleMath::Matrix& world)
	{
		RenderSubmission sub{};
		sub.mesh = mesh;
		sub.submesh = submesh;
		sub.mat = material;
		sub.world = world;
		m_submissions.push_back(sub);
	}

	void Renderer::SubmitMeshNoFaceCulling(Mesh mesh, u32 submesh, MaterialHandle material, const DirectX::SimpleMath::Matrix& world)
	{
		RenderSubmission sub{};
		sub.mesh = mesh;
		sub.submesh = submesh;
		sub.mat = material;
		sub.world = world;
		m_noCullSubmissions.push_back(sub);
	}

	void Renderer::SubmitAnimatedMesh(Mesh mesh, u32 submesh, MaterialHandle material, const DirectX::SimpleMath::Matrix& world)
	{
		RenderSubmission sub{};
		sub.mesh = mesh;
		sub.submesh = submesh;
		sub.mat = material;
		sub.world = world;
		m_animatedDraws.push_back(sub);
	}

	void Renderer::Update(f32 dt)
	{
		m_boneJourno->UpdateJoints();


		// Update per frame data
		{
			m_pfData.viewMatrix = m_viewMat;
			m_pfData.projMatrix = m_projMat;
			m_pfData.projMatrix.Invert(m_pfData.invProjMatrix);
			m_pfData.time += dt;

			// Get camera position
			DirectX::XMVECTOR tmp;
			auto invVm = DirectX::XMMatrixInverse(&tmp, m_viewMat);
			auto pos = invVm.r[3];
			DirectX::XMFLOAT3 posFloat3;
			DirectX::XMStoreFloat3(&posFloat3, pos);
			m_pfData.camPos = { posFloat3.x, posFloat3.y, posFloat3.z, 0.0f };

			m_pfDataTable->RequestUpdate(m_pfHandle, &m_pfData, sizeof(m_pfData));
			m_pfDataTable->SendCopyRequests(*m_perFrameUploadCtx);

			// Get offset after update
			m_currPfDescriptor = m_pfDataTable->GetLocalOffset(m_pfHandle);
			m_globalEffectData.perFrameTableOffset = &m_currPfDescriptor;

		}
	}

	void Renderer::Render(f32)
	{
		ZoneNamedN(RenderScope, "Render", true);

		// Resolve any per frame copies from CPU
		{
			ZoneNamedN(FrameCopyResolve, "Frame Copies", true);
			m_perFrameUploadCtx->SubmitCopies();
		}

		
		m_rg = std::move(std::make_unique<RenderGraph>(m_rd, m_rgResMan.get(), m_bin.get()));
		auto& rg = *m_rg;

		// Forward pass to HDR
		{
			struct JointData
			{
				DirectX::XMFLOAT4X4 joints[130];
			};

			struct PerDrawData
			{
				DirectX::XMMATRIX world;
				u32 globalSubmeshID{ UINT_MAX };
				u32 globalMaterialID{ UINT_MAX };
				u32 jointsDescriptor{ UINT_MAX };
			};

			/*
				@todo:
					Still need some way to pre-allocate per draw data prior to render pass.
					Perhaps go through the submissions and collect data --> Upload to GPU (maybe instance it as well?)
					and during forward pass we simply read from it 
			*/
			auto drawSubmissions = [this](RenderDevice* rd, CommandList cmdl, const std::vector<RenderSubmission> submissions, bool animated = false)
			{
				for (const auto& sub : submissions)
				{
					auto perDrawHandle = m_dynConstants->Allocate((u32)std::ceilf(sizeof(PerDrawData) / (float)256));
					PerDrawData perDrawData{};
					perDrawData.world = sub.world;
					perDrawData.globalSubmeshID = m_globalMeshTable->GetSubmeshMD_GPU(sub.mesh, sub.submesh);
					perDrawData.globalMaterialID = m_globalMaterialTable->GetMaterialIndex(sub.mat);

					if (animated)
					{
						// Resolve joints
						JointData jointsData{};
						auto jointsHandle = m_dynConstantsAnimated->Allocate((u32)std::ceilf(sizeof(JointData) / (float)256));
						for (size_t i = 0; i < m_boneJourno->m_vsJoints.size(); ++i)
							jointsData.joints[i] = m_boneJourno->m_vsJoints[i];
						std::memcpy(jointsHandle.memory, &jointsData, sizeof(jointsData));
						perDrawData.jointsDescriptor = jointsHandle.globalDescriptor;
					}

					std::memcpy(perDrawHandle.memory, &perDrawData, sizeof(perDrawData));

					auto args = ShaderArgs()
						.AppendConstant(m_globalEffectData.globalDataDescriptor)
						.AppendConstant(m_currPfDescriptor)
						.AppendConstant(perDrawHandle.globalDescriptor);

					rd->Cmd_UpdateShaderArgs(cmdl, QueueType::Graphics, args);

					auto sm = m_globalMeshTable->GetSubmeshMD_CPU(sub.mesh, sub.submesh);
					rd->Cmd_DrawIndexed(cmdl, sm.indexCount, 1, sm.indexStart, 0, 0);
				}
			};

			struct PassData {};
			rg.AddPass<PassData>("Forward Pass",
				[&](PassData&, RenderGraph::PassBuilder& builder)
				{
					builder.DeclareTexture(RG_RESOURCE(MainDepth), RGTextureDesc::DepthWrite2D(DepthFormat::D32, m_clientWidth, m_clientHeight));
					builder.DeclareTexture(RG_RESOURCE(LitHDR), RGTextureDesc::RenderTarget2D(DXGI_FORMAT_R16G16B16A16_FLOAT, m_clientWidth, m_clientHeight)
						.AddFlag(D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS));

					builder.WriteRenderTarget(RG_RESOURCE(LitHDR), RenderPassAccessType::ClearPreserve,
						TextureViewDesc(ViewType::RenderTarget, TextureViewDimension::Texture2D, DXGI_FORMAT_R16G16B16A16_FLOAT));
					builder.WriteDepthStencil(RG_RESOURCE(MainDepth), RenderPassAccessType::ClearDiscard,
						TextureViewDesc(ViewType::DepthStencil, TextureViewDimension::Texture2D, DXGI_FORMAT_D32_FLOAT));
				},
				[&](const PassData&, RenderDevice* rd, CommandList cmdl, RenderGraph::PassResources&)
				{
					rd->Cmd_SetViewports(cmdl, m_globalEffectData.defRenderVPs);
					rd->Cmd_SetScissorRects(cmdl, m_globalEffectData.defRenderScissors);
						
					rd->Cmd_SetIndexBuffer(cmdl, m_globalEffectData.meshTable->GetIndexBuffer());

					rd->Cmd_SetPipeline(cmdl, m_meshPipe);
					drawSubmissions(rd, cmdl, m_submissions);
					drawSubmissions(rd, cmdl, m_animatedDraws, true);

					rd->Cmd_SetPipeline(cmdl, m_meshPipeNoCull);
					drawSubmissions(rd, cmdl, m_noCullSubmissions);
				});
		}

		// Test compute on Lit HDR
		// Uncomment to enable the test compute effect!
		//m_testComputeEffect->Add(rg);

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

		// Final ImGUI pass
		m_imGUIEffect->Add(rg);

		{
			ZoneNamedN(RGBuildScope, "RG Building", true);
			rg.Build();
		}

		{
			ZoneNamedN(RGExecuteScope, "RG Execution", true);
			rg.Execute();
		}

	}

	void Renderer::OnResize(u32 clientWidth, u32 clientHeight)
	{
		m_clientWidth = clientWidth;
		m_clientHeight = clientHeight;

		m_sc->OnResize(clientWidth, clientHeight);
	}

	void DOG::gfx::Renderer::SetFullscreenState(WindowMode windowMode, DXGI_MODE_DESC mode)
	{
		bool succeeded;
		if (windowMode == WindowMode::Windowed)
		{
			succeeded = m_sc->SetFullscreenState(false, mode);
			if (!succeeded)
			{
				std::cout << "SetFullscreenState Windowed failed" << std::endl;
			}
		}
		else if (windowMode == WindowMode::FullScreen)
		{
			succeeded = m_sc->SetFullscreenState(true, mode);
			if (!succeeded)
			{
				std::cout << "SetFullscreenState Fullscreen failed" << std::endl;
			}
		}
	}

	WindowMode DOG::gfx::Renderer::GetFullscreenState() const
	{
		return m_sc->GetFullscreenState() ? WindowMode::FullScreen : WindowMode::Windowed;
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
		m_noCullSubmissions.clear();
		m_animatedDraws.clear();

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
