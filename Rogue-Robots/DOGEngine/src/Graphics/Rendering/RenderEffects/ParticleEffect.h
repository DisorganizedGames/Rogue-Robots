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
		Pipeline m_aliveMarkerPipeline;
		Pipeline m_updatePipeline;
		Pipeline m_sortPipeline;
		Pipeline m_drawPipeline;

		static constexpr u32 S_MAX_PARTICLES = 32*1024;
		static constexpr u32 S_COUNTERS = 2;
		static constexpr u32 S_SORT_COMPUTE_GROUP = 256;
	
	private:
		struct Particle
		{
			u32 emitterHandle = 0;
			f32 pos[3] = { 0, 0, 0 };
			f32 age = 0;
			f32 vel[3] = { 0, 0, 0 };
			f32 size = 0;
			u32 alive = 0;
			f32 pad[2] = { 0 };
		};

	private:
		void AddSortPasses(RenderGraph& renderGraph);
		void AddLocal(RenderGraph& renderGraph, u32 groupSize);
		void AddGlobal(RenderGraph& renderGraph, u32 groupSize);
	};

		
	
}