#include "Bloom.h"

#include "RECommonIncludes.h"

#include "../../RHI/RenderDevice.h"
#include "../../RHI/ShaderCompilerDXC.h"
#include "../GPUDynamicConstants.h"


namespace DOG::gfx
{
	Bloom::Bloom(GlobalEffectData& globalEffectData, GPUDynamicConstants* dynConsts)
		: RenderEffect(globalEffectData), m_dynamicConstants(dynConsts)
	{

		m_width = 1280;
		m_height = 720;

		auto& device = globalEffectData.rd;

		auto bloomShader = globalEffectData.sclr->CompileFromFile("BloomCS.hlsl", ShaderType::Compute);
		m_computePipe = device->CreateComputePipeline(ComputePipelineDesc(bloomShader.get()));
		auto downSampleShader = globalEffectData.sclr->CompileFromFile("BloomDownSample.hlsl", ShaderType::Compute);
		m_compPipDownSample = device->CreateComputePipeline(ComputePipelineDesc(downSampleShader.get()));

		auto debugShader = globalEffectData.sclr->CompileFromFile("BloomDebug.hlsl", ShaderType::Compute);
		m_compPipDebug = device->CreateComputePipeline(ComputePipelineDesc(debugShader.get()));


		m_bloomTexDesc.width = m_width;
		m_bloomTexDesc.height = m_height;
		m_bloomTexDesc.mipLevels = 1;
		m_bloomTexDesc.flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		m_bloomTexDesc.format = DXGI_FORMAT_R16G16B16A16_FLOAT;

		m_bloomTexture.emplace_back(device->CreateTexture(m_bloomTexDesc), m_bloomTexDesc);

		TextureDesc desc = m_bloomTexDesc;
		desc.width = m_bloomTexDesc.width / 2;
		desc.height = m_bloomTexDesc.height / 2;
		constexpr u32 minMipSize = 8;
		while (desc.width > minMipSize && desc.height > minMipSize)
		{
			m_bloomTexture.emplace_back(device->CreateTexture(desc), desc);
			desc.width /= 2;
			desc.height /= 2;
		}


	}
	void Bloom::Add(RenderGraph& rg)
	{
		struct PassData
		{
			GPUDynamicConstant constantBufferHandle;
			RGResourceView srcTextureHandle;
			RGResourceView dstTextureHandle;
			int level;
			u32 width;
			u32 height;
		};
		auto cb = m_dynamicConstants->Allocate(1);
		rg.AddPass<PassData>("Bloom Pass0",
			[&](PassData& passData, RenderGraph::PassBuilder& builder)		// Build
			{
				passData.constantBufferHandle = cb;
				/*passData.srcTextureHandle = builder.ReadResource(RG_RESOURCE(LitHDR), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
					TextureViewDesc(ViewType::ShaderResource, TextureViewDimension::Texture2D, DXGI_FORMAT_R16G16B16A16_FLOAT));*/

				passData.srcTextureHandle = builder.ReadWriteTarget(RG_RESOURCE(LitHDR),
					TextureViewDesc(ViewType::UnorderedAccess, TextureViewDimension::Texture2D, DXGI_FORMAT_R16G16B16A16_FLOAT));

				builder.ImportTexture(RG_RESOURCE(BloomTexture0), m_bloomTexture[0].first, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COMMON);
				passData.dstTextureHandle = builder.ReadWriteTarget(RG_RESOURCE(BloomTexture0), TextureViewDesc(ViewType::UnorderedAccess, TextureViewDimension::Texture2D, DXGI_FORMAT_R16G16B16A16_FLOAT));

				BloomConstantBuffer perDrawData{};
				perDrawData.threshold = 0.12f;
				perDrawData.color = { 1, 1, 1 };

				*reinterpret_cast<BloomConstantBuffer*>(passData.constantBufferHandle.memory) = perDrawData;
			},
			[&](const PassData& passData, RenderDevice* rd, CommandList cmdl, RenderGraph::PassResources& resources)		// Execute
			{
				u32 view = resources.GetView(passData.srcTextureHandle);
				u32 bloomTextureHandle = resources.GetView(passData.dstTextureHandle);

				rd->Cmd_SetPipeline(cmdl, m_computePipe);
				auto args = ShaderArgs()
					.AppendConstant(view)
					.AppendConstant(bloomTextureHandle)
					.AppendConstant(passData.constantBufferHandle.globalDescriptor)
					.AppendConstant(m_width)
					.AppendConstant(m_height);
				rd->Cmd_UpdateShaderArgs(cmdl, QueueType::Compute, args);

				constexpr u32 groupSize = 32;

				u32 tgx = m_width / groupSize + 1 * static_cast<bool>(m_width % groupSize);
				u32 tgy = m_height / groupSize + 1 * static_cast<bool>(m_height % groupSize);
				rd->Cmd_ClearUnorderedAccessFLOAT(cmdl,
					resources.GetTextureView(passData.dstTextureHandle), { 0.f, 0.f, 0.f, 0.f }, ScissorRects().Append(0, 0, m_width, m_height));
				rd->Cmd_Dispatch(cmdl, tgx, tgy, 1);
			});




		for (int i = 1; i < m_bloomTexture.size(); i++)
		{
			rg.AddPass<PassData>("Bloom Pass" + std::to_string(i),
				[&](PassData& passData, RenderGraph::PassBuilder& builder)		// Build
				{
					passData.level = i;
					passData.width = m_bloomTexture[passData.level].second.width;
					passData.height = m_bloomTexture[passData.level].second.height;
					builder.ImportTexture(RGResourceID("BloomTexture" + std::to_string(i)), m_bloomTexture[i].first, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COMMON);

					passData.srcTextureHandle = builder.ReadResource(RGResourceID("BloomTexture" + std::to_string(i-1)), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, TextureViewDesc(ViewType::ShaderResource, TextureViewDimension::Texture2D, DXGI_FORMAT_R16G16B16A16_FLOAT));

					passData.dstTextureHandle = builder.ReadWriteTarget(RGResourceID("BloomTexture" + std::to_string(i)), TextureViewDesc(ViewType::UnorderedAccess, TextureViewDimension::Texture2D, DXGI_FORMAT_R16G16B16A16_FLOAT));
				},
				[&, index = i](const PassData& passData, RenderDevice* rd, CommandList cmdl, RenderGraph::PassResources& resources)		// Execute
				{
					u32 bloomReadHandle = resources.GetView(passData.srcTextureHandle);
					u32 bloomWriteHandle = resources.GetView(passData.dstTextureHandle);

					rd->Cmd_SetPipeline(cmdl, m_compPipDownSample);
					auto args = ShaderArgs()
						.AppendConstant(bloomReadHandle)
						.AppendConstant(bloomWriteHandle)
						.AppendConstant(passData.width)
						.AppendConstant(passData.height);
					rd->Cmd_UpdateShaderArgs(cmdl, QueueType::Compute, args);

					constexpr u32 groupSize = 32;

					u32 tgx = passData.width / groupSize + 1 * static_cast<bool>(passData.width % groupSize);
					u32 tgy = passData.height / groupSize + 1 * static_cast<bool>(passData.height % groupSize);

					rd->Cmd_ClearUnorderedAccessFLOAT(cmdl,
						resources.GetTextureView(passData.dstTextureHandle), { 0.f, 0.f, 0.f, 0.f }, ScissorRects().Append(0, 0, passData.width, passData.height));

					rd->Cmd_Dispatch(cmdl, tgx, tgy, 1);
				});
		}
		return;
		rg.AddPass<PassData>("Bloom Debug",
			[&](PassData& passData, RenderGraph::PassBuilder& builder)		// Build
			{
				//passData.level = m_bloomTexture.size() - 1;
				passData.level = 5;
				//passData.width =m_bloomTexture[passData.level].second.width;
				//passData.height = m_bloomTexture[passData.level].second.height;
				passData.width = m_width / 2;
				passData.height = m_height / 2;
				passData.constantBufferHandle = cb;

				/*passData.srcTextureHandle = builder.ReadResource(RGResourceID("BloomTexture" + std::to_string(passData.level)), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
					TextureViewDesc(ViewType::ShaderResource, TextureViewDimension::Texture2D, DXGI_FORMAT_R16G16B16A16_FLOAT));*/

				passData.srcTextureHandle = builder.ReadResource(RGResourceID("BloomTexture" + std::to_string(passData.level)), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
					TextureViewDesc(ViewType::ShaderResource, TextureViewDimension::Texture2D, DXGI_FORMAT_R16G16B16A16_FLOAT));

				//passData.dstTextureHandle = builder.ReadWriteTarget(RG_RESOURCE(LitHDR), TextureViewDesc(ViewType::UnorderedAccess, TextureViewDimension::Texture2D, DXGI_FORMAT_R16G16B16A16_FLOAT));

				passData.dstTextureHandle = builder.ReadWriteTarget(RG_RESOURCE(LitHDR),
					TextureViewDesc(ViewType::UnorderedAccess, TextureViewDimension::Texture2D, DXGI_FORMAT_R16G16B16A16_FLOAT));
			},
			[&](const PassData& passData, RenderDevice* rd, CommandList cmdl, RenderGraph::PassResources& resources)		// Execute
			{
				u32 read = resources.GetView(passData.srcTextureHandle);
				u32 write = resources.GetView(passData.dstTextureHandle);

				rd->Cmd_SetPipeline(cmdl, m_compPipDebug);
				auto args = ShaderArgs()
					.AppendConstant(read)
					.AppendConstant(write)
					.AppendConstant(passData.constantBufferHandle.globalDescriptor)
					.AppendConstant(passData.width)
					.AppendConstant(passData.height);
				rd->Cmd_UpdateShaderArgs(cmdl, QueueType::Compute, args);

				constexpr u32 groupSize = 32;

				u32 tgx = passData.width / groupSize + 1 * static_cast<bool>(passData.width % groupSize);
				u32 tgy = passData.height / groupSize + 1 * static_cast<bool>(passData.height % groupSize);

				rd->Cmd_Dispatch(cmdl, tgx, tgy, 1);
			});




		//rg.AddPass<PassData>("Bloom Pass1",
		//	[&](PassData& passData, RenderGraph::PassBuilder& builder)		// Build
		//	{
		//		builder.ImportTexture(RG_RESOURCE(BloomTexture1), m_bloomTexture[1].first, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COMMON);

		//		passData.srcTextureHandle = builder.ReadResource(RG_RESOURCE(BloomTexture0), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, TextureViewDesc(ViewType::ShaderResource, TextureViewDimension::Texture2D, DXGI_FORMAT_R16G16B16A16_FLOAT));
		//		//passData.srcTextureHandle = builder.ReadWriteTarget(RG_RESOURCE(BloomTexture0), TextureViewDesc(ViewType::UnorderedAccess, TextureViewDimension::Texture2D, DXGI_FORMAT_R16G16B16A16_FLOAT));

		//		passData.dstTextureHandle = builder.ReadWriteTarget(RG_RESOURCE(BloomTexture1), TextureViewDesc(ViewType::UnorderedAccess, TextureViewDimension::Texture2D, DXGI_FORMAT_R16G16B16A16_FLOAT));

		//		passData.constantBufferHandle = m_dynamicConstants->Allocate(1);
		//		BloomConstantBuffer perDrawData{};
		//		perDrawData.threshold = 0.1f;
		//		perDrawData.color = { 1, 1, 1 };
		//		perDrawData.res.x = m_width;
		//		perDrawData.res.y = m_height;

		//		*reinterpret_cast<BloomConstantBuffer*>(passData.constantBufferHandle.memory) = perDrawData;
		//	},
		//	[&](const PassData& passData, RenderDevice* rd, CommandList cmdl, RenderGraph::PassResources& resources)		// Execute
		//	{
		//		u32 bloomReadHandle = resources.GetView(passData.srcTextureHandle);
		//		u32 bloomWriteHandle = resources.GetView(passData.dstTextureHandle);

		//		rd->Cmd_SetPipeline(cmdl, m_compPipDownSample);
		//		auto args = ShaderArgs()
		//			.AppendConstant(bloomReadHandle)
		//			.AppendConstant(bloomWriteHandle)
		//			.AppendConstant(passData.constantBufferHandle.globalDescriptor)
		//			.AppendConstant(m_width/2)
		//			.AppendConstant(m_height/2);
		//		rd->Cmd_UpdateShaderArgs(cmdl, QueueType::Compute, args);

		//		constexpr u32 groupSize = 32;

		//		u32 tgx = (m_width/2)  / groupSize + 1 * static_cast<bool>((m_width / 2) % groupSize);
		//		u32 tgy = (m_height/2) / groupSize + 1 * static_cast<bool>((m_height / 2) % groupSize);

		//		rd->Cmd_Dispatch(cmdl, tgx, tgy, 1);

		//		/*rd->Cmd_ClearUnorderedAccessFLOAT(cmdl,
		//			resources.GetTextureView(passData.srcTextureHandle), { 0.f, 0.f, 0.f, 0.f }, ScissorRects().Append(0, 0, m_width, m_height));*/
		//	});
	}
}
