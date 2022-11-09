#include "ParticleEffect.h"
#include "RECommonIncludes.h"
#include "../../src/Core/Time.h"

#include "../UploadContext.h"
#include "../../RHI/RenderDevice.h"
#include "../../RHI/ShaderCompilerDXC.h"
#include "../../RHI/PipelineBuilder.h"

using namespace DOG::gfx;

ParticleEffect::ParticleEffect(GlobalEffectData& globalEffectData, RGResourceManager* resourceManager, UploadContext*) :
	RenderEffect(globalEffectData)
{
	auto shaderCompiler = m_globalEffectData.sclr;
	auto device = m_globalEffectData.rd;
	m_resourceManager = resourceManager;

	auto emitterShader = shaderCompiler->CompileFromFile("Particles/EmitterCS.hlsl", ShaderType::Compute);
	m_emitPipeline = device->CreateComputePipeline(ComputePipelineDesc(emitterShader.get()));

	auto updateShader = shaderCompiler->CompileFromFile("Particles/BasicUpdateCS.hlsl", ShaderType::Compute);
	m_updatePipeline = device->CreateComputePipeline(ComputePipelineDesc(updateShader.get()));
	
	auto drawVS = shaderCompiler->CompileFromFile("Particles/ParticleVS.hlsl", ShaderType::Vertex);
	auto drawPS = shaderCompiler->CompileFromFile("Particles/ParticlePS.hlsl", ShaderType::Pixel);
	m_drawPipeline = device->CreateGraphicsPipeline(GraphicsPipelineBuilder()
		.SetShader(drawVS.get())
		.SetShader(drawPS.get())
		.AppendRTFormat(DXGI_FORMAT_R16G16B16A16_FLOAT)
		.Build()
	);

	{
		BufferDesc emitterBufferDesc(MemoryType::Default, S_MAX_EMITTERS * sizeof(Emitter),
			D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_DEST);
		m_emitterBuffer = device->CreateBuffer(emitterBufferDesc);

		m_resourceManager->ImportBuffer(RG_RESOURCE(ParticleEmitterBuffer), m_emitterBuffer,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	}

	{
		BufferDesc particleBufferDesc(MemoryType::Default, S_MAX_PARTICLES * sizeof(Particle),
			D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		m_particleBuffer = device->CreateBuffer(particleBufferDesc);

		m_resourceManager->ImportBuffer(RG_RESOURCE(ParticleBuffer), m_particleBuffer,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	}

	{
		BufferDesc aliveBufferDesc(MemoryType::Default, sizeof(u32),
			D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		m_particlesAlive = device->CreateBuffer(aliveBufferDesc);

		m_resourceManager->ImportBuffer(RG_RESOURCE(ParticlesAliveBuffer), m_particlesAlive,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	}
}

ParticleEffect::~ParticleEffect()
{
	auto device = m_globalEffectData.rd;

	device->FreeBuffer(m_emitterBuffer);
	device->FreeBuffer(m_particleBuffer);
	device->FreeBuffer(m_particlesAlive);
	
	m_resourceManager->FreeImported(RG_RESOURCE(ParticleEmitterBuffer));
	m_resourceManager->FreeImported(RG_RESOURCE(ParticleBuffer));
	m_resourceManager->FreeImported(RG_RESOURCE(ParticlesAliveBuffer));
}

void ParticleEffect::Add(RenderGraph& renderGraph)
{
	struct PassData
	{
		RGResourceView emitterBufferHandle;
		RGResourceView particleBufferHandle;
		RGResourceView particlesAliveHandle;
	};

	renderGraph.AddPass<PassData>("Particle Emitter Pass",
		[this](PassData& passData, RenderGraph::PassBuilder& builder) // Build
		{
			passData.emitterBufferHandle = builder.ReadWriteTarget(RG_RESOURCE(ParticleEmitterBuffer),
				BufferViewDesc(ViewType::UnorderedAccess, 0, sizeof(Emitter), S_MAX_EMITTERS));

			passData.particleBufferHandle = builder.ReadWriteTarget(RG_RESOURCE(ParticleBuffer),
				BufferViewDesc(ViewType::UnorderedAccess, 0, sizeof(Particle), S_MAX_PARTICLES));

			passData.particlesAliveHandle = builder.ReadWriteTarget(RG_RESOURCE(ParticlesAliveBuffer),
				BufferViewDesc(ViewType::UnorderedAccess, 0, sizeof(u32), 1));

		},
		[this](const PassData& passData, RenderDevice* rd, CommandList cmdl, RenderGraph::PassResources& resources) // Execute
		{
			rd->Cmd_SetPipeline(cmdl, m_emitPipeline);

			ShaderArgs shaderArgs;
			shaderArgs
				.AppendConstant(resources.GetView(passData.emitterBufferHandle))
				.AppendConstant(resources.GetView(passData.particleBufferHandle))
				.AppendConstant(resources.GetView(passData.particlesAliveHandle));

			rd->Cmd_UpdateShaderArgs(cmdl, QueueType::Compute, shaderArgs);

			rd->Cmd_Dispatch(cmdl, 1, 1, 1);
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
			passData.emitterBufferHandle = builder.ReadWriteTarget(RG_RESOURCE(ParticleEmitterBuffer),
				BufferViewDesc(ViewType::UnorderedAccess, 0, sizeof(Emitter), S_MAX_EMITTERS));

			passData.particleBufferHandle = builder.ReadWriteTarget(RG_RESOURCE(ParticleBuffer),
				BufferViewDesc(ViewType::UnorderedAccess, 0, sizeof(Particle), S_MAX_PARTICLES));

			passData.particlesAliveHandle = builder.ReadWriteTarget(RG_RESOURCE(ParticlesAliveBuffer),
				BufferViewDesc(ViewType::UnorderedAccess, 0, sizeof(u32), 1));

		},
		[this](const PassData& passData, RenderDevice* rd, CommandList cmdl, RenderGraph::PassResources& resources) // Execute
		{
			rd->Cmd_SetPipeline(cmdl, m_updatePipeline);

			ShaderArgs shaderArgs;
			shaderArgs
				.AppendConstant(m_globalEffectData.globalDataDescriptor)
				.AppendConstant(*m_globalEffectData.perFrameTableOffset)
				.AppendConstant(resources.GetView(passData.emitterBufferHandle))
				.AppendConstant(resources.GetView(passData.particleBufferHandle))
				.AppendConstant(resources.GetView(passData.particlesAliveHandle));

			rd->Cmd_UpdateShaderArgs(cmdl, QueueType::Compute, shaderArgs);

			rd->Cmd_Dispatch(cmdl, 1, 1, 1);
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
			passData.emitterBufferHandle = builder.ReadWriteTarget(RG_RESOURCE(ParticleEmitterBuffer),
				BufferViewDesc(ViewType::UnorderedAccess, 0, sizeof(Emitter), S_MAX_EMITTERS));

			passData.particleBufferHandle = builder.ReadWriteTarget(RG_RESOURCE(ParticleBuffer),
				BufferViewDesc(ViewType::UnorderedAccess, 0, sizeof(Particle), S_MAX_PARTICLES));

			passData.particlesAliveHandle = builder.ReadWriteTarget(RG_RESOURCE(ParticlesAliveBuffer),
				BufferViewDesc(ViewType::UnorderedAccess, 0, sizeof(u32), 1));

			builder.WriteRenderTarget(RG_RESOURCE(LitHDR), RenderPassAccessType::PreservePreserve,
				TextureViewDesc(ViewType::RenderTarget, TextureViewDimension::Texture2D, DXGI_FORMAT_R16G16B16A16_FLOAT));

		},
		[this](const PassData& passData, RenderDevice* rd, CommandList cmdl, RenderGraph::PassResources& resources) // Execute
		{
			rd->Cmd_SetPipeline(cmdl, m_drawPipeline);

			ShaderArgs shaderArgs;
			shaderArgs
				.AppendConstant(m_globalEffectData.globalDataDescriptor)
				.AppendConstant(*m_globalEffectData.perFrameTableOffset)
				.AppendConstant(resources.GetView(passData.emitterBufferHandle))
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

