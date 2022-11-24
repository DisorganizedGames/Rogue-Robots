#pragma once
#include "RenderEffect.h"

namespace DOG::gfx
{
	class RGResourceManager;
	class UploadContext;

	class ParticleEffect final : public RenderEffect
	{
	public:
		ParticleEffect(GlobalEffectData& globalEffectData, RGResourceManager* resourceManager, UploadContext* upCtx, u32 maxEmitters);
		~ParticleEffect();
		void Add(RenderGraph& renderGraph) override;

		void SetEmitters(u32 globalDescriptor, u32 localOffset)
		{
			m_emitterGlobalDescriptor = globalDescriptor;
			m_emitterLocalOffset = localOffset;
		};

	private:
		Buffer m_particleBuffer;
		Buffer m_particlesAlive;

		Buffer m_emitterToSpawn;

		Texture m_noiseTexture;

		u32 m_emitterGlobalDescriptor = 0;
		u32 m_emitterLocalOffset = 0;

		u32 m_maxEmitters = 0;

		RGResourceManager* m_resourceManager;

		Pipeline m_emitPipeline;
		Pipeline m_postEmitPipeline;
		Pipeline m_compactPipeline;
		Pipeline m_updatePipeline;
		Pipeline m_drawPipeline;

		static constexpr u32 S_MAX_PARTICLES = 16 * 1024;
		static constexpr u32 S_COUNTERS = 2;
	
	private:
		struct Particle
		{
			u32 emitterHandle = 0;
			f32 pos[3] = { 0, 0, 0 };
			f32 age = 0;
			f32 vel[3] = { 0, 0, 0 };
			f32 size = 0;
			f32 pad[3] = { 0, 0, 0 };
		};

	};
	
}