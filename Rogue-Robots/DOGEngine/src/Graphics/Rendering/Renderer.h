#pragma once
#include "../../Core/Types/GraphicsTypes.h"
#include "../RHI/RenderResourceHandles.h"
#include "../RHI/Types/HardwareTypes.h"
#include "../../Core/AnimationManager.h"
#include "../../Core/CoreUtils.h"
#include "GPUTable.h"

#include "RenderEffects/RenderEffect.h"
#include "RenderEffects/EffectData/GlobalEffectData.h"

namespace DOG::gfx
{
	class RenderBackend;
	class ImGUIBackend;
	class D2DBackend;
	class RenderDevice;
	class Swapchain;
	class ShaderCompilerDXC;

	class GPUGarbageBin;
	class UploadContext;
	class GPUDynamicConstants;
	class MaterialTable;
	class MeshTable;
	class LightTable;
	class TextureManager;

	class GraphicsBuilder;

	class RenderGraph;
	class RGResourceManager;
	class RGBlackboard;

	class Renderer
	{
	private:
		static constexpr u8 S_NUM_BACKBUFFERS = 2;
		static constexpr u8 S_MAX_FIF = 1;

		static_assert(S_MAX_FIF <= S_NUM_BACKBUFFERS);
	public:
		Renderer(HWND hwnd, u32 clientWidth, u32 clientHeight, bool debug);
		~Renderer();

		LightTable* GetLightTable() const { return m_globalLightTable.get(); }
		GraphicsBuilder* GetBuilder() const { return m_builder.get(); }
		Monitor GetMonitor() const;
		DXGI_MODE_DESC GetMatchingDisplayMode(std::optional<DXGI_MODE_DESC> mode = std::nullopt) const;

		// Must be called at the start of any frame to pick up CPU side ImGUI code
		void BeginGUI();

		void SetMainRenderCamera(const DirectX::XMMATRIX& view, DirectX::XMMATRIX* proj = nullptr);

		void SubmitMesh(Mesh mesh, u32 submesh, MaterialHandle material, const DirectX::SimpleMath::Matrix& world);
		void SubmitMeshNoFaceCulling(Mesh mesh, u32 submesh, MaterialHandle material, const DirectX::SimpleMath::Matrix& world);
		void SubmitMeshWireframe(Mesh mesh, u32 submesh, MaterialHandle material, const DirectX::SimpleMath::Matrix& world);
		void SubmitMeshWireframeNoFaceCulling(Mesh mesh, u32 submesh, MaterialHandle material, const DirectX::SimpleMath::Matrix& world);

		void SubmitAnimatedMesh(Mesh mesh, u32 submesh, MaterialHandle material, const DirectX::SimpleMath::Matrix& world);




		// Internal state updates
		void Update(f32 dt);

		// GPU submission work
		void Render(f32 dt);


		void OnResize(u32 clientWidth, u32 clientHeight);
		void SetGraphicsSettings(GraphicsSettings requestedSettings);
		WindowMode GetFullscreenState() const;


		void BeginFrame_GPU();
		void EndFrame_GPU(bool vsync);

		void Flush();

		const std::function<LRESULT(HWND, UINT, WPARAM, LPARAM)>& GetWMCallback() { return m_wmCallback; }

	private:
		void EndGUI();	// Called at EndFrame_GPU

		void SpawnRenderDebugWindow(bool& open);




		LRESULT WinProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	private:
		struct RenderSubmission
		{
			Mesh mesh;
			u32 submesh;
			MaterialHandle mat;			// mat args 
			DirectX::SimpleMath::Matrix world;

			// bitflags for target passes? (i.e multipass)
		};

	private:
		std::function<LRESULT(HWND, UINT, WPARAM, LPARAM)> m_wmCallback;
		std::unique_ptr<RenderBackend> m_backend;
		std::unique_ptr<ImGUIBackend> m_imgui;
		std::unique_ptr<UI> m_ui; 
		std::unique_ptr<ShaderCompilerDXC> m_sclr;
		std::unique_ptr<GPUGarbageBin> m_bin;
		RenderDevice* m_rd{ nullptr };
		Swapchain* m_sc{ nullptr };

