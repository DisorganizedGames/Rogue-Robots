#pragma once
#include "../../Core/Types/MeshTypes.h"
#include "../../Core/Types/MaterialTypes.h"
#include "../RHI/RenderResourceHandles.h"

namespace DOG::gfx
{
	class RenderDevice;
	class RenderBackend;
	class Swapchain;
	class ShaderCompilerDXC;

	class GPUGarbageBin;
	class UploadContext;
	class GPUDynamicConstants;
	class MaterialTable;
	class MeshTable;
	class TextureManager;

	class Renderer
	{
	private:
		static constexpr u32 S_NUM_BACKBUFFERS = 2;
		static constexpr u32 S_MAX_FIF = S_NUM_BACKBUFFERS;

	public:
		Renderer(HWND hwnd, u32 clientWidth, u32 clientHeight, bool debug);
		~Renderer();

		// GraphicsBuilders* GetBuilders();

		// Must be called at the start of any frame to pick up CPU side ImGUI code
		void BeginGUI();

		void SetMainRenderCamera(const DirectX::XMMATRIX& view, const DirectX::XMMATRIX proj);

		void SubmitMesh(Mesh mesh, u32 submesh, MaterialHandle material);

		// Internal state updates
		void Update(f32 dt);

		// GPU submission work
		void Render(f32 dt);


		void OnResize(u32 clientWidth, u32 clientHeight);

		void BeginFrame_GPU();
		void EndFrame_GPU(bool vsync);

		void Flush();

	private:
		void EndGUI();	// Called at EndFrame_GPU

	private:
		std::unique_ptr<RenderBackend> m_backend;
		RenderDevice* m_rd{ nullptr };
		Swapchain* m_sc{ nullptr };

		u32 m_clientWidth{ 0 };
		u32 m_clientHeight{ 0 };
		
		std::unique_ptr<ShaderCompilerDXC> m_sclr;

		std::unique_ptr<GPUGarbageBin> m_bin;
		std::unique_ptr<UploadContext> m_uploadCtx;
		std::unique_ptr<UploadContext> m_texUploadCtx;

		// Ring-buffered dynamic constant allocator (allocate, use, and forget)
		std::unique_ptr<GPUDynamicConstants> m_dynConstants;

		// Big buffers store meshes and materials
		std::unique_ptr<MaterialTable> m_globalMaterialTable;
		std::unique_ptr<MeshTable> m_globalMeshTable;

		// Caches textures (for now, temp?)
		std::unique_ptr<TextureManager> m_texMan;		

	

		// ================= RENDERING RESOURCES
		Texture m_depthTex;
		TextureView m_depthTarget;

		std::array<Texture, S_NUM_BACKBUFFERS> m_scTextures;
		std::array<TextureView, S_NUM_BACKBUFFERS> m_scViews;
		std::array<RenderPass, S_NUM_BACKBUFFERS> m_scPasses;

		Pipeline m_pipe, m_meshPipe;

		// Reusing a single command list for now
		CommandList m_cmdl;



		// TEMP
		MeshContainer m_sponza;
		std::vector<MaterialHandle> m_mats;




	};
}