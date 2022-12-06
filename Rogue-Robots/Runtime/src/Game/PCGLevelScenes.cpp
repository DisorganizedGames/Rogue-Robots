#include "PCGLevelScenes.h"
#include "PrefabInstantiatorFunctions.h"
#include "PcgLevelLoader.h"
#include "ItemManager/ItemManager.h"
#include "Pathfinder/Pathfinder.h"
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

	// Prepare Pathfinder
	Pathfinder::Get().BuildNavScene(m_sceneType);


	//Identify spawnblock.
	m_spawnblockPos = Vector3(20.0f, 20.0f, 20.0f);
	EntityManager::Get().Collect<SpawnBlockComponent>().Do([&](entity e, SpawnBlockComponent&)
		{
			m_spawnblockPos = EntityManager::Get().GetComponent<TransformComponent>(e).GetPosition();
		});
	m_spawnblockPos.x += 2.5f;
	m_spawnblockPos.y += 7.0f;
	m_spawnblockPos.z += 2.5f;

	//Spawn players
	std::vector<entity> players = SpawnPlayers(m_spawnblockPos, m_nrOfPlayers, 5.f);
	AddEntities(players);
	AddEntities(AddFlashlightsToPlayers(players));
	AddEntities(AddGunsToPlayers(players));

	//Spawn enemies and items
	uint32_t enemySpawnRarity = static_cast<uint32_t>(std::ceil(5.0f / m_nrOfPlayers)); //Spawns enemies once every X blocks.
	uint32_t maxEnemiesPerSpawn = 3u; //Max amount of enemies per spawn.
	uint32_t itemSpawnModifier = static_cast<uint32_t>(std::ceil(4.0f / m_nrOfPlayers)); //Increase this to lower spawnrate of items.

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
				Vector3 diff = pos - m_spawnblockPos;
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
			ItemManager::Get().CreateItem(EntityTypes::Reviver, Vector3(pos.x, pos.y + 1.0f, pos.z));
			++enemyCounter;
			++itemCounter;
		});



	auto&& createAmbient = [&](const std::string& fileName, float startOffset, float mean, float stdDiv, float volume) {
		entity e = CreateEntity();
		auto& audio = AddComponent<AudioComponent>(e);
		audio.assetID = AssetManager::Get().LoadAudio("Assets/Audio/Ambient/" + fileName);
		audio.volume = volume;

		auto& ambientSound = AddComponent<AmbientSoundComponent>(e);
		ambientSound.singleTimeStartOffsetTime = startOffset;
		ambientSound.meanRepeatTime = mean;
		ambientSound.stdDiv = stdDiv;
	};

	createAmbient("CreepyAmbience.wav", 60, 65, 30, 0.6f);
	createAmbient("CreepyAmbience2.wav", 10, 63, 40, 0.6f);
	createAmbient("CreepyAmbience3.wav", 0, 70, 20, 0.4f);
	createAmbient("CreepyAmbience4.wav", 50, 80, 30, 0.7f);
	createAmbient("AmbienceLong.wav", -119, 120, 1, 0.5f);
	createAmbient("AmbienceLong3.wav", 120, 120, 1, 0.5f);
}

const DirectX::SimpleMath::Vector3& PCGLevelScene::GetSpawnblock()
{
	return m_spawnblockPos;
}