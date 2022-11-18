#pragma once
#include "../GPUTable.h"
#include "../RenderEffects/ParticleEffect.h"

namespace DOG::gfx
{
	enum class ParticleSpawnType : u8
	{
		Cone = 0,
		Cylinder = 1,
		AABB = 2,

		Default = 0xFF,
	};

	struct ParticleEmitter
	{
		u32 alive = 0;
		DirectX::SimpleMath::Vector3 pos = { 0, 0, 0 };
		f32 rate = 0;
		f32 lifetime = 0;

		u32 spawnType = 0;
		f32 opt1 = 0.f;
		f32 opt2 = 0.f;
		f32 opt3 = 0.f;

		u32 textureHandle = 0;
		u32 texSegX = 1;
		u32 texSegY = 1;

		u32 pad[3] = {0, 0, 0};
	};
	
	class ParticleBackend
	{
		struct EmitterTableHandle { u64 handle{ 0 }; };
	public:
		ParticleBackend() = delete;
		ParticleBackend(RenderDevice* rd, GPUGarbageBin* bin, u32 framesInFlight, GlobalEffectData& globEffectData, RGResourceManager* resourceManager);

		~ParticleBackend() = default;

		void AddEffect(RenderGraph& rg) {
			m_particleEffect->Add(rg);
			FreeCurrentFrameTable();
		};

		void UploadEmitters(const std::vector<ParticleEmitter>& emitters);
		void FreeCurrentFrameTable() { m_emitterTable->Free(m_currentTableHandle); };

	private:
		constexpr static inline u32 S_MAX_EMITTERS = 128;

		std::unique_ptr<GPUTableHostVisible<EmitterTableHandle>> m_emitterTable;
		EmitterTableHandle m_currentTableHandle;

		std::unique_ptr<ParticleEffect> m_particleEffect;

	};

}