#pragma once

#include "RenderEffect.h"

#include "../../RHI/RenderResourceHandles.h"

namespace DOG::gfx
{
	class RenderGraph;
	class GPUDynamicConstants;
	class Bloom : public RenderEffect
	{
	public:
		Bloom(GlobalEffectData& globalEffectData, GPUDynamicConstants* dynConsts, u32 renderResX, u32 renderResY);
		~Bloom();
		void Add(RenderGraph& rg) override;
		void SetGraphicsSettings(const GraphicsSettings& settings) override;
	private:
		static constexpr u32 computeGroupSize = 32;
		struct BloomConstantBuffer
		{
			f32 threshold;
		};
		f32 m_threshold = 0.5f;
		// Untill the rendergraph can handle subresources we will use fake mip levels by using more textures
		std::vector<std::pair<Texture, TextureDesc>> m_bloomTexture;

		u32 m_hdrRenderTargerResX;
		u32 m_hdrRenderTargerResY;

		u32 m_width;
		u32 m_height;

		GPUDynamicConstants* m_dynamicConstants{ nullptr };
		Pipeline m_compPipeBloomSelect;
		Pipeline m_compPipeDownSample;
		Pipeline m_compPipeUpSample;
		Pipeline m_compPipDebug;
	};
}