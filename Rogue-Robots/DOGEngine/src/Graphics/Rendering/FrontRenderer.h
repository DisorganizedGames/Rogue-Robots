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

		// Set GPU frame span: EndFrame finalizes GUI as well
		void BeginGPUFrame();
		void EndGPUFrame();

		// Draw!
		void Render(f32);
		
		// ECS specifics
		void PerformDeferredDeletion();

	private:
		// Gathers and updates any neccessary world state rendering this frame
		void Update(f32);

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
		};

		std::vector<ShadowSubmission> m_singleSidedShadowed;
		std::vector<ShadowSubmission> m_doubleSidedShadowed;

	};


}