#include "Bloom.h"

#include "RECommonIncludes.h"

#include "../../RHI/RenderDevice.h"
#include "../../RHI/ShaderCompilerDXC.h"
#include "../GPUDynamicConstants.h"

namespace DOG::gfx
{
	Bloom::Bloom(RGResourceManager* resMan, GlobalEffectData& globalEffectData, GPUDynamicConstants* dynConsts, u32 renderResX, u32 renderResY)
		: RenderEffect(globalEffectData), m_resMan(resMan), m_dynamicConstants(dynConsts), m_hdrRenderTargerResX(renderResX), m_hdrRenderTargerResY(renderResY)
	{
		m_width = m_hdrRenderTargerResX / 2;
		m_height = m_hdrRenderTargerResY / 2;

		auto& device = globalEffectData.rd;

		auto bloomShader = globalEffectData.sclr->CompileFromFile("BloomSelect.hlsl", ShaderType::Compute);
		m_compPipeBloomSelect = device->CreateComputePipeline(ComputePipelineDesc(bloomShader.get()));

		auto downSampleShader = globalEffectData.sclr->CompileFromFile("BloomDownSample.hlsl", ShaderType::Compute);
		m_compPipeDownSample = device->CreateComputePipeline(ComputePipelineDesc(downSampleShader.get()));

		auto upSampleShader = globalEffectData.sclr->CompileFromFile("BloomUpSample.hlsl", ShaderType::Compute);
		m_compPipeUpSample = device->CreateComputePipeline(ComputePipelineDesc(upSampleShader.get()));

		/*auto debugShader = globalEffectData.sclr->CompileFromFile("BloomDebug.hlsl", ShaderType::Compute);
		m_compPipDebug = device->CreateComputePipeline(ComputePipelineDesc(debugShader.get()));*/

		TextureDesc topLevelBloomDesc;
		topLevelBloomDesc.width = m_width;
		topLevelBloomDesc.height = m_height;
		topLevelBloomDesc.mipLevels = 1;
		topLevelBloomDesc.flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		topLevelBloomDesc.format = DXGI_FORMAT_R16G16B16A16_FLOAT;

		m_bloomTexture.emplace_back(device->CreateTexture(topLevelBloomDesc), topLevelBloomDesc);

		TextureDesc desc = topLevelBloomDesc;
		desc.width = topLevelBloomDesc.width / 2;
		desc.height = topLevelBloomDesc.height / 2;
		constexpr u32 minMipSize = 8;
		constexpr u32 maxMips = 6;
		int n = 0;
		while (desc.width > minMipSize && desc.height > minMipSize && n < maxMips)
		{
			m_bloomTexture.emplace_back(device->CreateTexture(desc), desc);
			desc.width /= 2;
			desc.height /= 2;
			n++;
		}

		resMan->ImportTexture(RG_RESOURCE(BloomTexture0), m_bloomTexture[0].first, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COMMON);

		for (int i = 1; i < m_bloomTexture.size(); i++)
			resMan->ImportTexture(RGResourceID("BloomTexture" + std::to_string(i)), m_bloomTexture[i].first, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COMMON);

	}

	Bloom::~Bloom()
	{
		auto& device = m_globalEffectData.rd;
		for (auto& [texture, desc] : m_bloomTexture)
		{
			device->FreeTexture(texture);
		}

		//device->FreePipeline(m_compPipDebug);
		device->FreePipeline(m_compPipeBloomSelect);
		device->FreePipeline(m_compPipeDownSample);
		device->FreePipeline(m_compPipeUpSample);

		m_resMan->FreeImported(RG_RESOURCE(BloomTexture0));
		for (int i = 1; i < m_bloomTexture.size(); i++)
			m_resMan->FreeImported(RGResourceID("BloomTexture" + std::to_string(i)));
	}

