#include "ParticleManager.h"
#include "../../../ECS/EntityManager.h"
#include "../../../Core/Time.h"
#include "ParticleBackend.h"

using namespace DOG;
using namespace gfx;

ParticleManager::ParticleManager()
{
	m_emitters.resize(S_MAX_EMITTERS);
}

const std::vector<ParticleEmitter>& ParticleManager::GatherEmitters()
{
	EntityManager::Get().Collect<TransformComponent, ParticleEmitterComponent>()
		.Do([this](entity e, TransformComponent& transform, ParticleEmitterComponent& emitter)
			{
				if (emitter.emitterIndex == static_cast<u32>(-1))
				{
					emitter.emitterIndex = GetFreeEmitter();
				}

				ParticleEmitter& em = m_emitters[emitter.emitterIndex];

				em.pos = transform.GetPosition();
				em.rate = emitter.spawnRate;
				em.lifetime = emitter.particleLifetime;

				SetSpawnProperties(e, em);

				em.rotationMatrix = transform.GetRotation();

				em.textureHandle = emitter.textureHandle;
				em.texSegX = emitter.textureSegmentsX;
				em.texSegY = emitter.textureSegmentsY;

				em.startColor = emitter.startColor;
				em.endColor = emitter.endColor;

				em.alive = 1; // true
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
u32 ParticleManager::GetFreeEmitter() noexcept
{
	auto idx = m_lastEmitter++;
	m_lastEmitter = m_lastEmitter % S_MAX_EMITTERS;
	return idx;
}

void ParticleManager::SetSpawnProperties(entity e, ParticleEmitter& emitter)
{
	auto& entityManager = EntityManager::Get();
	

	if (auto opt = entityManager.TryGetComponent<ConeSpawnComponent>(e))
	{
		emitter.spawnType = (u32)ParticleSpawnType::Cone;
		emitter.opt1 = opt->get().angle;
		emitter.opt2 = opt->get().speed;
		return;
	}
	if (auto opt = entityManager.TryGetComponent<CylinderSpawnComponent>(e))
	{
		emitter.spawnType = (u32)ParticleSpawnType::Cylinder;
		emitter.opt1 = opt->get().radius;
		emitter.opt2 = opt->get().height;
		return;
	}
	if (auto opt = entityManager.TryGetComponent<BoxSpawnComponent>(e))
	{
		emitter.spawnType = (u32)ParticleSpawnType::AABB;
		emitter.opt1 = opt->get().x;
		emitter.opt2 = opt->get().y;
		emitter.opt3 = opt->get().z;
		return;
	}
	
	emitter.spawnType = (u32)ParticleSpawnType::Default;
}

