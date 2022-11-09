#pragma once
#include "../../ECS/EntityManager.h"			// Grab world state

namespace DOG::gfx
{
	class Renderer;

	/*
		Bridge between the Renderer and the outside architecture
	*/
	class FrontRenderer
	{
	public:
		FrontRenderer(Renderer* renderer);
		~FrontRenderer();

		// Mark when to start capturing GUI (ImGUI)
		void BeginFrameUICapture();

		// Gathers and updates any necessary world state for rendering this frame
		// Should be called prior to BeginGPUFrame()
		void Update(f32);

		// Set GPU frame span: EndFrame finalizes GUI as well
		void BeginGPUFrame();		// Waits for any previous GPU frames in flight!
		void EndGPUFrame();

		// Draw!
		void Render(f32);
		
		// ECS specifics
		void PerformDeferredDeletion();

		void ToggleShadowMapping(bool turnOn);

	private:


		void UpdateLights();
		void GatherDrawCalls();
		void SetRenderCamera();
		void GatherShadowCasters();
		void CullShadowDraws();

	private:
		Renderer* m_renderer{ nullptr };

		DirectX::SimpleMath::Matrix m_viewMat;
		
		u32 m_shadowMapCapacity{ 2 };
		std::vector<std::pair<entity, u32>> m_activeSpotlightShadowCasters;		// { entity, shadowID } 

		struct ShadowSubmission
		{
			Mesh mesh;
			u32 submesh{ 0 };
			TransformComponent tc;
			bool singleSided{ true };

			bool animated{ false };
			u32 jointOffset{ 0 };
		};

		std::vector<ShadowSubmission> m_singleSidedShadowed;
		std::vector<ShadowSubmission> m_doubleSidedShadowed;

	};


}