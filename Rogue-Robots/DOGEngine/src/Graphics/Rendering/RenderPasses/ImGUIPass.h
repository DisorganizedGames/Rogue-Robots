#pragma once
#include "GlobalPassData.h"

namespace DOG::gfx
{
	class RenderGraph;
	class RGBlackboard;

	class ImGUIBackend;

	class ImGUIPass
	{
	public:
		ImGUIPass(GlobalPassData& globalPassData, RGBlackboard& blackboard, ImGUIBackend* backend);
		
		void AddPass(RenderGraph& rg);

	private:
		GlobalPassData& m_globalPassData;
		RGBlackboard& m_blackboard;

		ImGUIBackend* m_imgui{ nullptr };
	};
}