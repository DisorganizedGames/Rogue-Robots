#include "HomingMissileSystem.h"
#include "AgentManager\AgentComponents.h"
using namespace DOG;
using namespace DirectX::SimpleMath;











void HomingMissileImpacteSystem::OnUpdate(entity e, HomingMissileComponent& missile, DOG::HasEnteredCollisionComponent& collision)
{
	if (missile.launched && collision.entitiesCount > 0)
	{
		for (u32 i = 0; i < collision.entitiesCount; i++)
		{
			if (EntityManager::Get().HasComponent<AgentHPComponent>(collision.entities[i]))
			{
				EntityManager::Get().GetComponent<AgentHPComponent>(collision.entities[i]).hp = 0;
			}
		}
		EntityManager::Get().AddComponent<ExplosionEffectComponent>(e, 5.0f);
		EntityManager::Get().DeferredEntityDestruction(e);
	}
}