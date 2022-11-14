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
		TiledLightCullingEffect(RGResourceManager* resMan, GlobalEffectData& globalEffectData, u32 renderResX, u32 renderResY);
		~TiledLightCullingEffect();
		void Add(RenderGraph& rg) override;
		void SetGraphicsSettings(const GraphicsSettings& settings) override;

		static constexpr u32 maxLightsPerTile = 31; // 32 - 1 leave room for the count
		struct LocalLightBufferLayout
		{
			u32 count;
			u32 lightIndices[maxLightsPerTile];
		};

		Vector2u GetGroupCount() const;

	private:
		static constexpr u32 computeGroupSize = 16;

		u32 m_threadGroupCountX;
		u32 m_threadGroupCountY;

		u32 m_width;
		u32 m_height;

		Pipeline m_compPipeline;

		RGResourceManager* m_resMan{ nullptr };
	};



	class TiledLightCullingVisualizationEffect : public RenderEffect
	{
	public:
		TiledLightCullingVisualizationEffect(RGResourceManager* resMan, GlobalEffectData& globalEffectData, u32 renderResX, u32 renderResY);
		~TiledLightCullingVisualizationEffect();
		void Add(RenderGraph& rg) override;
		void SetGraphicsSettings(const GraphicsSettings& settings) override;

	private:
		static constexpr u32 computeGroupSize = 16;

		u32 m_threadGroupCountX;
		u32 m_threadGroupCountY;

		u32 m_width;
		u32 m_height;
		Pipeline m_compPipeline;

		RGResourceManager* m_resMan{ nullptr };
	};
}