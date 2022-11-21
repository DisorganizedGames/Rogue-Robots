#include "ParticleBackend.h"

using namespace DOG::gfx;

ParticleBackend::ParticleBackend(RenderDevice* rd, GPUGarbageBin* bin, u32 framesInFlight, GlobalEffectData& globEffectData, RGResourceManager* resourceManager, UploadContext* upCtx)
{
	m_emitterTable = std::make_unique<GPUTableHostVisible<EmitterTableHandle>>(rd, bin, (u32)sizeof(ParticleEmitter), S_MAX_EMITTERS * (framesInFlight + 1));
	m_particleEffect = std::make_unique<ParticleEffect>(globEffectData, resourceManager, upCtx, S_MAX_EMITTERS);
}

void ParticleBackend::UploadEmitters(const std::vector<ParticleEmitter>& emitters)
{
	m_currentTableHandle = m_emitterTable->Allocate(S_MAX_EMITTERS, (void*)emitters.data());
	m_particleEffect->SetEmitters(m_emitterTable->GetGlobalDescriptor(), m_emitterTable->GetLocalOffset(m_currentTableHandle));
}
