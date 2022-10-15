#pragma once
#include "EffectData/GlobalEffectData.h"
#include "../../../Core/CoreUtils.h"

namespace DOG::gfx
{
	class RenderGraph;
	class RGBlackboard;

	/*
		A Render Effect represents a collection of one or more passes.
		Inherit from this to make your new effect! 
 
		Note to devs:
			If more functions are required, i.e OnResize or similar, please inform me (Nad).
			Architecture is being explored as it's being used, so all cases are not known.

	*/
	class RenderEffect
	{
	public:
		virtual ~RenderEffect() {}

		virtual void Add(RenderGraph& rg) = 0;
		virtual void SetGraphicsSettings(const GraphicsSettings& settings) {};

	protected:
		RenderEffect(GlobalEffectData& globalEffectData) :
			m_globalEffectData(globalEffectData) {}

		GlobalEffectData& m_globalEffectData;
	};
}