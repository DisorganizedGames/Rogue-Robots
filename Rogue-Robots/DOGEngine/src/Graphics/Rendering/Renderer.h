#pragma once
#include "../../Core/Types/GraphicsTypes.h"
#include "../RHI/RenderResourceHandles.h"
#include "../../Core/AnimationManager.h"
namespace DOG::gfx
{
	class RenderBackend;
	class ImGUIBackend;
	class RenderDevice;
	class Swapchain;
	class ShaderCompilerDXC;

	class GPUGarbageBin;
	class UploadContext;
	class GPUDynamicConstants;
	class MaterialTable;
	class MeshTable;
	class TextureManager;

	class GraphicsBuilder;

	class RenderGraph;
	class RGResourceManager;

	class Renderer
	{
	private:
		static constexpr u8 S_NUM_BACKBUFFERS = 2;
		static constexpr u8 S_MAX_FIF = 1;

		static_assert(S_MAX_FIF <= S_NUM_BACKBUFFERS);
	public:
		Renderer(HWND hwnd, u32 clientWidth, u32 clientHeight, bool debug);
		~Renderer();

		GraphicsBuilder* GetBuilder() const { return m_builder.get(); }


		// Must be called at the start of any frame to pick up CPU side ImGUI code
		void BeginGUI();

		void SetMainRenderCamera(const DirectX::XMMATRIX& view, DirectX::XMMATRIX* proj = nullptr);

		void SubmitMesh(Mesh mesh, u32 submesh, MaterialHandle material, const DirectX::SimpleMath::Matrix& world);
		void SubmitMeshNoFaceCulling(Mesh mesh, u32 submesh, MaterialHandle material, const DirectX::SimpleMath::Matrix& world);



		// Internal state updates
		void Update(f32 dt);

		// GPU submission work
		void Render(f32 dt);


		void OnResize(u32 clientWidth, u32 clientHeight);

		void BeginFrame_GPU();
		void EndFrame_GPU(bool vsync);

		void Flush();

		const std::function<LRESULT(HWND, UINT, WPARAM, LPARAM)>& GetWMCallback() { return m_wmCallback; }

	private:
		void EndGUI();	// Called at EndFrame_GPU



		LRESULT WinProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	private:
		struct RenderSubmission
		{
			Mesh mesh;
			u32 submesh;
			MaterialHandle mat;
			DirectX::SimpleMath::Matrix world;
		};

	private:
		std::unique_ptr<RenderBackend> m_backend;
		std::unique_ptr<ImGUIBackend> m_imgui;
		RenderDevice* m_rd{ nullptr };
		Swapchain* m_sc{ nullptr };
		
		std::unique_ptr<GraphicsBuilder> m_builder;
		std::vector<RenderSubmission> m_submissions;		// temporary
		std::vector<RenderSubmission> m_noCullSubmissions;	// temporary


		DirectX::XMMATRIX m_viewMat, m_projMat;

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

		std::function<LRESULT(HWND, UINT, WPARAM, LPARAM)> m_wmCallback;

		// ================= RENDERING RESOURCES

		Pipeline m_pipe, m_meshPipe, m_meshPipeNoCull;
		Pipeline m_testCompPipe;

		// Reusing a single command list for now
		CommandList m_cmdl;

		std::unique_ptr<RenderGraph> m_rg;
		std::unique_ptr<RGResourceManager> m_rgResMan;

		//TMP
		std::unique_ptr<AnimationManager> m_boneJourno;
	};
}