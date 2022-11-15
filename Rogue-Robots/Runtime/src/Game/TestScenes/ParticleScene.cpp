#include "ParticleScene.h"
#include "../PrefabInstantiatorFunctions.h"

using namespace DOG;
using namespace DirectX;
using namespace SimpleMath;

ParticleScene::ParticleScene() : Scene(SceneComponent::Type::ParticleScene)
{

}

void ParticleScene::SetUpScene(std::vector<std::function<std::vector<DOG::entity>()>> entityCreators)
{
	// Necessary set up :(
	{
		u32 blueCubeID = AssetManager::Get().LoadModelAsset("Assets/Models/Temporary_Assets/blue_cube.glb");

		std::vector<entity> players = SpawnPlayers(Vector3(0.0f, 0.0f, 0.0f), 1, 0.f);
		AddEntities(players);

		entity ground = CreateEntity();
		AddComponent<TransformComponent>(ground, Vector3(0, -2.f, 0), Vector3::Zero, Vector3(5, 1, 5));
		AddComponent<BoxColliderComponent>(ground, ground, Vector3(5.f, 1.f, 5.f), false);
		AddComponent<ModelComponent>(ground, blueCubeID);

		// A hack to not instantly win the game
		entity enemy = CreateEntity();
		AddComponent<AgentIdComponent>(enemy);
	}

	//Particle system
	{
		entity ps = CreateEntity();
		AddComponent<TransformComponent>(ps, Vector3(0, -1.f, 0));
		auto& em = AddComponent<ParticleEmitterComponent>(ps);
		em = {
			.spawnRate = 8.f,
			.particleLifetime = 0.5f,
		};
	}
}

