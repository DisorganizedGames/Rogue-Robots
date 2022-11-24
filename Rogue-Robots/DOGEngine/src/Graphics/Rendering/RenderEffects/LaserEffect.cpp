#include "LaserEffect.h"

#include "RECommonIncludes.h"

#include "../../RHI/RenderDevice.h"
#include "../../RHI/ShaderCompilerDXC.h"
#include "../GPUDynamicConstants.h"
#include "../../RHI/PipelineBuilder.h"
#include "../PostProcess.h"

namespace DOG::gfx
{
	LaserEffect::LaserEffect(GlobalEffectData& globalEffectData, GPUDynamicConstants* dynConsts) : RenderEffect(globalEffectData), m_dynamicConstants(dynConsts)
	{
		auto& device = globalEffectData.rd;

		auto vs = globalEffectData.sclr->CompileFromFile("LaserVS.hlsl", ShaderType::Vertex);
		auto ps = globalEffectData.sclr->CompileFromFile("LaserPS.hlsl", ShaderType::Pixel);
		m_laserEffectPipeline = device->CreateGraphicsPipeline(GraphicsPipelineBuilder()
			.SetShader(vs.get())
			.SetShader(ps.get())
			.AppendRTFormat(DXGI_FORMAT_R16G16B16A16_FLOAT)
			.SetRasterizer(RasterizerBuilder().SetCullMode(D3D12_CULL_MODE_NONE))
			.SetDepthFormat(DepthFormat::D32)
			.SetDepthStencil(DepthStencilBuilder().SetDepthEnabled(true))
			.Build());
	}

	LaserEffect::~LaserEffect()
	{
		m_globalEffectData.rd->FreePipeline(m_laserEffectPipeline);
	}

	void LaserEffect::Add(RenderGraph& rg)
	{
		
		struct PassData
		{
		};

		rg.AddPass<PassData>("Laser Beam Effect",
			[&](PassData&, RenderGraph::PassBuilder& builder)		// Build
			{
				builder.WriteRenderTarget(RG_RESOURCE(LitHDR), RenderPassAccessType::PreservePreserve, 
					TextureViewDesc(ViewType::RenderTarget, TextureViewDimension::Texture2D, DXGI_FORMAT_R16G16B16A16_FLOAT));

				builder.WriteDepthStencil(RG_RESOURCE(MainDepth), RenderPassAccessType::PreservePreserve,
					TextureViewDesc(ViewType::DepthStencil, TextureViewDimension::Texture2D, DXGI_FORMAT_D32_FLOAT));
			},
			[&](const PassData&, RenderDevice* rd, CommandList cmdl, RenderGraph::PassResources&)		// Execute
			{
				auto& lasersBeams = PostProcess::Get().GetLaserBeams();
				if (lasersBeams.empty()) return;
				rd->Cmd_SetViewports(cmdl, m_globalEffectData.defRenderVPs);
				rd->Cmd_SetScissorRects(cmdl, m_globalEffectData.defRenderScissors);
				rd->Cmd_SetPipeline(cmdl, m_laserEffectPipeline);

				struct LaserBuffer
				{
					DirectX::SimpleMath::Matrix worldMatrix;
					DirectX::SimpleMath::Vector3 color;
					float length{1.0f};
				};
				for (auto& laser : lasersBeams)
				{
					auto cb = m_dynamicConstants->Allocate(1, false);
					LaserBuffer laserBuffer;
					laserBuffer.color = laser.color;
					laserBuffer.length = laser.length;
					laserBuffer.worldMatrix = laser.matrix;
					
					std::memcpy(cb.memory, &laserBuffer, sizeof(LaserBuffer));

					auto args = ShaderArgs()
						.AppendConstant(m_globalEffectData.globalDataDescriptor)
						.AppendConstant(*m_globalEffectData.perFrameTableOffset)
						.SetPrimaryCBV(cb.buffer, cb.bufferOffset);

					rd->Cmd_UpdateShaderArgs(cmdl, QueueType::Graphics, args);
					rd->Cmd_Draw(cmdl, 6, 1, 0, 0);
				}
				lasersBeams.clear();
			});
	}
}
