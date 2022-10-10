#pragma once
#include "RenderEffect.h"

namespace DOG::gfx
{
	class ImGUIBackend;

	class ImGUIEffect : public RenderEffect
	{
	public:
		ImGUIEffect(GlobalEffectData& globalEffectData, ImGUIBackend* backend);
		
		void Add(RenderGraph& rg);

	private:
		ImGUIBackend* m_imgui{ nullptr };
	};
}