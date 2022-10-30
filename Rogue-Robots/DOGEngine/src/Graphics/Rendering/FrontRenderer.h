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
		void SetRenderCamera();
		void GatherShadowCasters();

	private:
		Renderer* m_renderer{ nullptr };
		
		u32 m_shadowMapCapacity{ 2 };
		std::set<entity> m_activeSpotlightShadowCasters;

	};


}