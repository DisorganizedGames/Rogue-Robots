#include "HomingMissileSystem.h"
#include "AgentManager\AgentComponents.h"
using namespace DOG;
using namespace DirectX::SimpleMath;











void HomingMissileImpacteSystem::OnUpdate(entity e, HomingMissileComponent& missile, DOG::HasEnteredCollisionComponent& collision, DOG::TransformComponent& transform)
{
	if (missile.launched && collision.entitiesCount > 0)
	{
		EntityManager::Get().Collect<AgentHitComponent, DOG::TransformComponent>().Do([&](AgentHitComponent& agentHit, DOG::TransformComponent& tr)
			{
				float distSquared = Vector3::DistanceSquared(transform.GetPosition(), tr.GetPosition());
				if (distSquared < missile.explosionRadius * missile.explosionRadius)
				{
					agentHit.HitBy({e, missile.dmg / (1.0f + distSquared) , missile.playerNetworkID });
				}
			});

		EntityManager::Get().AddComponent<ExplosionEffectComponent>(e, 0.8f * missile.explosionRadius);
		EntityManager::Get().DeferredEntityDestruction(e);
	}
}

void HomingMissileTargetingSystem::OnUpdate(HomingMissileComponent& missile, DOG::TransformComponent& transform)
{
	float minDistSquared = 1000000;
	entity target = NULL_ENTITY;
	if (missile.homingTarget == NULL_ENTITY)
	{
		EntityManager::Get().Collect<AgentHPComponent, DOG::TransformComponent>().Do([&](entity e, AgentHPComponent&, DOG::TransformComponent& tr)
			{
				float distSquared = Vector3::DistanceSquared(tr.GetPosition(), transform.GetPosition());
				if (distSquared < minDistSquared)
				{
					minDistSquared = distSquared;
					target = e;
				}
			});
		missile.homingTarget = target;
	}
}
