#include "TestComputeEffect.h"
#include "RECommonIncludes.h"

#include "../../RHI/RenderDevice.h"
#include "../../RHI/ShaderCompilerDXC.h"

namespace DOG::gfx
{
	TestComputeEffect::TestComputeEffect(GlobalEffectData& globalEffectData) :
		RenderEffect(globalEffectData)
	{
		auto& sclr = m_globalEffectData.sclr;
		auto& rd = m_globalEffectData.rd;

		auto testCS = sclr->CompileFromFile("TestComputeCS.hlsl", ShaderType::Compute);
		m_computePipe = rd->CreateComputePipeline(ComputePipelineDesc(testCS.get()));
	}

	void TestComputeEffect::Add(RenderGraph& rg)
	{
		struct PassData 
		{
			RGResourceView litHDRView;
		};
		rg.AddPass<PassData>("Compute Pass",
			[&](PassData& passData, RenderGraph::PassBuilder& builder)		// Build
			{
				passData.litHDRView = builder.ReadWriteTarget(RG_RESOURCE(LitHDR),
					TextureViewDesc(ViewType::UnorderedAccess, TextureViewDimension::Texture2D, DXGI_FORMAT_R16G16B16A16_FLOAT));
			},
			[&](const PassData& passData, RenderDevice* rd, CommandList cmdl, RenderGraph::PassResources& resources)		// Execute
			{
				u32 view = resources.GetView(passData.litHDRView);

				rd->Cmd_SetPipeline(cmdl, m_computePipe);
				auto args = ShaderArgs()
					.AppendConstant(view)
					.AppendConstant(1)
					.AppendConstant(0)
					.AppendConstant(0)
					.AppendConstant(14);
				rd->Cmd_UpdateShaderArgs(cmdl, QueueType::Compute, args);
				rd->Cmd_Dispatch(cmdl, 25, 14, 1);

			});
	}
}