#include "Renderer.h"

#include "../RHI/DX12/RenderBackend_DX12.h"
#include "../RHI/DX12/ImGUIBackend_DX12.h"
#include "../RHI/DX12/RenderDevice_DX12.h"
#include "../RHI/DX12/d2dBackend_DX12.h"
#include "../RHI/ShaderCompilerDXC.h"
#include "../RHI/PipelineBuilder.h"

#include "GPUGarbageBin.h"
#include "UploadContext.h"
#include "GPUTable.h"
#include "GPUDynamicConstants.h"
#include "MaterialTable.h"
#include "MeshTable.h"
#include "TextureManager.h"

#include "../../Core/AssimpImporter.h"
#include "../../Core/TextureFileImporter.h"


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
		m_ui = std::make_unique<UI>(m_rd, m_sc, S_NUM_BACKBUFFERS, hwnd);
		
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

		// Test mesh manager		



		// Load ONE model
		{
			// Load mesh
			auto res = AssimpImporter("Assets/Sponza_gltf/glTF/Sponza.gltf").GetResult();
			MeshTable::MeshSpecification loadSpec{};
			loadSpec.indices = res->mesh.indices;
			for (const auto& attr : res->mesh.vertexData)
				loadSpec.vertexDataPerAttribute[attr.first] = res->mesh.vertexData[attr.first];
			loadSpec.submeshData = res->submeshes;
			m_sponza = m_globalMeshTable->LoadMesh(loadSpec, *m_uploadCtx);

			m_uploadCtx->SubmitCopies();


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
				if (!m_texMan->Exists(path))
				{
					auto importedTex = TextureFileImporter(path, genMips).GetResult();
					if (importedTex)
						tex = loadTexture(*m_texMan, *m_texUploadCtx, path, *importedTex, srgb);
				}
				else
					tex = m_texMan->GetTexture(path);

				if (tex)
				{
					return m_rd->CreateView(*tex, TextureViewDesc(
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

				m_mats.push_back(m_globalMaterialTable->LoadMaterial(matSpec, *m_uploadCtx));
			}


			m_uploadCtx->SubmitCopies();

			m_texUploadCtx->SubmitCopies();
		}

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

	void Renderer::SubmitMesh(Mesh, u32, MaterialHandle)
	{
	}

	void Renderer::Update(f32)
	{
	}

	void Renderer::Render(f32)
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
		for (u32 i = 0; i < m_sponza.numSubmeshes; ++i)
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
				.AppendConstant(m_globalMeshTable->GetSubmeshMD_GPU(m_sponza.mesh, i))
				.AppendConstant(m_globalMeshTable->GetSubmeshDescriptor())
				.AppendConstant(m_globalMeshTable->GetAttributeDescriptor(VertexAttribute::Position))
				.AppendConstant(m_globalMeshTable->GetAttributeDescriptor(VertexAttribute::UV))
				.AppendConstant(m_globalMeshTable->GetAttributeDescriptor(VertexAttribute::Normal))
				.AppendConstant(m_globalMeshTable->GetAttributeDescriptor(VertexAttribute::Tangent))
				.AppendConstant(m_globalMaterialTable->GetDescriptor())
				.AppendConstant(m_globalMaterialTable->GetMaterialIndex(m_mats[i])
				);

			m_rd->Cmd_UpdateShaderArgs(m_cmdl, args);

			auto sm = m_globalMeshTable->GetSubmeshMD_CPU(m_sponza.mesh, i);
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

		//m_d2d->Render(m_rd, m_sc, m_cmdl);
		m_ui->m_d2d.BeginFrame(m_rd, m_sc);
		m_ui->drawUI();
		m_ui->m_d2d.EndFrame(m_rd, m_sc);

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

		m_bin->BeginFrame();
		m_rd->RecycleCommandList(m_cmdl);
		m_cmdl = m_rd->AllocateCommandList();
	}

	void Renderer::EndFrame_GPU(bool vsync)
	{
		EndGUI();
		m_bin->EndFrame();

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

}
