#include "DamageDiskEffect.h"
#include "RECommonIncludes.h"

#include "../../RHI/RenderDevice.h"
#include "../../RHI/ShaderCompilerDXC.h"
#include "../../RHI/PipelineBuilder.h"

namespace DOG::gfx
{
	DamageDiskEffect::DamageDiskEffect(GlobalEffectData& globalEffectData) :
		RenderEffect(globalEffectData)
	{
		auto rd = globalEffectData.rd;
		auto sclr = globalEffectData.sclr;		

		auto fullscreenTriVS = sclr->CompileFromFile("FullscreenTriVS.hlsl", ShaderType::Vertex);
		auto ddPS = sclr->CompileFromFile("DamageDiskPS.hlsl", ShaderType::Pixel);
		m_pipe = rd->CreateGraphicsPipeline(GraphicsPipelineBuilder()
			.SetShader(fullscreenTriVS.get())
			.SetShader(ddPS.get())
			.SetBlend(BlendBuilder().SetAlphaToCoverageEnabled(true))
			.AppendRTFormat(DXGI_FORMAT_R16G16B16A16_FLOAT)
			.Build());
	}

	DamageDiskEffect::~DamageDiskEffect()
	{
		m_globalEffectData.rd->FreePipeline(m_pipe);
	}


	void DamageDiskEffect::Add(RenderGraph& rg)
	{
		struct PassData {};
		rg.AddPass<PassData>("Damage Disk Pass",
			[&](PassData&, RenderGraph::PassBuilder& builder)
			{
				builder.WriteRenderTarget(RG_RESOURCE(LitHDR), RenderPassAccessType::PreservePreserve,
					TextureViewDesc(ViewType::RenderTarget, TextureViewDimension::Texture2D, DXGI_FORMAT_R16G16B16A16_FLOAT));
			},
			[&](const PassData&, RenderDevice* rd, CommandList cmdl, RenderGraph::PassResources&)
			{
				rd->Cmd_SetViewports(cmdl, m_globalEffectData.defRenderVPs);
				rd->Cmd_SetScissorRects(cmdl, m_globalEffectData.defRenderScissors);

				float xDirf = 0.f;
				float yDirf = 1.f;
				float visf = 1.f;

				// [-1, 1] --> [0, 2]
				xDirf = xDirf * 0.5f;
				xDirf += 0.5f;
				yDirf = yDirf * 0.5f;
				yDirf += 0.5f;


				UINT xDir = *(UINT*)&xDirf;
				UINT yDir = *(UINT*)&yDirf;
				UINT vis = *(UINT*)&visf;
				auto args = ShaderArgs()
					.AppendConstant(xDir)
					.AppendConstant(yDir)
					.AppendConstant(vis);
				rd->Cmd_UpdateShaderArgs(cmdl, QueueType::Graphics, args);

				rd->Cmd_SetPipeline(cmdl, m_pipe);
				rd->Cmd_Draw(cmdl, 3, 1, 0, 0);
			});
	}
}
