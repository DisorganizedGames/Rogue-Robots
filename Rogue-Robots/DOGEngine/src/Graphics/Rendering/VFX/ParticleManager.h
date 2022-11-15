#pragma once
#include "../GPUTable.h"

namespace DOG::gfx
{
	class RenderDevice;
	class GPUGarbageBin;
	struct ParticleEmitter;

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

	};

}