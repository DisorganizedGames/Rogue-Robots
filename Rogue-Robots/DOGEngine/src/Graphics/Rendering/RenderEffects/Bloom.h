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
		struct BloomConstantBuffer
		{
			DirectX::SimpleMath::Vector3 color;
			f32 threshold;
			Vector2u res;
		};

		TextureDesc m_bloomTexDesc;
		//Texture m_bloomTexture;

		// Untill the rendergraph can handle subresources we will use fake mip levels by using more textures
		std::vector<Texture> m_bloomTexture;


		u32 m_width;
		u32 m_height;

		GPUDynamicConstants* m_dynamicConstants{ nullptr };
		Pipeline m_computePipe;
		Pipeline m_compPipDownSample;
	};
}