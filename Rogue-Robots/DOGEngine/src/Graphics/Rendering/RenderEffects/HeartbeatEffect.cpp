#include "HeartbeatEffect.h"
#include "RECommonIncludes.h"

#include "../../RHI/RenderDevice.h"
#include "../../RHI/ShaderCompilerDXC.h"
#include "../../RHI/PipelineBuilder.h"

#include "../PostProcess.h"

namespace DOG::gfx
{
	HeartbeatEffect::HeartbeatEffect(GlobalEffectData& globalEffectData) :
		RenderEffect(globalEffectData)
	{
		auto rd = globalEffectData.rd;
		auto sclr = globalEffectData.sclr;

		D3D12_RENDER_TARGET_BLEND_DESC bd{};
		bd.BlendEnable = true;
		bd.LogicOpEnable = false;
		//bd.SrcBlend = D3D12_BLEND_SRC_COLOR;
		//bd.DestBlend = D3D12_BLEND_DEST_COLOR;
		//bd.BlendOp = D3D12_BLEND_OP_ADD;
		//bd.SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
		//bd.DestBlendAlpha = D3D12_BLEND_DEST_ALPHA;
		//bd.BlendOpAlpha = D3D12_BLEND_OP_MAX;
		bd.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

		bd.SrcBlend = D3D12_BLEND_SRC_ALPHA;
		bd.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
		bd.BlendOp = D3D12_BLEND_OP_ADD;
		bd.SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
		bd.DestBlendAlpha = D3D12_BLEND_DEST_ALPHA;
		bd.BlendOpAlpha = D3D12_BLEND_OP_MAX;

		auto fullscreenTriVS = sclr->CompileFromFile("FullscreenTriVS.hlsl", ShaderType::Vertex);
		auto ddPS = sclr->CompileFromFile("HeartbeatPS.hlsl", ShaderType::Pixel);
		m_pipe = rd->CreateGraphicsPipeline(GraphicsPipelineBuilder()
			.SetShader(fullscreenTriVS.get())
			.SetShader(ddPS.get())
			.SetBlend(BlendBuilder().AppendRTBlend(bd).SetAlphaToCoverageEnabled(true))
			.AppendRTFormat(DXGI_FORMAT_R16G16B16A16_FLOAT)
			.Build());
	}

	HeartbeatEffect::~HeartbeatEffect()
	{
		m_globalEffectData.rd->FreePipeline(m_pipe);
	}

	void HeartbeatEffect::Add(RenderGraph& rg)
	{
		struct PassData {};
		rg.AddPass<PassData>("Heartbeat Effect Pass",
			[&](PassData&, RenderGraph::PassBuilder& builder)
			{
				builder.WriteRenderTarget(RG_RESOURCE(LitHDR), RenderPassAccessType::PreservePreserve,
					TextureViewDesc(ViewType::RenderTarget, TextureViewDimension::Texture2D, DXGI_FORMAT_R16G16B16A16_FLOAT));
			},
			[&](const PassData&, RenderDevice* rd, CommandList cmdl, RenderGraph::PassResources&)
			{
				rd->Cmd_SetViewports(cmdl, m_globalEffectData.defRenderVPs);
				rd->Cmd_SetScissorRects(cmdl, m_globalEffectData.defRenderScissors);

				f32 effect = PostProcess::Get().GetHeartbeatIntensity();
				f32 factor = PostProcess::Get().GetHeartbeatTransitionFactor();
				rd->Cmd_SetPipeline(cmdl, m_pipe);
				UINT effect32Packed = *(UINT*)&effect;
				UINT factor32Packed = *(UINT*)&factor;
				rd->Cmd_UpdateShaderArgs(cmdl, QueueType::Graphics, ShaderArgs()
					.AppendConstant(m_globalEffectData.defRenderVPs.vps[0].Width)
					.AppendConstant(m_globalEffectData.defRenderVPs.vps[0].Height)
					.AppendConstant(effect32Packed)
					.AppendConstant(factor32Packed));
				rd->Cmd_Draw(cmdl, 3, 1, 0, 0);
			});
	}
}
