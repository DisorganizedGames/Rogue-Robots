#pragma once
#include "../GPUTable.h"

namespace DOG
{
	using entity = u32;
}

namespace DOG::gfx
{
	class RenderDevice;
	class GPUGarbageBin;
	struct ParticleEmitter;
	enum class ParticleSpawnType : u8;

	class ParticleManager
	{
		using Vector3 = DirectX::SimpleMath::Vector3;

	public:
		ParticleManager();
		~ParticleManager() = default;

		const std::vector<ParticleEmitter>& GatherEmitters();

		void DeferredDeletion();

	private:
		constexpr static inline u32 S_MAX_EMITTERS = 128;

		std::vector<ParticleEmitter> m_emitters;

	private:
		[[nodiscard]]
		u32 GetFreeEmitter() const noexcept;

		
		void SetSpawnProperties(DOG::entity e, ParticleEmitter& emitter);

	};

}