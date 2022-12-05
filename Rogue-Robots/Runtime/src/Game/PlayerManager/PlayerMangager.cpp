#include "PlayerManager.h"
#include "Game/GameLayer.h"
#include "../DOGEngine/src/Graphics/Rendering/PostProcess.h"

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

bool PlayerManager::IsThisPlayerHost()
{
	if (s_entityManager.GetComponent<NetworkPlayerComponent>(GetThisPlayer()).playerId == 0)
		return true;
	else
		return false;
}

bool PlayerManager::IsThisMultiplayer()
{
	bool nStatus = false;
	EntityManager::Get().Collect<OnlinePlayer>().Do([&](OnlinePlayer&)
		{
			nStatus = true;
		});
	return nStatus;
}

void PlayerManager::HurtOnlinePlayers(entity player)
{
	std::cout << u32(player) << std::endl; //removes warning, is being used in another branch, remove this line when function is used
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
			if (eMan.GetComponent<BulletComponent>(collision.entities[i]).playerEntityID != PlayerManager::Get().GetThisPlayer())
			{
				PlayerManager::Get().HurtThisPlayer(eMan.GetComponent<BulletComponent>( collision.entities[i]).damage/TEAM_DAMAGE_MODIFIER); 
				EntityManager::Get().Collect<ThisPlayer, InputController>().Do(
					[&](ThisPlayer&, InputController& inputC)
					{
						inputC.teamDamageTaken += eMan.GetComponent<BulletComponent>(collision.entities[i]).damage / TEAM_DAMAGE_MODIFIER;
					});

				if (EntityManager::Get().HasComponent<PlayerAliveComponent>(e))
				{
					// Add visual effect
					const auto& pos1 = eMan.GetComponent<TransformComponent>(collision.entities[i]).GetPosition();
					const auto& pos2 = eMan.GetComponent<TransformComponent>(e).GetPosition();
					auto dir = pos1 - pos2;
					dir.Normalize();

					DOG::gfx::PostProcess::Get().InstantiateDamageDisk({ dir.x, dir.z }, 2.f, 1.5f);
				}
			}
		}

		if (auto dmgDealer = eMan.TryGetComponent<TeamDamageDealerComponent>(collision.entities[i]))
		{
			if (dmgDealer->get().canDamageSelf || dmgDealer->get().playerEntityID != PlayerManager::Get().GetThisPlayer())
			{
				float dmg = dmgDealer->get().damage / TEAM_DAMAGE_MODIFIER;
				PlayerManager::Get().HurtThisPlayer(dmg);
				eMan.GetComponent<InputController>(e).teamDamageTaken += dmg;

				if (EntityManager::Get().HasComponent<PlayerAliveComponent>(e))
				{
					// Add visual effect
					const auto& pos1 = eMan.GetComponent<TransformComponent>(collision.entities[i]).GetPosition();
					const auto& pos2 = eMan.GetComponent<TransformComponent>(e).GetPosition();
					auto dir = pos1 - pos2;
					dir.Normalize();

					DOG::gfx::PostProcess::Get().InstantiateDamageDisk({ dir.x, dir.z }, 2.f, 1.5f);
				}
			}
		}
	}
}
