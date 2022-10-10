#pragma once
#include "RenderEffect.h"

#include "../../RHI/RenderResourceHandles.h"

namespace DOG::gfx
{
	class RenderGraph;
	class RGBlackboard;
	class RenderDevice;
	class ShaderCompilerDXC;

	class TestComputeEffect : public RenderEffect
	{
	public:
		TestComputeEffect(GlobalEffectData& globalEffectData);

		void Add(RenderGraph& rg);

	private:
		Pipeline m_computePipe;
	};


}