		u32 m_renderWidth{ 0 };
		u32 m_renderHeight{ 0 };

		// Big buffers store meshes and materials
		std::unique_ptr<MaterialTable> m_globalMaterialTable;
		std::unique_ptr<MeshTable> m_globalMeshTable;
		std::unique_ptr<LightTable> m_globalLightTable;


		std::vector<RenderSubmission> m_submissions;		// temporary
		std::vector<RenderSubmission> m_noCullSubmissions;	// temporary
		std::vector<RenderSubmission> m_animatedDraws;		// temp
		std::vector<RenderSubmission> m_wireframeDraws;		// temp
		std::vector<RenderSubmission> m_noCullWireframeDraws;		// temp


		DirectX::XMMATRIX m_viewMat, m_projMat;


		

		std::unique_ptr<UploadContext> m_uploadCtx;
		std::unique_ptr<UploadContext> m_perFrameUploadCtx;
		std::unique_ptr<UploadContext> m_texUploadCtx;

		// Ring-buffered dynamic constant allocator (allocate, use, and forget)
		std::unique_ptr<GPUDynamicConstants> m_dynConstants;
		std::unique_ptr<GPUDynamicConstants> m_dynConstantsAnimated;		// temp storage for per draw joints


	
		// ================= External interfaces
		std::unique_ptr<GraphicsBuilder> m_builder;


		// ================= RENDERING RESOURCES

		Pipeline m_pipe, m_meshPipe, m_meshPipeNoCull;
		Pipeline m_meshPipeWireframe, m_meshPipeWireframeNoCull;

		// Reusing a single command list for now
		CommandList m_cmdl;

		std::unique_ptr<RenderGraph> m_rg;
		std::unique_ptr<RGResourceManager> m_rgResMan;
		std::unique_ptr<RGBlackboard> m_rgBlackboard;





		//TMP
		std::unique_ptr<AnimationManager> m_boneJourno;





		GlobalEffectData m_globalEffectData{};

		struct LightOffsets
		{
			u32 staticOffset;
			u32 infreqOffset;
			u32 dynOffset;
		};

		// Per frame shader data
		struct PerFrameData
		{
			DirectX::SimpleMath::Matrix viewMatrix;
			DirectX::SimpleMath::Matrix projMatrix;
			DirectX::SimpleMath::Matrix invProjMatrix;
			DirectX::SimpleMath::Vector4 camPos;
			f32 time{ 0.f };

			LightOffsets pointLightOffsets;
			LightOffsets spotLightOffsets;
			LightOffsets areaLightOffsets;

		} m_pfData{};
		struct PfDataHandle { friend class TypedHandlePool; u64 handle{ 0 }; };
		std::unique_ptr<GPUTableDeviceLocal<PfDataHandle>> m_pfDataTable;
		PfDataHandle m_pfHandle;
		u32 m_currPfDescriptor{ 0 };

		// Per frame global data
		struct GlobalData
		{
			// Mesh
			u32 meshTableSubmeshMD{ 0 };
			u32 meshTablePos{ 0 };
			u32 meshTableUV{ 0 };
			u32 meshTableNor{ 0 };
			u32 meshTableTan{ 0 };
			u32 meshTableBlend{ 0 };

			u32 perFrameTable;
			u32 materialTable{ 0 };

			u32 lightTableMD{ 0 };
			u32 pointLightTable{ 0 };
			u32 spotLightTable{ 0 };
			u32 areaLightTable{ 0 };

		} m_globalData{};
		struct GlobalDataHandle{ friend class TypedHandlePool; u64 handle{ 0 }; };
		std::unique_ptr<GPUTableDeviceLocal<GlobalDataHandle>> m_globalDataTable;
		GlobalDataHandle m_gdHandle;

		// Passes
		std::unique_ptr<RenderEffect> m_imGUIEffect;
		std::unique_ptr<RenderEffect> m_testComputeEffect;

	};
}