#pragma once
#include "RenderEffect.h"

namespace DOG::gfx
{
	class RGResourceManager;
	class UploadContext;
	struct Buffer;

	class ParticleEffect final : public RenderEffect
	{	
	public:
		ParticleEffect(GlobalEffectData& globalEffectData, RGResourceManager* resourceManager, UploadContext* uploadCtx);
		~ParticleEffect();
		void Add(RenderGraph& renderGraph) override;

	private:
		Buffer m_emitterBuffer;
		Buffer m_particleBuffer;

		RGResourceManager* m_resourceManager;

		Pipeline m_emitPipeline;
		Pipeline m_updatePipeline;

		static constexpr u32 S_MAX_EMITTERS = 512;
		static constexpr u32 S_MAX_PARTICLES = 100'000;
	
	private:
		struct Particle
		{
			u32 emitterHandle = 0; // A particle is alive if its emitter handle is non-zero
			f32 pos[3] = {0, 0, 0};
			f32 vel[3] = {0, 0, 0};
			f32 size = 0;
			f32 color[3] = {0, 0, 0};
			f32 age = 0;
		};

		struct Emitter
		{
			f32 pos[3];
			f32 lifetime;
			u32 particlesAlive;
			u32 padding[3];
		};
	};
	
}