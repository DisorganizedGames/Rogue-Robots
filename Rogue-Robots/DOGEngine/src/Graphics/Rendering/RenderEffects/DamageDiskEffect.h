#pragma once
#include "RenderEffect.h"
#include "../../RHI/RenderResourceHandles.h"

namespace DOG::gfx
{
	class DamageDiskEffect : public RenderEffect
	{
	public:
		DamageDiskEffect(GlobalEffectData& globalEffectData);
		~DamageDiskEffect();

		void Add(RenderGraph& rg);

	private:
		Pipeline m_pipe;

	};
}