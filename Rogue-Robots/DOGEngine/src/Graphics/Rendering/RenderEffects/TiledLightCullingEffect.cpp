#include "TiledLightCullingEffect.h"

#include "RECommonIncludes.h"

#include "../../RHI/RenderDevice.h"
#include "../../RHI/ShaderCompilerDXC.h"
#include "../GPUDynamicConstants.h"

namespace DOG::gfx
{
	TiledLightCullingEffect::TiledLightCullingEffect(RGResourceManager* resMan, GlobalEffectData& globalEffectData, GPUDynamicConstants* dynConsts, u32 renderResX, u32 renderResY)
		: RenderEffect(globalEffectData), m_resMan(resMan), m_dynamicConstants(dynConsts), m_hdrRenderTargerResX(renderResX), m_hdrRenderTargerResY(renderResY)
	{
		m_width = m_hdrRenderTargerResX;
		m_height = m_hdrRenderTargerResY;

		auto& device = globalEffectData.rd;

		auto cullShader = globalEffectData.sclr->CompileFromFile("TiledLightCullingCS.hlsl", ShaderType::Compute);
		m_compPipeline = device->CreateComputePipeline(ComputePipelineDesc(cullShader.get()));

	}
	TiledLightCullingEffect::~TiledLightCullingEffect()
	{
		m_globalEffectData.rd->FreePipeline(m_compPipeline);
	}
	void TiledLightCullingEffect::Add(RenderGraph& rg)
	{
		struct PassData
		{
			GPUDynamicConstant constantBufferHandle;
			RGResourceView litHDRView;
		};

		// Copy the colors that exceeds the threshold from our hdr render targer to our bloomTexture. This should also scale to a lower resolution, but for now the bloomTexture has a hard coded size. 

		rg.AddPass<PassData>("Tiled light culling",
			[&](PassData& passData, RenderGraph::PassBuilder& builder)		// Build
			{
				passData.litHDRView = builder.ReadWriteTarget(RG_RESOURCE(LitHDR),
					TextureViewDesc(ViewType::UnorderedAccess, TextureViewDimension::Texture2D, DXGI_FORMAT_R16G16B16A16_FLOAT));
			},
			[&](const PassData& passData, RenderDevice* rd, CommandList cmdl, RenderGraph::PassResources& resources)		// Execute
			{

				rd->Cmd_SetPipeline(cmdl, m_compPipeline);
				auto args = ShaderArgs()
					.AppendConstant(m_globalEffectData.globalDataDescriptor)
					.AppendConstant(*m_globalEffectData.perFrameTableOffset)
					.AppendConstant(resources.GetView(passData.litHDRView))
					.AppendConstant(passData.constantBufferHandle.globalDescriptor)
					.AppendConstant(m_width)
					.AppendConstant(m_height);
				rd->Cmd_UpdateShaderArgs(cmdl, QueueType::Compute, args);

				u32 tgx = m_width / computeGroupSize + 1 * static_cast<bool>(m_width % computeGroupSize);
				u32 tgy = m_height / computeGroupSize + 1 * static_cast<bool>(m_height % computeGroupSize);

				rd->Cmd_Dispatch(cmdl, tgx, tgy, 1);
			},
				[&](PassData& passData)		// Pre-graph work
			{
				auto cb = m_dynamicConstants->Allocate(1);

				passData.constantBufferHandle = cb;

				TiledLightCullingEffectBuffer perDrawData{};
				perDrawData.threshold = 1;

				*reinterpret_cast<TiledLightCullingEffectBuffer*>(passData.constantBufferHandle.memory) = perDrawData;
			});
	}
	void TiledLightCullingEffect::SetGraphicsSettings(const GraphicsSettings& settings)
	{

	}
}