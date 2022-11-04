#include "TunnelScenes.h"
#include "PrefabInstantiatorFunctions.h"
#include "PcgLevelLoader.h"

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
	std::vector<entity> players = SpawnPlayers(Vector3(12.0f, 90.0f, 38.0f), m_nrOfPlayers, 10.f);
	AddEntities(players);
	AddEntities(AddFlashlightsToPlayers(players));

	AddEntities(LoadLevel(pcgLevelNames::tunnels));
	
	// room 1: a few rooms connected by tunnels
	AddEntities(m_spawnAgents(EntityTypes::Scorpio, Vector3(58.f, 80.f, 40.f), 3, 2.5f));	// location 2
	AddEntities(m_spawnAgents(EntityTypes::Scorpio, Vector3(68.f, 78.f, 27.f), 4, 5.f));	// location 3
	AddEntities(m_spawnAgents(EntityTypes::Scorpio, Vector3(37.f, 80.f, 8.f), 7, 3.f));		// location 4

	// room 2: a larger, more open room
	AddEntities(m_spawnAgents(EntityTypes::Scorpio, Vector3(78.f, 80.f, 63.f), 4, 2.5f));	// location 2
	AddEntities(m_spawnAgents(EntityTypes::Scorpio, Vector3(104.f, 80.f, 65.f), 5, 4.f));	// location 3
	AddEntities(m_spawnAgents(EntityTypes::Scorpio, Vector3(124.f, 80.f, 65.f), 4, 1.5f));	// location 4
	AddEntities(m_spawnAgents(EntityTypes::Scorpio, Vector3(124.f, 80.f, 24.f), 2, 1.f));	// location 5

	// room 3: huge cave system
	AddEntities(m_spawnAgents(EntityTypes::Scorpio, Vector3(90.f, 55.f, 41.f), 8, 5.f));	// location 2
	AddEntities(m_spawnAgents(EntityTypes::Scorpio, Vector3(135.f, 58.f, 48.f), 15, 7.5f));	// location 3
	AddEntities(m_spawnAgents(EntityTypes::Scorpio, Vector3(80.f, 55.f, 5.5f), 10, 2.5f));	// location 4
	AddEntities(m_spawnAgents(EntityTypes::Scorpio, Vector3(104.5f, 53.f, 5.f), 2, .5f));	// location 5
	AddEntities(m_spawnAgents(EntityTypes::Scorpio, Vector3(109.5f, 53.f, 5.f), 2, .5f));	// location 6
	AddEntities(m_spawnAgents(EntityTypes::Scorpio, Vector3(104.5f, 53.f, 5.f), 2, .5f));	// location 7
	AddEntities(m_spawnAgents(EntityTypes::Scorpio, Vector3(119.f, 53.f, 5.f), 2, .5f));	// location 8
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
	std::vector<entity> players = SpawnPlayers(Vector3(2.0f, 80.0f, 13.0f), m_nrOfPlayers, 3.f);	// location 1
	AddEntities(players);
	AddEntities(AddFlashlightsToPlayers(players));

	AddEntities(LoadLevel(pcgLevelNames::tunnels));

	AddEntities(m_spawnAgents(EntityTypes::Scorpio, Vector3(58.f, 80.f, 40.f), 3, 2.5f));	// location 2
	AddEntities(m_spawnAgents(EntityTypes::Scorpio, Vector3(68.f, 78.f, 27.f), 4, 5.f));	// location 3
	AddEntities(m_spawnAgents(EntityTypes::Scorpio, Vector3(37.f, 80.f, 8.f), 7, 3.f));		// location 4
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
	std::vector<entity> players = SpawnPlayers(Vector3(106.0f, 80.0f, 31.0f), m_nrOfPlayers, 5.0f);	// location 1
	AddEntities(players);
	AddEntities(AddFlashlightsToPlayers(players));

	AddEntities(LoadLevel(pcgLevelNames::tunnels));

	AddEntities(m_spawnAgents(EntityTypes::Scorpio, Vector3(78.f, 80.f, 63.f), 4, 2.5f));	// location 2
	AddEntities(m_spawnAgents(EntityTypes::Scorpio, Vector3(104.f, 80.f, 65.f), 5, 4.f));	// location 3
	AddEntities(m_spawnAgents(EntityTypes::Scorpio, Vector3(124.f, 80.f, 65.f), 4, 1.5f));	// location 4
	AddEntities(m_spawnAgents(EntityTypes::Scorpio, Vector3(124.f, 80.f, 24.f), 2, 1.f));	// location 5
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
	std::vector<entity> players = SpawnPlayers(Vector3(76.5f, 56.0f, 68.0f), m_nrOfPlayers, 2.8f); // locaton 1
	AddEntities(players);
	AddEntities(AddFlashlightsToPlayers(players));

	AddEntities(LoadLevel(pcgLevelNames::tunnels));

	// room 3: huge cave system
	AddEntities(m_spawnAgents(EntityTypes::Scorpio, Vector3(90.f, 55.f, 41.f), 8, 5.f));	// location 2
	AddEntities(m_spawnAgents(EntityTypes::Scorpio, Vector3(135.f, 58.f, 48.f), 15, 7.5f));	// location 3
	AddEntities(m_spawnAgents(EntityTypes::Scorpio, Vector3(80.f, 55.f, 5.5f), 10, 2.5f));	// location 4
	AddEntities(m_spawnAgents(EntityTypes::Scorpio, Vector3(104.5f, 53.f, 5.f), 2, .5f));	// location 5
	AddEntities(m_spawnAgents(EntityTypes::Scorpio, Vector3(109.5f, 53.f, 5.f), 2, .5f));	// location 6
	AddEntities(m_spawnAgents(EntityTypes::Scorpio, Vector3(104.5f, 53.f, 5.f), 2, .5f));	// location 7
	AddEntities(m_spawnAgents(EntityTypes::Scorpio, Vector3(119.f, 53.f, 5.f), 2, .5f));	// location 8
}
