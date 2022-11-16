#include "PlayerManager.h"
#include "Game/GameLayer.h"

using namespace DOG;
using namespace DirectX::SimpleMath;

EntityManager& PlayerManager::s_entityManager = EntityManager::Get();
PlayerManager PlayerManager::s_amInstance;
bool PlayerManager::s_notInitialized = true;

/*******************************
		Public Methods
*******************************/

entity PlayerManager::GetThisPlayer()
{
	entity playerEntity = 0;
	EntityManager::Get().Collect<ThisPlayer>().Do([&](entity id, ThisPlayer&)
		{
			playerEntity = id;
		});
	return playerEntity;
}

u8 PlayerManager::GetNrOfPlayers()
{
	u8 nrOfPlayers = 0;
	EntityManager::Get().Collect<NetworkPlayerComponent>().Do([&](NetworkPlayerComponent&)
		{
			nrOfPlayers++;
		});
	return nrOfPlayers;
}

void PlayerManager::HurtThisPlayer(f32 damage)
{
	s_entityManager.GetComponent<PlayerStatsComponent>(GetThisPlayer()).health -= damage;
}


/*******************************
		Private Methods
*******************************/

PlayerManager::PlayerManager() noexcept
{

}


void PlayerManager::Initialize()
{
	// Set status to initialized
	s_notInitialized = false;
}

#pragma warning( disable : 4100 )
void PlayerHit::OnUpdate(entity e, HasEnteredCollisionComponent& collision, ThisPlayer&)
{
	EntityManager& eMan = EntityManager::Get();
	for (u32 i = 0; i < collision.entitiesCount; ++i)
	{
		if (eMan.HasComponent<BulletComponent>(collision.entities[i]))
		{
			if(eMan.GetComponent<BulletComponent>(collision.entities[i]).playerEntityID != PlayerManager::Get().GetThisPlayer())
				PlayerManager::Get().HurtThisPlayer(eMan.GetComponent<BulletComponent>( collision.entities[i]).damage/15.0f);
		}
	}
}
