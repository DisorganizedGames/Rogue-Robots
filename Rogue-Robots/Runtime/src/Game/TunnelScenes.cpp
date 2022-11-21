#include "TunnelScenes.h"
#include "PrefabInstantiatorFunctions.h"
#include "PcgLevelLoader.h"
#include "ItemManager/ItemManager.h"
using namespace DOG;
using namespace DirectX;
using namespace DirectX::SimpleMath;

// Room 0
TunnelRoom0Scene::TunnelRoom0Scene(u8 numPlayers, std::function<std::vector<DOG::entity>(const EntityTypes, const DirectX::SimpleMath::Vector3&, u8, f32)> spawnAgents)
	: Scene(SceneComponent::Type::TunnelRoom0Scene), m_spawnAgents(spawnAgents), m_nrOfPlayers(numPlayers)
{

}

void TunnelRoom0Scene::SetUpScene(std::vector<std::function<std::vector<DOG::entity>()>> entityCreators)
{
	for (auto& func : entityCreators)
		AddEntities(func());

	// room 0: small room - maybe nice entry point?
	std::vector<entity> players = SpawnPlayers(Vector3(38.0f, 90.0f, 12.0f), m_nrOfPlayers, 10.f);
	AddEntities(players);
	AddEntities(AddFlashlightsToPlayers(players));

	AddEntities(LoadLevel(pcgLevelNames::tunnels));
	
	// room 1: a few rooms connected by tunnels
	AddEntities(m_spawnAgents(EntityTypes::Scorpio, Vector3(40.f, 80.f, 58.f), 3, 2.5f));	// location 2
	AddEntities(m_spawnAgents(EntityTypes::Scorpio, Vector3(27.f, 78.f, 68.f), 4, 5.f));	// location 3
	AddEntities(m_spawnAgents(EntityTypes::Scorpio, Vector3(8.f, 80.f, 37.f), 7, 3.f));		// location 4

	// room 2: a larger, more open room
	AddEntities(m_spawnAgents(EntityTypes::Scorpio, Vector3(63.f, 80.f, 78.f), 4, 2.5f));	// location 2
	AddEntities(m_spawnAgents(EntityTypes::Scorpio, Vector3(65.f, 80.f, 104.f), 5, 4.f));	// location 3
	AddEntities(m_spawnAgents(EntityTypes::Scorpio, Vector3(65.f, 80.f, 124.f), 4, 1.5f));	// location 4
	AddEntities(m_spawnAgents(EntityTypes::Scorpio, Vector3(24.f, 80.f, 124.f), 2, 1.f));	// location 5

	// room 3: huge cave system
	AddEntities(m_spawnAgents(EntityTypes::Scorpio, Vector3(41.f, 55.f, 90.f), 8, 5.f));	// location 2
	AddEntities(m_spawnAgents(EntityTypes::Scorpio, Vector3(48.f, 58.f, 135.f), 15, 7.5f));	// location 3
	AddEntities(m_spawnAgents(EntityTypes::Scorpio, Vector3(5.5f, 55.f, 80.f), 10, 2.5f));	// location 4
	AddEntities(m_spawnAgents(EntityTypes::Scorpio, Vector3(5.f, 53.f, 104.5f), 2, .5f));	// location 5
	AddEntities(m_spawnAgents(EntityTypes::Scorpio, Vector3(5.f, 53.f, 109.5f), 2, .5f));	// location 6
	AddEntities(m_spawnAgents(EntityTypes::Scorpio, Vector3(5.f, 53.f, 104.5f), 2, .5f));	// location 7
	AddEntities(m_spawnAgents(EntityTypes::Scorpio, Vector3(5.f, 53.f, 119.f), 2, .5f));	// location 8
}



// Room 1
TunnelRoom1Scene::TunnelRoom1Scene(u8 numPlayers, std::function<std::vector<DOG::entity>(const EntityTypes, const DirectX::SimpleMath::Vector3&, u8, f32)> spawnAgents)
	: Scene(SceneComponent::Type::TunnelRoom1Scene), m_spawnAgents(spawnAgents), m_nrOfPlayers(numPlayers)
{

}

void TunnelRoom1Scene::SetUpScene(std::vector<std::function<std::vector<DOG::entity>()>> entityCreators)
{
	for (auto& func : entityCreators)
		AddEntities(func());


	// room 1: a few rooms connected by tunnels
	std::vector<entity> players = SpawnPlayers(Vector3(13.0f, 80.0f, 2.0f), m_nrOfPlayers, 3.f);	// location 1
	AddEntities(players);
	AddEntities(AddFlashlightsToPlayers(players));

	AddEntities(LoadLevel(pcgLevelNames::tunnels));

	AddEntities(m_spawnAgents(EntityTypes::Scorpio, Vector3(40.f, 80.f, 58.f), 3, 2.5f));	// location 2
	AddEntities(m_spawnAgents(EntityTypes::Scorpio, Vector3(27.f, 78.f, 68.f), 4, 5.f));	// location 3
	AddEntities(m_spawnAgents(EntityTypes::Scorpio, Vector3(8.f, 80.f, 37.f), 7, 3.f));		// location 4
}



