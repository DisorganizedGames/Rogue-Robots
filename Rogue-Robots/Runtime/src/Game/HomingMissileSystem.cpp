#include "HomingMissileSystem.h"

using namespace DOG;
using namespace DirectX::SimpleMath;











void HomingMissileImpacteSystem::OnUpdate(entity e, HomingMissileComponent& missile, DOG::HasEnteredCollisionComponent& collision)
{
	if (missile.launched && collision.entitiesCount > 0)
	{
		EntityManager::Get().DeferredEntityDestruction(e);
	}
}