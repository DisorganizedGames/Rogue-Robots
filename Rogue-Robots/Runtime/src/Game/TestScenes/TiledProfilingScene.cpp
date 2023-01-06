#include "TiledProfilingScene.h"
#include "../PrefabInstantiatorFunctions.h"

using namespace DOG;
using namespace DirectX;
using namespace SimpleMath;

TiledProfilingScene::TiledProfilingScene() : Scene(SceneComponent::Type::TiledProfilingScene)
{

}

void TiledProfilingScene::SetUpScene(std::vector<std::function<std::vector<DOG::entity>()>> entityCreators)
{


	// Necessary set up :(
	{
		u32 blueCubeID = AssetManager::Get().LoadModelAsset("Assets/Models/Temporary_Assets/blue_cube.glb");

		std::vector<entity> players = SpawnPlayers(Vector3(0.0f, 0.0f, -3.f), 1, 0.f);
		AddEntities(players);
		AddEntities(AddFlashlightsToPlayers(players));
		entity ground = CreateEntity();
		AddComponent<TransformComponent>(ground, Vector3(0, -2.f, 0), Vector3::Zero, Vector3(5, 1, 5));
		AddComponent<BoxColliderComponent>(ground, ground, Vector3(5.f, 1.f, 5.f), false);
		AddComponent<ModelComponent>(ground, blueCubeID);


		u32 sponzaID = AssetManager::Get().LoadModelAsset("Assets/Models/Temporary_Assets/Sponza_gltf/glTF/Sponza.gltf");

		entity sponza = CreateEntity();
		AddComponent<TransformComponent>(sponza, Vector3(0, -2.f, 0), Vector3::Zero, Vector3(0.02f, 0.02f, 0.02f));
		AddComponent<ModelComponent>(sponza, sponzaID);

		// A hack to not instantly win the game
		entity enemy = CreateEntity();
		AddComponent<AgentIdComponent>(enemy);
	}


	DOG::ImGuiMenuLayer::RegisterDebugWindow("TiledProfilingMenu", [this](bool& open) { TiledProfilingMenu(open); });
}

TiledProfilingScene::~TiledProfilingScene()
{
	DOG::ImGuiMenuLayer::UnRegisterDebugWindow("TiledProfilingMenu");
}

void TiledProfilingScene::TiledProfilingMenu(bool& open)
{
	if (ImGui::BeginMenu("View"))
	{
		if (ImGui::MenuItem("Tiled Profiling"))
		{
			open = true;
		}
		ImGui::EndMenu(); // "View"
	}

	if (open)
	{
		if (ImGui::Begin("Tiled Profiling", &open))
		{
			
		}

		ImGui::End();
	}
}
