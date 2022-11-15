#include "TiledLightCullingEffect.h"

#include "RECommonIncludes.h"

#include "../../RHI/RenderDevice.h"
#include "../../RHI/ShaderCompilerDXC.h"
#include "../GPUDynamicConstants.h"

namespace DOG::gfx
{
	TiledLightCullingEffect::TiledLightCullingEffect(RGResourceManager* resMan, GlobalEffectData& globalEffectData, u32 renderResX, u32 renderResY, float pointLightCullFactor)
		: RenderEffect(globalEffectData), m_resMan(resMan), m_width(renderResX), m_height(renderResY), m_pointLightCullFactor(pointLightCullFactor)
	{
		m_threadGroupCountX = m_width / computeGroupSize + 1 * static_cast<bool>(m_width % computeGroupSize);
		m_threadGroupCountY = m_height / computeGroupSize + 1 * static_cast<bool>(m_height % computeGroupSize);

		auto& device = globalEffectData.rd;

		auto cullShader = globalEffectData.sclr->CompileFromFile("TiledLightCullingCS.hlsl", ShaderType::Compute);
		m_compPipeline = device->CreateComputePipeline(ComputePipelineDesc(cullShader.get()));

	}

	TiledLightCullingEffect::~TiledLightCullingEffect()
	{
		m_globalEffectData.rd->FreePipeline(m_compPipeline);
	}

	Vector2u TiledLightCullingEffect::GetGroupCount() const
	{
		return Vector2u(m_threadGroupCountX, m_threadGroupCountY);
	}

	void TiledLightCullingEffect::Add(RenderGraph& rg)
	{
		struct PassData
		{
			RGResourceView localLightBuffer;
			RGResourceView depthBuffer;
		};

		rg.AddPass<PassData>("Tiled light culling",
			[&](PassData& passData, RenderGraph::PassBuilder& builder)		// Build
			{
				builder.DeclareBuffer(RG_RESOURCE(LocalLightBuf), RGBufferDesc(sizeof(LocalLightBufferLayout) * m_threadGroupCountX * m_threadGroupCountY, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS));

				passData.localLightBuffer = builder.ReadWriteTarget(RG_RESOURCE(LocalLightBuf), BufferViewDesc(ViewType::UnorderedAccess, 0, sizeof(LocalLightBufferLayout), m_threadGroupCountX * m_threadGroupCountY));

				passData.depthBuffer = builder.ReadResource(RG_RESOURCE(ZPrePassDepth), D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE,
					TextureViewDesc(ViewType::ShaderResource, TextureViewDimension::Texture2D, DXGI_FORMAT_R32_FLOAT));
			},
			[&](const PassData& passData, RenderDevice* rd, CommandList cmdl, RenderGraph::PassResources& resources)		// Execute
			{
				u32 plCullFactor = *((u32*)&m_pointLightCullFactor);
				rd->Cmd_SetPipeline(cmdl, m_compPipeline);
				auto args = ShaderArgs()
					.AppendConstant(m_globalEffectData.globalDataDescriptor)
					.AppendConstant(*m_globalEffectData.perFrameTableOffset)
					.AppendConstant(resources.GetView(passData.localLightBuffer))
					.AppendConstant(m_width)
					.AppendConstant(m_height)
					.AppendConstant(plCullFactor);

				rd->Cmd_UpdateShaderArgs(cmdl, QueueType::Compute, args);


				rd->Cmd_Dispatch(cmdl, m_threadGroupCountX, m_threadGroupCountY, 1);
			});
	}

	void TiledLightCullingEffect::SetGraphicsSettings(const GraphicsSettings& settings)
	{
		m_width = settings.renderResolution.x;
		m_height = settings.renderResolution.y;
		m_threadGroupCountX = m_width / computeGroupSize + 1 * static_cast<bool>(m_width % computeGroupSize);
		m_threadGroupCountY = m_height / computeGroupSize + 1 * static_cast<bool>(m_height % computeGroupSize);
		m_pointLightCullFactor = settings.pointLightCullFactor;
	}






	//----------------------------------------------------


	TiledLightCullingVisualizationEffect::TiledLightCullingVisualizationEffect(RGResourceManager* resMan, GlobalEffectData& globalEffectData, u32 renderResX, u32 renderResY)
		: RenderEffect(globalEffectData), m_resMan(resMan), m_width(renderResX), m_height(renderResY)
	{
		m_threadGroupCountX = m_width / computeGroupSize + 1 * static_cast<bool>(m_width % computeGroupSize);
		m_threadGroupCountY = m_height / computeGroupSize + 1 * static_cast<bool>(m_height % computeGroupSize);

		auto& device = globalEffectData.rd;

		auto cullShader = globalEffectData.sclr->CompileFromFile("TiledLightCullingVisualizationCS.hlsl", ShaderType::Compute);
		m_compPipeline = device->CreateComputePipeline(ComputePipelineDesc(cullShader.get()));

	}

	TiledLightCullingVisualizationEffect::~TiledLightCullingVisualizationEffect()
	{
		m_globalEffectData.rd->FreePipeline(m_compPipeline);
	}

	void TiledLightCullingVisualizationEffect::Add(RenderGraph& rg)
	{
		struct PassData
		{
			RGResourceView litHDRView;
			RGResourceView localLightBuffer;
		};

		rg.AddPass<PassData>("Tiled light culling vis",
			[&](PassData& passData, RenderGraph::PassBuilder& builder)		// Build
			{
				passData.litHDRView = builder.ReadWriteTarget(RG_RESOURCE(LitHDR),
					TextureViewDesc(ViewType::UnorderedAccess, TextureViewDimension::Texture2D, DXGI_FORMAT_R16G16B16A16_FLOAT));

				passData.localLightBuffer = builder.ReadResource(RG_RESOURCE(LocalLightBuf), D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE, BufferViewDesc(ViewType::ShaderResource, 0, sizeof(TiledLightCullingEffect::LocalLightBufferLayout), m_threadGroupCountX * m_threadGroupCountY));

			},
			[&](const PassData& passData, RenderDevice* rd, CommandList cmdl, RenderGraph::PassResources& resources)		// Execute
			{

				rd->Cmd_SetPipeline(cmdl, m_compPipeline);
				auto args = ShaderArgs()
					.AppendConstant(resources.GetView(passData.litHDRView))
					.AppendConstant(resources.GetView(passData.localLightBuffer))
					.AppendConstant(m_width)
					.AppendConstant(m_height);
				rd->Cmd_UpdateShaderArgs(cmdl, QueueType::Compute, args);


				rd->Cmd_Dispatch(cmdl, m_threadGroupCountX, m_threadGroupCountY, 1);
			},
			[&](PassData&)		// Pre-graph work
			{

			});
	}

	void TiledLightCullingVisualizationEffect::SetGraphicsSettings(const GraphicsSettings&)
	{

	}
}