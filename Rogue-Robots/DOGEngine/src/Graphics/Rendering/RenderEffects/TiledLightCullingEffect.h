#pragma once

#include "RenderEffect.h"

#include "../../RHI/RenderResourceHandles.h"

namespace DOG::gfx
{
	class RenderGraph;
	class RGResourceManager;
	class GPUDynamicConstants;
	class TiledLightCullingEffect : public RenderEffect
	{
	public:
		TiledLightCullingEffect(RGResourceManager* resMan, GlobalEffectData& globalEffectData, GPUDynamicConstants* dynConsts, u32 renderResX, u32 renderResY);
		~TiledLightCullingEffect();
		void Add(RenderGraph& rg) override;
		void SetGraphicsSettings(const GraphicsSettings& settings) override;
	private:
		static constexpr u32 computeGroupSize = 16;
		struct TiledLightCullingEffectBuffer
		{
			f32 threshold;
		};

		u32 m_hdrRenderTargerResX;
		u32 m_hdrRenderTargerResY;

		u32 m_width;
		u32 m_height;

		GPUDynamicConstants* m_dynamicConstants{ nullptr };
		Pipeline m_compPipeline;

		RGResourceManager* m_resMan{ nullptr };
	};
}