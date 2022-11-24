#pragma once

#include "RenderEffect.h"

#include "../../RHI/RenderResourceHandles.h"

namespace DOG::gfx
{
	class RenderGraph;
	class GPUDynamicConstants;
	class LaserEffect : public RenderEffect
	{
	public:
		LaserEffect(GlobalEffectData& globalEffectData, GPUDynamicConstants* dynConsts);
		~LaserEffect();
		void Add(RenderGraph& rg) override;

	private:
		Pipeline m_laserEffectPipeline;
		GPUDynamicConstants* m_dynamicConstants{ nullptr };
	};
}