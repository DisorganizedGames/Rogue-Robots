#pragma once
#include "RenderEffect.h"
#include "../../RHI/RenderResourceHandles.h"

namespace DOG::gfx
{
	class HeartbeatEffect : public RenderEffect
	{
	public:
		HeartbeatEffect(GlobalEffectData& globalEffectData);
		~HeartbeatEffect();

		void Add(RenderGraph& rg);

	private:
		Pipeline m_pipe;

	};
}