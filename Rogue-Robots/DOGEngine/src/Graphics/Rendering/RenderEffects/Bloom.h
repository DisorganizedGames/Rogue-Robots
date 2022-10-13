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
		Bloom(GlobalEffectData& globalEffectData, GPUDynamicConstants* dynConsts);

		void Add(RenderGraph& rg) override;

	private:
		static constexpr u32 computeGroupSize = 32;
		struct BloomConstantBuffer
		{
			f32 threshold;
		};

		// Untill the rendergraph can handle subresources we will use fake mip levels by using more textures
		std::vector<std::pair<Texture, TextureDesc>> m_bloomTexture;


		u32 m_width;
		u32 m_height;

		GPUDynamicConstants* m_dynamicConstants{ nullptr };
		Pipeline m_compPipeBloomSelect;
		Pipeline m_compPipeDownSample;
		Pipeline m_compPipeUpSample;
		Pipeline m_compPipDebug;
	};
}