	void Bloom::Add(RenderGraph& rg)
	{
		struct PassData
		{
			GPUDynamicConstant constantBufferHandle;
			RGResourceView srcTextureHandle;
			RGResourceView dstTextureHandle;
			u32 level;
			u32 width;
			u32 height;
		};

		// Copy the colors that exceeds the threshold from our hdr render targer to our bloomTexture. This should also scale to a lower resolution, but for now the bloomTexture has a hard coded size. 

		rg.AddPass<PassData>("Bloom Pass threshold check",
			[&](PassData& passData, RenderGraph::PassBuilder& builder)		// Build
			{
				passData.srcTextureHandle = builder.ReadWriteTarget(RG_RESOURCE(LitHDR),
					TextureViewDesc(ViewType::UnorderedAccess, TextureViewDimension::Texture2D, DXGI_FORMAT_R16G16B16A16_FLOAT));

				//builder.ImportTexture(RG_RESOURCE(BloomTexture0), m_bloomTexture[0].first, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COMMON);
				passData.dstTextureHandle = builder.ReadWriteTarget(RG_RESOURCE(BloomTexture0), TextureViewDesc(ViewType::UnorderedAccess, TextureViewDimension::Texture2D, DXGI_FORMAT_R16G16B16A16_FLOAT));
			},
			[&](const PassData& passData, RenderDevice* rd, CommandList cmdl, RenderGraph::PassResources& resources)		// Execute
			{
				u32 view = resources.GetView(passData.srcTextureHandle);
				u32 bloomTextureHandle = resources.GetView(passData.dstTextureHandle);

				rd->Cmd_SetPipeline(cmdl, m_compPipeBloomSelect);
				auto args = ShaderArgs()
					.AppendConstant(view)
					.AppendConstant(bloomTextureHandle)
					.AppendConstant(m_width)
					.AppendConstant(m_height)
					.AppendConstant(passData.constantBufferHandle.globalDescriptor);
				rd->Cmd_UpdateShaderArgs(cmdl, QueueType::Compute, args);

				u32 tgx = m_width / computeGroupSize + 1 * static_cast<bool>(m_width % computeGroupSize);
				u32 tgy = m_height / computeGroupSize + 1 * static_cast<bool>(m_height % computeGroupSize);

				rd->Cmd_ClearUnorderedAccessFLOAT(cmdl,
					resources.GetTextureView(passData.dstTextureHandle), { 0.f, 0.f, 0.f, 0.f }, ScissorRects().Append(0, 0, m_width, m_height));

				rd->Cmd_Dispatch(cmdl, tgx, tgy, 1);
			},
			[&](PassData& passData)		// Pre-graph work
			{
				auto cb = m_dynamicConstants->Allocate(1);

				passData.constantBufferHandle = cb;

				BloomConstantBuffer perDrawData{};
				perDrawData.threshold = m_threshold;

				*reinterpret_cast<BloomConstantBuffer*>(passData.constantBufferHandle.memory) = perDrawData;
			});


		// Down scale to half resolution and filter

		for (int i = 1; i < m_bloomTexture.size(); i++)
		{
			rg.AddPass<PassData>("Bloom Pass" + std::to_string(i),
				[&](PassData& passData, RenderGraph::PassBuilder& builder)		// Build
				{
					passData.level = i;
					passData.width = m_bloomTexture[passData.level].second.width;
					passData.height = m_bloomTexture[passData.level].second.height;
					//builder.ImportTexture(RGResourceID("BloomTexture" + std::to_string(i)), m_bloomTexture[i].first, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COMMON);

					// Nad: builder.DeclareTexture("BloomTexture" + std::to_string(i - 1), bloomDescs[i]);

					passData.srcTextureHandle = builder.ReadWriteTarget(RGResourceID("BloomTexture" + std::to_string(i-1)), TextureViewDesc(ViewType::UnorderedAccess, TextureViewDimension::Texture2D, DXGI_FORMAT_R16G16B16A16_FLOAT));

					passData.dstTextureHandle = builder.ReadWriteTarget(RGResourceID("BloomTexture" + std::to_string(i)), TextureViewDesc(ViewType::UnorderedAccess, TextureViewDimension::Texture2D, DXGI_FORMAT_R16G16B16A16_FLOAT));
				},
				[&](const PassData& passData, RenderDevice* rd, CommandList cmdl, RenderGraph::PassResources& resources)		// Execute
				{
					u32 bloomReadHandle = resources.GetView(passData.srcTextureHandle);
					u32 bloomWriteHandle = resources.GetView(passData.dstTextureHandle);

					rd->Cmd_SetPipeline(cmdl, m_compPipeDownSample);
					auto args = ShaderArgs()
						.AppendConstant(bloomReadHandle)
						.AppendConstant(bloomWriteHandle)
						.AppendConstant(passData.width)
						.AppendConstant(passData.height);
					rd->Cmd_UpdateShaderArgs(cmdl, QueueType::Compute, args);

					u32 tgx = passData.width / computeGroupSize + 1 * static_cast<bool>(passData.width % computeGroupSize);
					u32 tgy = passData.height / computeGroupSize + 1 * static_cast<bool>(passData.height % computeGroupSize);

					rd->Cmd_ClearUnorderedAccessFLOAT(cmdl,
						resources.GetTextureView(passData.dstTextureHandle), { 0.f, 0.f, 0.f, 0.f }, ScissorRects().Append(0, 0, passData.width, passData.height));

					rd->Cmd_Dispatch(cmdl, tgx, tgy, 1);
				});
		}

		// Up scale and add back the now blured smaller texture to the texture it was downscaled from

		for (auto i = std::ssize(m_bloomTexture) - 2; i >= 0; i--)
		{
			rg.AddPass<PassData>("Bloom upscale Pass" + std::to_string(i),
				[&](PassData& passData, RenderGraph::PassBuilder& builder)		// Build
				{
					passData.level = static_cast<u32>(i);
					passData.width = m_bloomTexture[passData.level].second.width;
					passData.height = m_bloomTexture[passData.level].second.height;

					passData.srcTextureHandle = builder.ReadResource(RGResourceID("BloomTexture" + std::to_string(i + 1)), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, TextureViewDesc(ViewType::ShaderResource, TextureViewDimension::Texture2D, DXGI_FORMAT_R16G16B16A16_FLOAT));
					passData.dstTextureHandle = builder.ReadWriteTarget(RGResourceID("BloomTexture" + std::to_string(i)), TextureViewDesc(ViewType::UnorderedAccess, TextureViewDimension::Texture2D, DXGI_FORMAT_R16G16B16A16_FLOAT));
				},
				[&](const PassData& passData, RenderDevice* rd, CommandList cmdl, RenderGraph::PassResources& resources)		// Execute
				{
					u32 bloomReadHandle = resources.GetView(passData.srcTextureHandle);
					u32 bloomWriteHandle = resources.GetView(passData.dstTextureHandle);

					rd->Cmd_SetPipeline(cmdl, m_compPipeUpSample);
					auto args = ShaderArgs()
						.AppendConstant(bloomReadHandle)
						.AppendConstant(bloomWriteHandle)
						.AppendConstant(passData.width)
						.AppendConstant(passData.height);
					rd->Cmd_UpdateShaderArgs(cmdl, QueueType::Compute, args);

					u32 tgx = passData.width / computeGroupSize + 1 * static_cast<bool>(passData.width % computeGroupSize);
					u32 tgy = passData.height / computeGroupSize + 1 * static_cast<bool>(passData.height % computeGroupSize);

					rd->Cmd_Dispatch(cmdl, tgx, tgy, 1);
				});
		}


		rg.AddPass<PassData>("Bloom composit Pass",
			[&](PassData& passData, RenderGraph::PassBuilder& builder)		// Build
			{
				builder.DeclareTexture(RG_RESOURCE(FinalBloom), RGTextureDesc::ReadWrite2D(DXGI_FORMAT_R16G16B16A16_FLOAT, m_hdrRenderTargerResX, m_hdrRenderTargerResY));

				passData.srcTextureHandle = builder.ReadResource(RGResourceID("BloomTexture0"), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
					TextureViewDesc(ViewType::ShaderResource, TextureViewDimension::Texture2D, DXGI_FORMAT_R16G16B16A16_FLOAT));

				//passData.dstTextureHandle = builder.ReadWriteTarget(RG_RESOURCE(LitHDR), TextureViewDesc(ViewType::UnorderedAccess,
				//	TextureViewDimension::Texture2D, DXGI_FORMAT_R16G16B16A16_FLOAT));
				passData.dstTextureHandle = builder.ReadWriteTarget(RG_RESOURCE(FinalBloom), TextureViewDesc(ViewType::UnorderedAccess,
					TextureViewDimension::Texture2D, DXGI_FORMAT_R16G16B16A16_FLOAT));
			},
			[&](const PassData& passData, RenderDevice* rd, CommandList cmdl, RenderGraph::PassResources& resources)		// Execute
			{
				u32 bloomReadHandle = resources.GetView(passData.srcTextureHandle);
				u32 hdrRenderTargetWriteHandle = resources.GetView(passData.dstTextureHandle);

				rd->Cmd_SetPipeline(cmdl, m_compPipeUpSample);
				auto args = ShaderArgs()
					.AppendConstant(bloomReadHandle)
					.AppendConstant(hdrRenderTargetWriteHandle)
					.AppendConstant(m_hdrRenderTargerResX)
					.AppendConstant(m_hdrRenderTargerResY);
				rd->Cmd_UpdateShaderArgs(cmdl, QueueType::Compute, args);

				rd->Cmd_ClearUnorderedAccessFLOAT(cmdl,
					resources.GetTextureView(passData.dstTextureHandle), { 0.f, 0.f, 0.f, 1.f }, ScissorRects().Append(0, 0, m_hdrRenderTargerResX, m_hdrRenderTargerResY));

				u32 tgx = m_hdrRenderTargerResX / computeGroupSize + 1 * static_cast<bool>(m_hdrRenderTargerResX % computeGroupSize);
				u32 tgy = m_hdrRenderTargerResY / computeGroupSize + 1 * static_cast<bool>(m_hdrRenderTargerResY % computeGroupSize);

				rd->Cmd_Dispatch(cmdl, tgx, tgy, 1);
			});



		// Just here to help debug how the different textures look like
		//rg.AddPass<PassData>("Bloom Debug",
		//	[&](PassData& passData, RenderGraph::PassBuilder& builder)		// Build
		//	{
		//		//passData.level = m_bloomTexture.size() - 1;
		//		passData.level = 1;
		//		passData.width = m_width / 2;
		//		passData.height = m_height / 2;

		//		passData.srcTextureHandle = builder.ReadResource(RGResourceID("BloomTexture" + std::to_string(passData.level)), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
		//			TextureViewDesc(ViewType::ShaderResource, TextureViewDimension::Texture2D, DXGI_FORMAT_R16G16B16A16_FLOAT));

		//		passData.dstTextureHandle = builder.ReadWriteTarget(RG_RESOURCE(LitHDR), TextureViewDesc(ViewType::UnorderedAccess,
		//			TextureViewDimension::Texture2D, DXGI_FORMAT_R16G16B16A16_FLOAT));
		//	},
		//	[&](const PassData& passData, RenderDevice* rd, CommandList cmdl, RenderGraph::PassResources& resources)		// Execute
		//	{
		//		u32 read = resources.GetView(passData.srcTextureHandle);
		//		u32 write = resources.GetView(passData.dstTextureHandle);

		//		rd->Cmd_SetPipeline(cmdl, m_compPipDebug);
		//		auto args = ShaderArgs()
		//			.AppendConstant(read)
		//			.AppendConstant(write)
		//			.AppendConstant(passData.width)
		//			.AppendConstant(passData.height);
		//		rd->Cmd_UpdateShaderArgs(cmdl, QueueType::Compute, args);

		//		u32 tgx = passData.width / computeGroupSize + 1 * static_cast<bool>(passData.width % computeGroupSize);
		//		u32 tgy = passData.height / computeGroupSize + 1 * static_cast<bool>(passData.height % computeGroupSize);

		//		rd->Cmd_Dispatch(cmdl, tgx, tgy, 1);
		//	});
	}
	void Bloom::SetGraphicsSettings(const GraphicsSettings& settings)
	{
		m_threshold = settings.bloomThreshold;
	}
}
