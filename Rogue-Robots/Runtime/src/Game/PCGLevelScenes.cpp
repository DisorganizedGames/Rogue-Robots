#include "PCGLevelScenes.h"
#include "PrefabInstantiatorFunctions.h"
#include "PcgLevelLoader.h"
#include "ItemManager/ItemManager.h"
using namespace DOG;
using namespace DirectX;
using namespace DirectX::SimpleMath;

PCGLevelScene::PCGLevelScene(u8 numPlayers, std::function<std::vector<DOG::entity>(const EntityTypes, const DirectX::SimpleMath::Vector3&, u8, f32)> spawnAgents, std::string levelName)
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
	spawnblockPos.y += 5.0f;
	spawnblockPos.z += 2.5f;

	//Spawn players
	std::vector<entity> players = SpawnPlayers(spawnblockPos, m_nrOfPlayers, 5.f);
	AddEntities(players);
	AddEntities(AddFlashlightsToPlayers(players));

	//Spawn enemies and items
	uint32_t enemySpawnRarity = 4u; // Spawns enemies once every X blocks.
	uint32_t itemSpawnModifier = 2u; //Increase this to lower spawnrate of items.
	uint32_t maxEnemiesPerSpawn = 5u; //Max amount of enemies per spawn.

	//Collect all floor blocks.
	uint32_t enemyCounter = 0u;
	uint32_t enemyNrCounter = 0u;
	uint32_t itemCounter = 0u;

	EntityManager::Get().Collect<FloorBlockComponent>().Do([&](entity e, FloorBlockComponent&)
		{
			Vector3 pos = EntityManager::Get().GetComponent<TransformComponent>(e).GetPosition();
			if (enemyCounter % enemySpawnRarity == 0)
			{
				Vector3 diff = pos - spawnblockPos;
				float dist = sqrt(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);
				if (dist > 20.0f)
				{
					//Spawn enemies
					AddEntities(m_spawnAgents(EntityTypes::Scorpio, Vector3(pos.x + 2.5f, pos.y + 5.0f, pos.z + 2.5f), (u8)(enemyNrCounter), 1.0f));
					++enemyNrCounter;
					if (enemyNrCounter > 5)
					{
						enemyNrCounter = 1;
					}
				}
			}
			//Spawn items
			ItemManager::Get().CreateItem(EntityTypes(((u32)itemCounter) % (u32(EntityTypes::Default) * itemSpawnModifier)), Vector3(pos.x + 2.5f, pos.y + 1.0f, pos.z + 2.5f));
			++enemyCounter;
			++itemCounter;
		});
}
