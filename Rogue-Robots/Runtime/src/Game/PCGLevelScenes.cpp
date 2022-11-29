#include "PCGLevelScenes.h"
#include "PrefabInstantiatorFunctions.h"
#include "PcgLevelLoader.h"
#include "ItemManager/ItemManager.h"
using namespace DOG;
using namespace DirectX;
using namespace DirectX::SimpleMath;

PCGLevelScene::PCGLevelScene(u8 numPlayers, std::function<std::vector<DOG::entity>(const EntityTypes, SceneComponent::Type scene, const DirectX::SimpleMath::Vector3&, u8, f32)> spawnAgents, std::string levelName)
	: Scene(SceneComponent::Type::PCGLevelScene), m_spawnAgents(spawnAgents), m_nrOfPlayers(numPlayers), m_levelName(levelName)
{

}

void PCGLevelScene::SetUpScene(std::vector<std::function<std::vector<DOG::entity>()>> entityCreators)
{
	for (auto& func : entityCreators)
		AddEntities(func());

	AddEntities(LoadLevel(m_levelName));

	//Identify spawnblock.
	Vector3 spawnblockPos = Vector3(20.0f, 20.0f, 20.0f);
	EntityManager::Get().Collect<SpawnBlockComponent>().Do([&](entity e, SpawnBlockComponent&)
		{
			spawnblockPos = EntityManager::Get().GetComponent<TransformComponent>(e).GetPosition();
		});
	spawnblockPos.x += 2.5f;
	spawnblockPos.y += 11.0f;
	spawnblockPos.z += 2.5f;

	//Spawn players
	std::vector<entity> players = SpawnPlayers(spawnblockPos, m_nrOfPlayers, 5.f);
	AddEntities(players);
	AddEntities(AddFlashlightsToPlayers(players));

	//Spawn enemies and items
	uint32_t enemySpawnRarity = 4u; //Spawns enemies once every X blocks.
	uint32_t itemSpawnModifier = 1u; //Increase this to lower spawnrate of items.
	uint32_t maxEnemiesPerSpawn = 5u; //Max amount of enemies per spawn.

	//Collect all floor blocks.
	uint32_t enemyCounter = 0u;
	uint32_t enemyNrCounter = 0u;
	uint32_t itemCounter = 0u;

	//Enemies can not spawn closer than this to the players.
	float safeZone = 20.0f;

	EntityManager::Get().Collect<FloorBlockComponent>().Do([&](entity e, FloorBlockComponent&)
		{
			Vector3 pos = EntityManager::Get().GetComponent<TransformComponent>(e).GetPosition();
			if (enemyCounter % enemySpawnRarity == 0)
			{
				Vector3 diff = pos - spawnblockPos;
				float dist = sqrt(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);
				if (dist > safeZone)
				{
					//Spawn enemies
					m_spawnAgents(EntityTypes::Scorpio, m_sceneType, Vector3(pos.x, pos.y + 2.5f, pos.z), (u8)(enemyNrCounter), 0.5f);
					++enemyNrCounter;
					if (enemyNrCounter > maxEnemiesPerSpawn)
					{
						enemyNrCounter = 1;
					}
				}
			}
			//Spawn items
			ItemManager::Get().CreateItem(EntityTypes(((u32)itemCounter) % (u32(EntityTypes::Default) * itemSpawnModifier)), Vector3(pos.x, pos.y + 1.0f, pos.z));
			++enemyCounter;
			++itemCounter;
		});
}
