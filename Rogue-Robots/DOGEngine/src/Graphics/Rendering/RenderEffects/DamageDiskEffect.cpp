#include "DamageDiskEffect.h"
#include "RECommonIncludes.h"

#include "../../RHI/RenderDevice.h"
#include "../../RHI/ShaderCompilerDXC.h"
#include "../../RHI/PipelineBuilder.h"

#include "../PostProcess.h"

namespace DOG::gfx
{
	DamageDiskEffect::DamageDiskEffect(GlobalEffectData& globalEffectData) :
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
		auto ddPS = sclr->CompileFromFile("DamageDiskPS.hlsl", ShaderType::Pixel);
		m_pipe = rd->CreateGraphicsPipeline(GraphicsPipelineBuilder()
			.SetShader(fullscreenTriVS.get())
			.SetShader(ddPS.get())
			.SetBlend(BlendBuilder().AppendRTBlend(bd).SetAlphaToCoverageEnabled(true))
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

				//auto& ddData = PostProcess::Get().GetDamageDiskData();

				//float xDirf = ddData.dir2D.x;
				//float yDirf = ddData.dir2D.y;
				//float intensityf = ddData.currIntensity;
				//float visf = ddData.visibility;



				//// [-1, 1] --> [0, 2]
				//xDirf = xDirf * 0.5f;
				//xDirf += 0.5f;
				//yDirf = yDirf * 0.5f;
				//yDirf += 0.5f;


				//UINT xDir = *(UINT*)&xDirf;
				//UINT yDir = *(UINT*)&yDirf;
				//UINT intensity = *(UINT*)&intensityf;
				//UINT vis = *(UINT*)&visf;
				//auto args = ShaderArgs()
				//	.AppendConstant(xDir)
				//	.AppendConstant(yDir)
				//	.AppendConstant(intensity)
				//	.AppendConstant(vis);

				////std::cout << *(float*)&vis << "\n";


				//rd->Cmd_UpdateShaderArgs(cmdl, QueueType::Graphics, args);

				//rd->Cmd_SetPipeline(cmdl, m_pipe);
				//rd->Cmd_Draw(cmdl, 3, 1, 0, 0);


				auto& ddDatas = PostProcess::Get().GetDamageDisks();

				for (const auto& ddData : ddDatas)
				{
					if (ddData.visibility < 0.01f)
						continue;

					float xDirf = ddData.dir2D.x;
					float yDirf = ddData.dir2D.y;
					float intensityf = ddData.currIntensity;
					float visf = ddData.visibility;


					// [-1, 1] --> [0, 2]
					xDirf = xDirf * 0.5f;
					xDirf += 0.5f;
					yDirf = yDirf * 0.5f;
					yDirf += 0.5f;


					UINT xDir = *(UINT*)&xDirf;
					UINT yDir = *(UINT*)&yDirf;
					UINT intensity = *(UINT*)&intensityf;
					UINT vis = *(UINT*)&visf;
					auto args = ShaderArgs()
						.AppendConstant(xDir)
						.AppendConstant(yDir)
						.AppendConstant(intensity)
						.AppendConstant(vis);

					//std::cout << *(float*)&vis << "\n";


					rd->Cmd_UpdateShaderArgs(cmdl, QueueType::Graphics, args);

					rd->Cmd_SetPipeline(cmdl, m_pipe);
					rd->Cmd_Draw(cmdl, 3, 1, 0, 0);
				}
			});
	}
}
