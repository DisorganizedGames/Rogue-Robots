#include "RippleEffect.h"

#include "RECommonIncludes.h"
#include "../../../Core/Time.h"
#include "../../RHI/RenderDevice.h"
#include "../../RHI/ShaderCompilerDXC.h"
#include "ImGUI/imgui.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;
using namespace DOG;
using namespace DOG::gfx;

RippleEffect::RippleEffect(GlobalEffectData& globalEffectData, u32 width, u32 height)
	: RenderEffect(globalEffectData), m_renderWidth(width), m_renderHeight(height)
{
	auto& sclr = m_globalEffectData.sclr;
	auto& rd = m_globalEffectData.rd;

	auto shader = sclr->CompileFromFile("RippleEffectCS.hlsl", ShaderType::Compute);
	m_computePipe = rd->CreateComputePipeline(ComputePipelineDesc(shader.get()));
}

void RippleEffect::Add(RenderGraph& rg)
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
			m_time += static_cast<f32>(Time::DeltaTime());
			if (m_time > 1)
				m_time = 0;


			static float waveParams[3] = {10, 0.8f, 0.03f};

			ImGui::DragFloat3("waveParams", waveParams, 0.05f);

			rd->Cmd_SetPipeline(cmdl, m_computePipe);
			ShaderArgs args = ShaderArgs()
				.AppendConstant(resources.GetView(passData.litHDRView))
				.AppendConstant(m_renderWidth)
				.AppendConstant(m_renderHeight)
				.AppendConstant(std::bit_cast<u32>(waveParams[0]))
				.AppendConstant(std::bit_cast<u32>(waveParams[1]))
				.AppendConstant(std::bit_cast<u32>(waveParams[2]))
				.AppendConstant(std::bit_cast<u32>(m_time))
				.AppendConstant(std::bit_cast<u32>(0.5f))
				.AppendConstant(std::bit_cast<u32>(0.5f));
			rd->Cmd_UpdateShaderArgs(cmdl, QueueType::Compute, args);
			u32 gCountX = (m_renderWidth + 15) / 16;
			u32 gCountY = (m_renderHeight + 15) / 16;
			rd->Cmd_Dispatch(cmdl, gCountX, gCountY, 1);
		});
}
