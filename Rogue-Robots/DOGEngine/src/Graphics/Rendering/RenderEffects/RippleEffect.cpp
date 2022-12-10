#include "RippleEffect.h"

#include "RECommonIncludes.h"
#include "../../../Core/Time.h"
#include "../../RHI/RenderDevice.h"
#include "../../RHI/ShaderCompilerDXC.h"
#include "../PostProcess.h"
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
			auto& shockWaves = PostProcess::Get().GetShockWaves();
			if (shockWaves.empty())
				return;
			rd->Cmd_SetPipeline(cmdl, m_computePipe);
			int deadCount = 0;
			for (auto& sw : shockWaves)
			{
				if (sw.currentTime >= sw.lifeTime)
				{
					deadCount++;
				}
				sw.currentTime += static_cast<f32>(Time::DeltaTime());

				Vector4 pos;
				pos.x = sw.position.x;
				pos.y = sw.position.y;
				pos.z = sw.position.z;
				pos.w = 1;

				Matrix vp = PostProcess::Get().GetViewMatrix() * PostProcess::Get().GetProjMatrix();

				pos = DirectX::XMVector4Transform(pos, vp);

				pos.x /= pos.w;
				pos.y /= pos.w;
				pos.z /= pos.w;

				pos.x += 0.5f;
				pos.y += 0.5f;

				ShaderArgs args = ShaderArgs()
					.AppendConstant(resources.GetView(passData.litHDRView))
					.AppendConstant(m_renderWidth)
					.AppendConstant(m_renderHeight)
					.AppendConstant(std::bit_cast<u32>(sw.args.x))
					.AppendConstant(std::bit_cast<u32>(sw.args.y))
					.AppendConstant(std::bit_cast<u32>(sw.args.z))
					.AppendConstant(std::bit_cast<u32>(sw.currentTime))
					.AppendConstant(std::bit_cast<u32>(pos.x))
					.AppendConstant(std::bit_cast<u32>(pos.y));
				rd->Cmd_UpdateShaderArgs(cmdl, QueueType::Compute, args);

				u32 gCountX = (m_renderWidth + 15) / 16;
				u32 gCountY = (m_renderHeight + 15) / 16;
				rd->Cmd_Dispatch(cmdl, gCountX, gCountY, 1);
			}

			std::sort(shockWaves.begin(), shockWaves.end(), [](auto& a, auto& b) { return a.currentTime < b.lifeTime; });
			for (int i = 0; i < deadCount; i++)
			{
				shockWaves.pop_back();
			}
				
		});
}

void DOG::gfx::RippleEffect::SetGraphicsSettings(const GraphicsSettings& settings)
{
	m_renderWidth = settings.renderResolution.x;
	m_renderHeight = settings.renderResolution.y;
}
