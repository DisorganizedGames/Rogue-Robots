#pragma once
#include "EffectData/GlobalEffectData.h"

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

	protected:
		RenderEffect(GlobalEffectData& globalEffectData, RGBlackboard& blackboard) :
			m_globalEffectData(globalEffectData),
			m_blackboard(blackboard) {}

		GlobalEffectData& m_globalEffectData;
		RGBlackboard& m_blackboard;
	};
}