// Room 2
TunnelRoom2Scene::TunnelRoom2Scene(u8 numPlayers, std::function<std::vector<DOG::entity>(const EntityTypes, const DirectX::SimpleMath::Vector3&, u8, f32)> spawnAgents)
	: Scene(SceneComponent::Type::TunnelRoom2Scene), m_spawnAgents(spawnAgents), m_nrOfPlayers(numPlayers)
{

}

void TunnelRoom2Scene::SetUpScene(std::vector<std::function<std::vector<DOG::entity>()>> entityCreators)
{
	for (auto& func : entityCreators)
		AddEntities(func());

	
	// room 2: a larger, more open room
	std::vector<entity> players = SpawnPlayers(Vector3(31.0f, 80.0f, 106.0f), m_nrOfPlayers, 5.0f);	// location 1
	AddEntities(players);
	AddEntities(AddFlashlightsToPlayers(players));

	AddEntities(LoadLevel(pcgLevelNames::tunnels)); //Change this to testRooms to try the room connections.

	AddEntities(m_spawnAgents(EntityTypes::Scorpio, Vector3(57.f, 75.f, 78.f) * 5.f / 4.6f, 4, 2.f));	// location 2
	AddEntities(m_spawnAgents(EntityTypes::Scorpio, Vector3(60.f, 75.f, 98.f) * 5.f / 4.6f, 5, 3.f));	// location 3
	AddEntities(m_spawnAgents(EntityTypes::Scorpio, Vector3(34.f, 75.f, 85.f) * 5.f / 4.6f, 4, 1.5f));	// location 4
	AddEntities(m_spawnAgents(EntityTypes::Scorpio, Vector3(27.f, 75.f, 130.f) * 5.f / 4.6f, 2, 1.f));	// location 5

	ItemManager& iM = ItemManager::Get();
	iM.CreateItem(EntityTypes::GrenadeBarrel, Vector3(46.0f, 69.7f, 114.0f) * 5.f / 4.6f);
	iM.CreateItem(EntityTypes::FrostMagazineModification, Vector3(43.0f, 69.7f, 114.0f) * 5.f / 4.6f);
	iM.CreateItem(EntityTypes::MissileBarrel, Vector3(40.0f, 70.0f, 110.0f) * 5.f / 4.6f);
	iM.CreateItem(EntityTypes::Trampoline, Vector3(49.0f, 70.0f, 110.0f) * 5.f / 4.6f);
	iM.CreateItem(EntityTypes::Turret, Vector3(45.0f, 70.0f, 110.0f) * 5.f / 4.6f);
	iM.CreateItem(EntityTypes::Turret, Vector3(45.0f, 70.0f, 107.0f) * 5.f / 4.6f);
	iM.CreateItem(EntityTypes::IncreaseMaxHp, Vector3(49.0f, 70.0f, 115.0f) * 5.f / 4.6f);
	iM.CreateItem(EntityTypes::IncreaseSpeed, Vector3(40.0f, 70.0f, 115.0f) * 5.f / 4.6f);
	iM.CreateItem(EntityTypes::IncreaseSpeed2, Vector3(35.0f, 70.0f, 115.0f) * 5.f / 4.6f);
	//iM.CreateItem(EntityTypes::Health, Vector3(30.0f, 70.0f, 110.0f) * 5.f / 4.6f); todo replace in future
}


// Room 3
TunnelRoom3Scene::TunnelRoom3Scene(u8 numPlayers, std::function<std::vector<DOG::entity>(const EntityTypes, const DirectX::SimpleMath::Vector3&, u8, f32)> spawnAgents)
	: Scene(SceneComponent::Type::TunnelRoom3Scene), m_spawnAgents(spawnAgents), m_nrOfPlayers(numPlayers)
{

}

void TunnelRoom3Scene::SetUpScene(std::vector<std::function<std::vector<DOG::entity>()>> entityCreators)
{
	for (auto& func : entityCreators)
		AddEntities(func());

	// room 3: huge cave system
	std::vector<entity> players = SpawnPlayers(Vector3(68.0f, 56.0f, 76.5f), m_nrOfPlayers, 2.8f); // locaton 1
	AddEntities(players);
	AddEntities(AddFlashlightsToPlayers(players));

	AddEntities(LoadLevel(pcgLevelNames::tunnels));

	// room 3: huge cave system
	AddEntities(m_spawnAgents(EntityTypes::Scorpio, Vector3(41.f, 55.f, 90.f), 8, 5.f));	// location 2
	AddEntities(m_spawnAgents(EntityTypes::Scorpio, Vector3(48.f, 58.f, 135.f), 15, 7.5f));	// location 3
	AddEntities(m_spawnAgents(EntityTypes::Scorpio, Vector3(5.5f, 55.f, 80.f), 10, 2.5f));	// location 4
	AddEntities(m_spawnAgents(EntityTypes::Scorpio, Vector3(5.f, 53.f, 104.5f), 2, .5f));	// location 5
	AddEntities(m_spawnAgents(EntityTypes::Scorpio, Vector3(5.f, 53.f, 109.5f), 2, .5f));	// location 6
	AddEntities(m_spawnAgents(EntityTypes::Scorpio, Vector3(5.f, 53.f, 104.5f), 2, .5f));	// location 7
	AddEntities(m_spawnAgents(EntityTypes::Scorpio, Vector3(5.f, 53.f, 119.f), 2, .5f));	// location 8
}


