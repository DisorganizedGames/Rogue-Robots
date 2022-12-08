#pragma once
#include "../../Core/Types/GraphicsTypes.h"
#include "../RHI/RenderResourceHandles.h"
#include "../RHI/Types/HardwareTypes.h"
#include "../../Core/AnimationManager.h"
#include "../../Core/CoreUtils.h"
#include "UI.h"
#include "GPUTable.h"

#include "RenderEffects/RenderEffect.h"
#include "RenderEffects/EffectData/GlobalEffectData.h"

#include "VFX/ParticleBackend.h"
#include "PostProcess.h"


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

	class ParticleBackend;
	struct ParticleEmitter;

	class Renderer
	{
	public:
		struct ShadowCaster
		{
			DirectX::SimpleMath::Matrix viewMat, projMat;
		};

		struct ActiveSpotlight
		{
			std::optional<ShadowCaster> shadow;

			DirectX::SimpleMath::Vector4 position;
			DirectX::SimpleMath::Vector3 color;
			float cutoffAngle{ 0.f };
			DirectX::SimpleMath::Vector3 direction;
			float strength{ 0.f };

			bool isPlayerLight{ false };
		};


	private:
		static constexpr u8 S_NUM_BACKBUFFERS = 2;
		static constexpr u8 S_MAX_FIF = 2;

		static_assert(S_MAX_FIF <= S_NUM_BACKBUFFERS);
	public:
		Renderer(HWND hwnd, u32 clientWidth, u32 clientHeight, bool debug, GraphicsSettings& settings);
		~Renderer();

		UploadContext* GetMeshUploadContext() const { return m_meshUploadCtx.get(); }
		MeshTable* GetMeshTable() const { return m_globalMeshTable.get(); }
		MaterialTable* GetMaterialTable() const { return m_globalMaterialTable.get(); }
		LightTable* GetLightTable() const { return m_globalLightTable.get(); }
		GraphicsBuilder* GetBuilder() const { return m_builder.get(); }
		Monitor GetMonitor() const;
		DXGI_MODE_DESC GetMatchingDisplayMode(std::optional<DXGI_MODE_DESC> mode = std::nullopt) const;
		void VerifyAndSanitizeGraphicsSettings(GraphicsSettings& settings, u32 clientWidth, u32 clientHeight) const;

		// Must be called at the start of any frame to pick up CPU side ImGUI code
		void BeginGUI();

		void SetMainRenderCamera(const DirectX::XMMATRIX& view, DirectX::XMMATRIX* proj = nullptr);

		// Weapon is special case..
		void SubmitMesh(Mesh mesh, u32 submesh, MaterialHandle material, const DirectX::SimpleMath::Matrix& world, bool isWeapon = false);
		void SubmitMeshNoFaceCulling(Mesh mesh, u32 submesh, MaterialHandle material, const DirectX::SimpleMath::Matrix& world);
		void SubmitMeshWireframe(Mesh mesh, u32 submesh, MaterialHandle material, const DirectX::SimpleMath::Matrix& world);
		void SubmitMeshWireframeNoFaceCulling(Mesh mesh, u32 submesh, MaterialHandle material, const DirectX::SimpleMath::Matrix& world);
		void SubmitOutlinedMesh(Mesh mesh, u32 submesh, const DirectX::SimpleMath::Vector3& color, const DirectX::SimpleMath::Matrix& world, bool animated, u32 jointOffset);

		void SubmitAnimatedMesh(Mesh mesh, u32 submesh, MaterialHandle material, const DirectX::SimpleMath::Matrix& world, u32 num);

		void SubmitSingleSidedShadowMesh(u32 shadowID, Mesh mesh, u32 submesh, const DirectX::SimpleMath::Matrix& world, bool animated = false, u32 jointOffset = 0);
		void SubmitDoubleSidedShadowMesh(u32 shadowID, Mesh mesh, u32 submesh, const DirectX::SimpleMath::Matrix& world, bool animated = false, u32 jointOffset = 0);

		void SubmitEmitters(const std::vector<ParticleEmitter>& emitters);

		// Optionally returns shadow ID if light casts shadows
		std::optional<u32> RegisterSpotlight(const ActiveSpotlight& data);

		// Internal state updates
		void Update(f32 dt);

		// GPU submission work
		void Render(f32 dt);


		void OnResize(u32 clientWidth, u32 clientHeight);
		void SetGraphicsSettings(GraphicsSettings requestedSettings);
		GraphicsSettings GetGraphicsSettings();
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

			DirectX::SimpleMath::Vector3 color;		// for outline

			bool animated{ false };
			// bitflags for target passes? (i.e multipass)
			u32 jointOffset{ 0 };
			bool isWeapon{ false };
		};

		void WaitForPrevFrame();

	private:
		std::function<LRESULT(HWND, UINT, WPARAM, LPARAM)> m_wmCallback;
		std::unique_ptr<RenderBackend> m_backend;
		std::unique_ptr<ImGUIBackend> m_imgui;
		
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


		std::vector<RenderSubmission> m_submissions;				// temporary
		std::vector<RenderSubmission> m_noCullSubmissions;			// temporary
		std::vector<RenderSubmission> m_animatedDraws;				// temp
		std::vector<RenderSubmission> m_wireframeDraws;				// temp
		std::vector<RenderSubmission> m_noCullWireframeDraws;		// temp
		std::vector<RenderSubmission> m_weaponSubmission;			// submission for weapons only

		u32 m_nextSingleSidedShadowBucket{ 0 };
		u32 m_nextDoubleSidedShadowBucket{ 0 };
		std::vector<std::vector<RenderSubmission>> m_singleSidedShadowDraws;
		std::vector<std::vector<RenderSubmission>> m_doubleSidedShadowDraws;

		std::vector<RenderSubmission> m_outlineDraws;


		DirectX::XMMATRIX m_viewMat, m_projMat;

		

		std::unique_ptr<UploadContext> m_uploadCtx;
		std::unique_ptr<UploadContext> m_perFrameUploadCtx;
		std::unique_ptr<UploadContext> m_texUploadCtx;
		std::unique_ptr<UploadContext> m_meshUploadCtx;

		// Ring-buffered dynamic constant allocator (allocate, use, and forget)
		std::unique_ptr<GPUDynamicConstants> m_dynConstants;
		std::unique_ptr<GPUDynamicConstants> m_dynConstantsTemp;
		std::unique_ptr<GPUDynamicConstants> m_dynConstantsAnimated;		// temp storage for per draw joints
		std::unique_ptr<GPUDynamicConstants> m_dynConstantsAnimatedShadows;		// temp storage for per draw joints for SHADOWS only

		
	
		// ================= External interfaces
		std::unique_ptr<GraphicsBuilder> m_builder;


		// ================= RENDERING RESOURCES

		Pipeline m_pipe, m_meshPipe, m_meshPipeNoCull, m_shadowPipe, m_shadowPipeNoCull;	// nocull are for the modular blocks (hack due to asset in shady state with negative scaling)
		Pipeline m_meshPipeWireframe, m_meshPipeWireframeNoCull;
		Pipeline m_ssaoPipe;
		Pipeline m_zPrePassPipe, m_zPrePassPipeNoCull, m_zPrePassPipeWirefram, m_zPrePassPipeWireframNoCull;
		Pipeline m_weaponMeshPipe;
		Pipeline m_outlineMeshPipe;
		Pipeline m_outlineBlitPipe;

		Pipeline m_boxBlurPipe;
		

		Texture m_ssaoNoise;
		Buffer m_ssaoSamples;


		// Reusing a single command list for now
		CommandList m_cmdl;

		std::unique_ptr<RGResourceManager> m_rgResMan;
		std::unique_ptr<RGBlackboard> m_rgBlackboard;
		std::unique_ptr<RenderGraph> m_rg;

		std::optional<SyncReceipt> m_frameCopyReceipt;




		//TMP
		std::unique_ptr<AnimationManager> m_jointMan;





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
			DirectX::SimpleMath::Matrix invViewMatrix;
			DirectX::SimpleMath::Matrix projMatrix;
			DirectX::SimpleMath::Matrix invProjMatrix;
			DirectX::SimpleMath::Vector4 camPos;
			f32 time{ 0.f };

			LightOffsets pointLightOffsets;
			LightOffsets spotLightOffsets;
			LightOffsets areaLightOffsets;

			f32 deltaTime{ 0.f };
			f32 farClip; // We have reverse z so farClip will hold the smaller value
			f32 nearClip;
			f32 pad[1];

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

		std::unique_ptr<ParticleBackend> m_particleBackend;

		// Passes
		std::unique_ptr<RenderEffect> m_imGUIEffect;
		std::unique_ptr<RenderEffect> m_testComputeEffect;
		std::unique_ptr<RenderEffect> m_bloomEffect;
		std::unique_ptr<RenderEffect> m_tiledLightCuller;
		std::unique_ptr<RenderEffect> m_tiledLightCullerVisualization;

		// Game related post process passes
		std::unique_ptr<RenderEffect> m_damageDiskEffect;
		std::unique_ptr<RenderEffect> m_heartbeatEffect;
		std::unique_ptr<RenderEffect> m_bloodViewEffect;
		std::unique_ptr<RenderEffect> m_laserBeamEffect;



		u32 m_shadowMapCapacity{ 1 };

		
		u32 m_currFrameIdx{ 0 };
		std::vector<std::optional<SyncReceipt>> m_frameSyncs;

		std::vector<ActiveSpotlight> m_activeSpotlights;
		struct ActiveShadowCaster
		{
			u32 singleSidedBucket{ UINT_MAX };
			u32 doubleSidedBucket{ UINT_MAX };
		};
		std::vector<ActiveShadowCaster> m_activeShadowCasters;

		GraphicsSettings m_graphicsSettings;

	};
}