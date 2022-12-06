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

	PlayHurtAudio(GetThisPlayer());
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
	PlayHurtAudio(player);
}

i8 PlayerManager::GetPlayerId(entity player)
{
	if(s_entityManager.HasComponent<NetworkPlayerComponent>(player))
		return s_entityManager.GetComponent<NetworkPlayerComponent>(player).playerId;
	return -1; // did not recive player entity
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

void PlayerManager::PlayHurtAudio(entity player)
{
	EntityManager& eMan = EntityManager::Get();
	if (!eMan.HasComponent<PlayerHurtSoundEffectComponent>(player))
	{
		PlayerHurtSoundEffectComponent& playerHurtSoundEffectComponent = eMan.AddComponent<PlayerHurtSoundEffectComponent>(player);

		playerHurtSoundEffectComponent.hurtAudioEntity = DOG::EntityManager::Get().CreateEntity();
		eMan.AddComponent<TransformComponent>(playerHurtSoundEffectComponent.hurtAudioEntity);
		eMan.AddComponent<AudioComponent>(playerHurtSoundEffectComponent.hurtAudioEntity).is3D = true;
		eMan.AddComponent<ChildComponent>(playerHurtSoundEffectComponent.hurtAudioEntity).parent = player;

		m_playerHurtAudio = AssetManager::Get().LoadAudio("Assets/Audio/PlayerHurt/Damage_Fixed.wav");
		m_playerDeathAudio = AssetManager::Get().LoadAudio("Assets/Audio/PlayerHurt/Death_Fixed.wav");
	}
	PlayerHurtSoundEffectComponent& playerHurtSoundEffectComponent = eMan.GetComponent<PlayerHurtSoundEffectComponent>(player);
	AudioComponent& audio = eMan.GetComponent<AudioComponent>(playerHurtSoundEffectComponent.hurtAudioEntity);

	if (eMan.GetComponent<PlayerStatsComponent>(player).health > 0.0f)
	{
		audio.assetID = m_playerHurtAudio;
		audio.volume = 0.7f;
		if (!audio.playing)
			audio.shouldPlay = true;
	}
	else
	{
		audio.assetID = m_playerDeathAudio;
		audio.shouldPlay = true;
		audio.volume = 1.0f;
	}
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

					DOG::gfx::PostProcess::Get().InstantiateDamageDisk({ dir.x, dir.z }, 2.f, 1.5f, { 1.f, 0.135f, 0.f });
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
