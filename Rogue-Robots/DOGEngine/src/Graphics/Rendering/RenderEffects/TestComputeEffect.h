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
		TestComputeEffect(GlobalEffectData& globalEffectData, RGBlackboard& blackboard, RenderDevice* rd, ShaderCompilerDXC* sclr);

		void Add(RenderGraph& rg);

	private:
		RenderDevice* m_rd{ nullptr };
		ShaderCompilerDXC* m_sclr{ nullptr };

		Pipeline m_computePipe;
	};


}