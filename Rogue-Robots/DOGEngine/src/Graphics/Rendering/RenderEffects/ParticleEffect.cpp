#include "ParticleEffect.h"
#include "RECommonIncludes.h"

#include "../UploadContext.h"
#include "../../RHI/RenderDevice.h"
#include "../../RHI/ShaderCompilerDXC.h"

using namespace DOG::gfx;

ParticleEffect::ParticleEffect(GlobalEffectData& globalEffectData, RGResourceManager* resourceManager, UploadContext*) :
	RenderEffect(globalEffectData)
{
	auto shaderCompiler = m_globalEffectData.sclr;
	auto device = m_globalEffectData.rd;
	m_resourceManager = resourceManager;

	auto emitterShader = shaderCompiler->CompileFromFile("Particles/EmitterCS.hlsl", ShaderType::Compute);
	m_emitPipeline = device->CreateComputePipeline(ComputePipelineDesc(emitterShader.get()));

	BufferDesc emitterBufferDesc(MemoryType::Default, S_MAX_EMITTERS * sizeof(u32),
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_DEST);

	m_emitterBuffer = device->CreateBuffer(emitterBufferDesc);

	m_resourceManager->ImportBuffer(RG_RESOURCE(ParticleEmitterBuffer), m_emitterBuffer,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	BufferDesc particleBufferDesc(MemoryType::Default, S_MAX_PARTICLES * sizeof(Particle),
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	m_particleBuffer = device->CreateBuffer(particleBufferDesc);

	m_resourceManager->ImportBuffer(RG_RESOURCE(ParticleBuffer), m_particleBuffer,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
}

ParticleEffect::~ParticleEffect()
{
	auto device = m_globalEffectData.rd;

	device->FreeBuffer(m_emitterBuffer);
	device->FreeBuffer(m_particleBuffer);
	
	m_resourceManager->FreeImported(RG_RESOURCE(ParticleEmitterBuffer));
	m_resourceManager->FreeImported(RG_RESOURCE(ParticleBuffer));
}

void ParticleEffect::Add(RenderGraph& renderGraph)
{
	struct PassData
	{
		RGResourceView emitterBufferHandle;
		RGResourceView particleBufferHandle;

		Pipeline* emitPipeline = nullptr;
		Pipeline* updatePipeline = nullptr;
	};

	renderGraph.AddPass<PassData>("Particle Emitter Pass",
		[this](PassData& passData, RenderGraph::PassBuilder& builder) // Build
		{
			passData.emitterBufferHandle = builder.ReadWriteTarget(RG_RESOURCE(ParticleEmitterBuffer),
				BufferViewDesc(ViewType::UnorderedAccess, 0, sizeof(u32), S_MAX_EMITTERS));

			passData.particleBufferHandle = builder.ReadWriteTarget(RG_RESOURCE(ParticleBuffer),
				BufferViewDesc(ViewType::UnorderedAccess, 0, sizeof(Particle), S_MAX_PARTICLES));

			passData.emitPipeline = &m_emitPipeline;

		},
		[](const PassData& passData, RenderDevice* rd, CommandList cmdl, RenderGraph::PassResources& resources) // Execute
		{
			rd->Cmd_SetPipeline(cmdl, *passData.emitPipeline);

			ShaderArgs shaderArgs;
			shaderArgs.AppendConstant(resources.GetView(passData.emitterBufferHandle));
			shaderArgs.AppendConstant(resources.GetView(passData.particleBufferHandle));

			rd->Cmd_UpdateShaderArgs(cmdl, QueueType::Compute, shaderArgs);

			rd->Cmd_Dispatch(cmdl, 1, 1, 1);
		},
		[](PassData&) // Pre-graph execution
		{
			
		},
		[](PassData&) // Post-graph execution
		{

		});

	// -- UPDATE PASS --

	renderGraph.AddPass<PassData>("Particle Update Pass",
		[](PassData& passData, RenderGraph::PassBuilder& builder) // Build
		{
			passData.emitterBufferHandle = builder.ReadWriteTarget(RG_RESOURCE(ParticleEmitterBuffer),
				BufferViewDesc(ViewType::UnorderedAccess, 0, sizeof(u32), S_MAX_EMITTERS));

			passData.particleBufferHandle = builder.ReadWriteTarget(RG_RESOURCE(ParticleBuffer),
				BufferViewDesc(ViewType::UnorderedAccess, 0, sizeof(Particle), S_MAX_PARTICLES));
		},
		[](const PassData& passData, RenderDevice* rd, CommandList cmdl, RenderGraph::PassResources& resources) // Execute
		{
			ShaderArgs shaderArgs;
			shaderArgs.AppendConstant(resources.GetView(passData.emitterBufferHandle));
			shaderArgs.AppendConstant(resources.GetView(passData.particleBufferHandle));

			rd->Cmd_UpdateShaderArgs(cmdl, QueueType::Compute, shaderArgs);
		},
		[](PassData&) // Pre-graph execution
		{
			
		},
		[](PassData&) // Post-graph execution
		{

		});
}

