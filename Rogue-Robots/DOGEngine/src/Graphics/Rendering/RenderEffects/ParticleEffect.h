#pragma once
#include "RenderEffect.h"

namespace DOG::gfx
{
	class RGResourceManager;
	class UploadContext;
	struct Buffer;

	class ParticleEffect final : public RenderEffect
	{
		using Matrix = DirectX::SimpleMath::Matrix;

	public:
		ParticleEffect(GlobalEffectData& globalEffectData, RGResourceManager* resourceManager, UploadContext* uploadCtx);
		~ParticleEffect();
		void Add(RenderGraph& renderGraph) override;

	private:
		Buffer m_emitterBuffer;
		Buffer m_particleBuffer;
		Buffer m_particlesAlive;

		RGResourceManager* m_resourceManager;

		Pipeline m_emitPipeline;
		Pipeline m_updatePipeline;
		Pipeline m_drawPipeline;

		static constexpr u32 S_MAX_EMITTERS = 128;
		static constexpr u32 S_MAX_PARTICLES = 16*1024;
	
	private:
		struct Particle
		{
			u32 emitterHandle = 0;
			f32 pos[3] = {0, 0, 0};
			f32 age = 0;
			f32 vel[3] = {0, 0, 0};
			f32 size = 0;
			f32 pad[3] = {0, 0, 0};
		};

		struct Emitter
		{
			f32 pos[3] = {0, 0, 0};
			f32 rate = 0;
			f32 lifetime = 0;
			f32 pad[3] = { 0, 0, 0 };
		};

	};
	
}