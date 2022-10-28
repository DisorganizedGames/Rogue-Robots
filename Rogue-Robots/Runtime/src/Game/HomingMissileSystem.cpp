#include "HomingMissileSystem.h"
#include "AgentManager\AgentComponents.h"
using namespace DOG;
using namespace DirectX::SimpleMath;


void HomingMissileSystem::OnUpdate(HomingMissileComponent& missile, DOG::TransformComponent& transform, DOG::RigidbodyComponent& rigidBody)
{
	if (missile.launched && missile.engineBurnTime > 0)
	{
		missile.engineBurnTime -= DOG::Time::DeltaTime<DOG::TimeType::Seconds, f32>();

		Vector3 forward = -transform.worldMatrix.Forward();
		if (DOG::EntityManager::Get().Exists(missile.homingTarget) && DOG::EntityManager::Get().HasComponent<DOG::TransformComponent>(missile.homingTarget))
		{
			Vector3 target = DOG::EntityManager::Get().GetComponent<DOG::TransformComponent>(missile.homingTarget).GetPosition();
			Vector3 targetDir = target - transform.GetPosition();
			targetDir.Normalize();
			Vector3 t = forward.Cross(targetDir);
			rigidBody.angularVelocity = missile.turnSpeed * t;
			rigidBody.linearVelocity = missile.speed * forward;
		}
		else if (missile.homeInOnPosition)
		{
			Vector3 targetDir = missile.targetPosition - transform.GetPosition();
			targetDir.Normalize();
			Vector3 t = forward.Cross(targetDir);
			rigidBody.angularVelocity = missile.turnSpeed * t;
			rigidBody.linearVelocity = missile.speed * forward;
		}
		else
		{
			rigidBody.linearVelocity = missile.speed * forward;
		}
	}
}


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
