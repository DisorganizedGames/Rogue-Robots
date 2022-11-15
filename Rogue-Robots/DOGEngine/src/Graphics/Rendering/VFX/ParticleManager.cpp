#include "ParticleManager.h"
#include "../../../ECS/EntityManager.h"
#include "../../../Core/Time.h"
#include "ParticleBackend.h"

using namespace DOG::gfx;

ParticleManager::ParticleManager()
{
	m_emitters.resize(S_MAX_EMITTERS);
}

const std::vector<ParticleEmitter>& ParticleManager::GatherEmitters()
{
	EntityManager::Get().Collect<TransformComponent, ParticleEmitterComponent>()
		.Do([this](TransformComponent& transform, ParticleEmitterComponent& emitter)
		{
			if (emitter.emitterIndex == static_cast<u32>(-1))
			{
				u32 idx = GetFreeEmitter();
				emitter.emitterIndex = idx;
				auto& em = m_emitters[idx];
				em.pos = transform.GetPosition();
				em.rate = emitter.spawnRate;
				em.lifetime = emitter.particleLifetime;
				em.alive = 1; // true
			}
			else
			{
				auto& em = m_emitters[emitter.emitterIndex];
				em.age += Time::DeltaTime<TimeType::Seconds, f32>();
			}
		});
	
	return m_emitters;
}

void ParticleManager::DeferredDeletion()
{
	EntityManager::Get().Collect<DeferredDeletionComponent, ParticleEmitterComponent>()
		.Do([this](auto&, ParticleEmitterComponent& emitter)
			{
				auto& em = m_emitters[emitter.emitterIndex];
				em.alive = 0;
			});
}

[[nodiscard]]
u32 ParticleManager::GetFreeEmitter() const noexcept
{

	for (u32 idx = 0; auto& e: m_emitters)
	{
		if (!e.alive)
			return idx;

		idx++;
	}

	ASSERT(false, "Failed to find a free space for new emitter");
	return 0;
}
