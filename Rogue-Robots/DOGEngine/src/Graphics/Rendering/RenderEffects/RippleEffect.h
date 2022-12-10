#pragma once
#include "RenderEffect.h"

#include "../../RHI/RenderResourceHandles.h"

namespace DOG::gfx
{
	class RenderGraph;
	class RGBlackboard;
	class RenderDevice;
	class ShaderCompilerDXC;

	class RippleEffect : public RenderEffect
	{
	public:
		RippleEffect(GlobalEffectData& globalEffectData, u32 width, u32 height);

		void Add(RenderGraph& rg);

	private:
		Pipeline m_computePipe;
		u32 m_renderWidth;
		u32 m_renderHeight;
		f32 m_time = 0;
	};
}