#include "OldDefaultScene.h"
#include "PrefabInstantiatorFunctions.h"
#include "PcgLevelLoader.h"
#include "TurretSystems.h"

using namespace DOG;
using namespace DirectX;
using namespace DirectX::SimpleMath;

OldDefaultScene::OldDefaultScene(u8 numPlayers, std::function<std::vector<DOG::entity>(const EntityTypes, SceneComponent::Type scene, const DirectX::SimpleMath::Vector3&, u8, f32)> spawnAgents)
	: Scene(SceneComponent::Type::OldDefaultScene), m_spawnAgents(spawnAgents), m_nrOfPlayers(numPlayers)
{
	
}

void OldDefaultScene::SetUpScene(std::vector<std::function<std::vector<DOG::entity>()>> entityCreators)
{
	for (auto& func : entityCreators)
		AddEntities(func());


	std::vector<entity> players = SpawnPlayers(Vector3(25.0f, 15.0f, 25.0f), m_nrOfPlayers, 10.f);
	AddEntities(players);
	AddEntities(AddFlashlightsToPlayers(players));

	AddEntities(LoadLevel(pcgLevelNames::oldDefault));

	m_spawnAgents(EntityTypes::Scorpio, m_sceneType, Vector3(20, 20, 50), 10, 3.0f);
	m_spawnAgents(EntityTypes::Scorpio, m_sceneType, Vector3(30, 20, 50), 10, 3.0f);
	m_spawnAgents(EntityTypes::Scorpio, m_sceneType, Vector3(40, 20, 50), 10, 3.0f);

	entity turretBase = CreateEntity();
	AddComponent<TransformComponent>(turretBase, Vector3(45, 5.5f, 20));
	AddComponent<ModelComponent>(turretBase).id = AssetManager::Get().LoadModelAsset("Assets/Models/Temporary_Assets/turretBase.glb");

	entity turretHead = CreateEntity();
	AddComponent<TransformComponent>(turretHead);
	auto& tr = AddComponent<ChildComponent>(turretHead);
	tr.parent = turretBase;
	tr.localTransform.SetPosition({ 0, 1, 0 });
	AddComponent<ModelComponent>(turretHead).id = AssetManager::Get().LoadModelAsset("Assets/Models/Temporary_Assets/turret2.glb");
	AddComponent<TurretTargetingComponent>(turretHead);
	AddComponent<TurretBasicShootingComponent>(turretHead).owningPlayer = GetPlayer();
}
