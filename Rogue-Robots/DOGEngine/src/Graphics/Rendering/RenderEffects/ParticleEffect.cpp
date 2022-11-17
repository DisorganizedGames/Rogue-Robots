#include "ParticleEffect.h"
#include "RECommonIncludes.h"
#include "../../src/Core/Time.h"

#include "../UploadContext.h"
#include "../../RHI/RenderDevice.h"
#include "../../RHI/ShaderCompilerDXC.h"
#include "../../RHI/PipelineBuilder.h"
#include "../GPUTable.h"

using namespace DOG::gfx;

ParticleEffect::ParticleEffect(GlobalEffectData& globalEffectData, RGResourceManager* resourceManager, u32 maxEmitters) :
	RenderEffect(globalEffectData), m_maxEmitters(maxEmitters)
{
	auto shaderCompiler = m_globalEffectData.sclr;
	auto device = m_globalEffectData.rd;
	m_resourceManager = resourceManager;

	auto emitterShader = shaderCompiler->CompileFromFile("Particles/EmitterCS.hlsl", ShaderType::Compute);
	m_emitPipeline = device->CreateComputePipeline(ComputePipelineDesc(emitterShader.get()));

	auto compactShader = shaderCompiler->CompileFromFile("Particles/CompactionCS.hlsl", ShaderType::Compute);
	m_compactPipeline = device->CreateComputePipeline(ComputePipelineDesc(compactShader.get()));

	auto updateShader = shaderCompiler->CompileFromFile("Particles/BasicUpdateCS.hlsl", ShaderType::Compute);
	m_updatePipeline = device->CreateComputePipeline(ComputePipelineDesc(updateShader.get()));
	
	auto drawVS = shaderCompiler->CompileFromFile("Particles/ParticleVS.hlsl", ShaderType::Vertex);
	auto drawPS = shaderCompiler->CompileFromFile("Particles/ParticlePS.hlsl", ShaderType::Pixel);
	m_drawPipeline = device->CreateGraphicsPipeline(GraphicsPipelineBuilder()
		.SetShader(drawVS.get())
		.SetShader(drawPS.get())
		.AppendRTFormat(DXGI_FORMAT_R16G16B16A16_FLOAT)
		.SetDepthFormat(DepthFormat::D32)
		.SetDepthStencil(DepthStencilBuilder().SetDepthEnabled(true))
		.SetBlend(BlendBuilder().AppendRTBlend(D3D12_RENDER_TARGET_BLEND_DESC{
				.BlendEnable = true,
				.LogicOpEnable = false,
				.SrcBlend = D3D12_BLEND_SRC_ALPHA,
				.DestBlend = D3D12_BLEND_INV_SRC_ALPHA,
				.BlendOp = D3D12_BLEND_OP_ADD,
				.SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA,
				.DestBlendAlpha = D3D12_BLEND_DEST_ALPHA,
				.BlendOpAlpha = D3D12_BLEND_OP_MAX,
				.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL,
			}).SetAlphaToCoverageEnabled(true))
		.Build()
	);

	{
		BufferDesc particleBufferDesc(MemoryType::Default, S_MAX_PARTICLES * sizeof(Particle),
			D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		m_particleBuffer = device->CreateBuffer(particleBufferDesc);

		m_resourceManager->ImportBuffer(RG_RESOURCE(ParticleBuffer), m_particleBuffer,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	}

	{
		BufferDesc aliveBufferDesc(MemoryType::Default, S_COUNTERS * sizeof(u32),
			D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		m_particlesAlive = device->CreateBuffer(aliveBufferDesc);

		m_resourceManager->ImportBuffer(RG_RESOURCE(ParticlesAliveBuffer), m_particlesAlive,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	}

	{
		BufferDesc toSpawnDesc(MemoryType::Default, m_maxEmitters * sizeof(f32),
			D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		m_emitterToSpawn = device->CreateBuffer(toSpawnDesc);

		m_resourceManager->ImportBuffer(RG_RESOURCE(EmitterToSpawnBuffer), m_emitterToSpawn,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	}
}

ParticleEffect::~ParticleEffect()
{
	auto device = m_globalEffectData.rd;

	device->FreeBuffer(m_particleBuffer);
	device->FreeBuffer(m_particlesAlive);
	device->FreeBuffer(m_emitterToSpawn);
	
	m_resourceManager->FreeImported(RG_RESOURCE(ParticleBuffer));
	m_resourceManager->FreeImported(RG_RESOURCE(ParticlesAliveBuffer));
	m_resourceManager->FreeImported(RG_RESOURCE(EmitterToSpawnBuffer));
}

void ParticleEffect::Add(RenderGraph& renderGraph)
{
	struct PassData
	{
		RGResourceView particleBufferHandle;
		RGResourceView particlesAliveHandle;
		RGResourceView emitterToSpawnHandle;
	};

	renderGraph.AddPass<PassData>("Particle Emitter Pass",
		[this](PassData& passData, RenderGraph::PassBuilder& builder) // Build
		{
			passData.particleBufferHandle = builder.ReadWriteTarget(RG_RESOURCE(ParticleBuffer),
				BufferViewDesc(ViewType::UnorderedAccess, 0, sizeof(Particle), S_MAX_PARTICLES));

			passData.particlesAliveHandle = builder.ReadWriteTarget(RG_RESOURCE(ParticlesAliveBuffer),
				BufferViewDesc(ViewType::UnorderedAccess, 0, sizeof(u32), S_COUNTERS));

			passData.emitterToSpawnHandle = builder.ReadWriteTarget(RG_RESOURCE(EmitterToSpawnBuffer),
				BufferViewDesc(ViewType::UnorderedAccess, 0, sizeof(u32), m_maxEmitters));
		},
		[this](const PassData& passData, RenderDevice* rd, CommandList cmdl, RenderGraph::PassResources& resources) // Execute
		{
			rd->Cmd_SetPipeline(cmdl, m_emitPipeline);

			ShaderArgs shaderArgs;
			shaderArgs
				.AppendConstant(m_globalEffectData.globalDataDescriptor)
				.AppendConstant(*m_globalEffectData.perFrameTableOffset)
				.AppendConstant(m_emitterGlobalDescriptor)
				.AppendConstant(m_emitterLocalOffset)
				.AppendConstant(resources.GetView(passData.particleBufferHandle))
				.AppendConstant(resources.GetView(passData.particlesAliveHandle))
				.AppendConstant(resources.GetView(passData.emitterToSpawnHandle));

			rd->Cmd_UpdateShaderArgs(cmdl, QueueType::Compute, shaderArgs);

			rd->Cmd_Dispatch(cmdl, m_maxEmitters, 1, 1);
		},
		[](PassData&) // Pre-graph execution
		{
			
		},
		[](PassData&) // Post-graph execution
		{

		});

	renderGraph.AddPass<PassData>("Particle Compaction Pass",
		[this](PassData& passData, RenderGraph::PassBuilder& builder) // Build
		{
			passData.particleBufferHandle = builder.ReadWriteTarget(RG_RESOURCE(ParticleBuffer),
				BufferViewDesc(ViewType::UnorderedAccess, 0, sizeof(Particle), S_MAX_PARTICLES));
			passData.particlesAliveHandle = builder.ReadWriteTarget(RG_RESOURCE(ParticlesAliveBuffer),
				BufferViewDesc(ViewType::UnorderedAccess, 0, sizeof(u32), S_COUNTERS));
		},
		[this](const PassData& passData, RenderDevice* rd, CommandList cmdl, RenderGraph::PassResources& resources) // Execute
		{
			rd->Cmd_SetPipeline(cmdl, m_compactPipeline);
		ShaderArgs shaderArgs;
			shaderArgs
				.AppendConstant(m_globalEffectData.globalDataDescriptor)
				.AppendConstant(*m_globalEffectData.perFrameTableOffset)
				.AppendConstant(m_emitterGlobalDescriptor)
				.AppendConstant(m_emitterLocalOffset)
				.AppendConstant(resources.GetView(passData.particleBufferHandle))
				.AppendConstant(resources.GetView(passData.particlesAliveHandle));
		rd->Cmd_UpdateShaderArgs(cmdl, QueueType::Compute, shaderArgs);
		rd->Cmd_Dispatch(cmdl, S_MAX_PARTICLES / 256, 1, 1);
		},
			[](PassData&) // Pre-graph execution
		{
	},
			[](PassData&) // Post-graph execution
		{
	});

	renderGraph.AddPass<PassData>("Particle Update Pass",
		[this](PassData& passData, RenderGraph::PassBuilder& builder) // Build
		{
			passData.particleBufferHandle = builder.ReadWriteTarget(RG_RESOURCE(ParticleBuffer),
				BufferViewDesc(ViewType::UnorderedAccess, 0, sizeof(Particle), S_MAX_PARTICLES));

			passData.particlesAliveHandle = builder.ReadWriteTarget(RG_RESOURCE(ParticlesAliveBuffer),
				BufferViewDesc(ViewType::UnorderedAccess, 0, sizeof(u32), S_COUNTERS));

		},
		[this](const PassData& passData, RenderDevice* rd, CommandList cmdl, RenderGraph::PassResources& resources) // Execute
		{
			rd->Cmd_SetPipeline(cmdl, m_updatePipeline);

			ShaderArgs shaderArgs;
			shaderArgs
				.AppendConstant(m_globalEffectData.globalDataDescriptor)
				.AppendConstant(*m_globalEffectData.perFrameTableOffset)
				.AppendConstant(m_emitterGlobalDescriptor)
				.AppendConstant(m_emitterLocalOffset)
				.AppendConstant(resources.GetView(passData.particleBufferHandle))
				.AppendConstant(resources.GetView(passData.particlesAliveHandle));

			rd->Cmd_UpdateShaderArgs(cmdl, QueueType::Compute, shaderArgs);

			rd->Cmd_Dispatch(cmdl, S_MAX_PARTICLES / 128, 1, 1);
		},
		[](PassData&) // Pre-graph execution
		{

		},
		[](PassData&) // Post-graph execution
		{

		});

	renderGraph.AddPass<PassData>("Particle Draw Pass",
		[this](PassData& passData, RenderGraph::PassBuilder& builder) // Build
		{
			passData.particleBufferHandle = builder.ReadWriteTarget(RG_RESOURCE(ParticleBuffer),
				BufferViewDesc(ViewType::UnorderedAccess, 0, sizeof(Particle), S_MAX_PARTICLES));

			passData.particlesAliveHandle = builder.ReadWriteTarget(RG_RESOURCE(ParticlesAliveBuffer),
				BufferViewDesc(ViewType::UnorderedAccess, 0, sizeof(u32), S_COUNTERS));

			builder.WriteRenderTarget(RG_RESOURCE(LitHDR), RenderPassAccessType::PreservePreserve,
				TextureViewDesc(ViewType::RenderTarget, TextureViewDimension::Texture2D, DXGI_FORMAT_R16G16B16A16_FLOAT));

			builder.WriteDepthStencil(RG_RESOURCE(MainDepth), RenderPassAccessType::PreservePreserve,
				TextureViewDesc(ViewType::DepthStencil, TextureViewDimension::Texture2D, DXGI_FORMAT_D32_FLOAT));

		},
		[this](const PassData& passData, RenderDevice* rd, CommandList cmdl, RenderGraph::PassResources& resources) // Execute
		{
			rd->Cmd_SetPipeline(cmdl, m_drawPipeline);

			ShaderArgs shaderArgs;
			shaderArgs
				.AppendConstant(m_globalEffectData.globalDataDescriptor)
				.AppendConstant(*m_globalEffectData.perFrameTableOffset)
				.AppendConstant(m_emitterGlobalDescriptor)
				.AppendConstant(m_emitterLocalOffset)
				.AppendConstant(resources.GetView(passData.particleBufferHandle))
				.AppendConstant(resources.GetView(passData.particlesAliveHandle));

			rd->Cmd_UpdateShaderArgs(cmdl, QueueType::Graphics, shaderArgs);

			rd->Cmd_Draw(cmdl, 6, S_MAX_PARTICLES, 0, 0);
		},
		[](PassData&) // Pre-graph execution
		{

		},
		[](PassData&) // Post-graph execution
		{

		});

}

