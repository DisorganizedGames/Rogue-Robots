#include "TiledProfilingScene.h"
#include "../PrefabInstantiatorFunctions.h"
#include "../AgentManager/AgentManager.h"

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
		AddComponent<TransformComponent>(sponza, Vector3(0, -0.9f, 0), Vector3::Zero, Vector3(0.02f, 0.02f, 0.02f));
		AddComponent<ModelComponent>(sponza, sponzaID);

		// A hack to not instantly win the game
		entity enemy = CreateEntity();
		AddComponent<AgentIdComponent>(enemy);
	}


	DOG::ImGuiMenuLayer::RegisterDebugWindow("TiledProfilingMenu", [this](bool& open) { TiledProfilingMenu(open); }, true);
}

TiledProfilingScene::~TiledProfilingScene()
{
	DOG::ImGuiMenuLayer::UnRegisterDebugWindow("TiledProfilingMenu");
}

std::vector<Vector3> GetGridPointsInAABB(Vector3 minAABB, Vector3 maxAABB, int numPoints)
{
	std::vector<Vector3> points;
	int n = std::ceil(std::cbrt(numPoints));

	for (int i = 0; i < n; i++)
	{
		float y = DOG::Remap(0, n - 1, minAABB.y, maxAABB.y, i);
		for (int j = 0; j < n; j++)
		{
			float x = DOG::Remap(0, n - 1, minAABB.x, maxAABB.x, j);
			for (int k = 0; k < n; k++)
			{
				if (points.size() < numPoints)
				{
					float z = DOG::Remap(0, n - 1, minAABB.z, maxAABB.z, k);
					points.emplace_back(x, y, z);
				}
				else
				{
					break;
				}
			}
		}
	}

	return points;
}



struct GridLight
{

};

void TiledProfilingScene::TiledProfilingMenu(bool& open)
{
	auto&& Vec3ToStr= [](Vector3 v)
	{
		std::string str;
		str += std::format("{:.1f}", v.x) + ", ";
		str += std::format("{:.1f}", v.y) + ", ";
		str += std::format("{:.1f}", v.z);
		return str;
	};



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
		auto& em = EntityManager::Get();

		if (ImGui::Begin("Tiled Profiling", &open, ImGuiWindowFlags_NoFocusOnAppearing))
		{

			entity camera = GetCamera();
			auto& camTr = em.GetComponent<TransformComponent>(camera);
			ImGui::Text(std::format("camera pos: {}", Vec3ToStr(camTr.GetPosition())).c_str());

			static int gridSize = 4096;
			static float lightRadius = 3.8f;
			ImGui::SliderInt("Grid size", &gridSize, 0, 4096);
			ImGui::SliderFloat("Light raduis", &lightRadius, 0.1f, 10.0f);
			if (ImGui::Button("Spawn Grid"))
			{
				u32 modelID = AssetManager::Get().LoadModelAsset("Assets/Models/Temporary_Assets/red_cube.glb");

				auto points = GetGridPointsInAABB(Vector3(-33, 0, -20), Vector3(33, 25, 22), gridSize);
				for (auto& p : points)
				{
					entity e = CreateEntity();
					AddComponent<TransformComponent>(e, p, Vector3::Zero, 0.2f * Vector3::One);
					//AddComponent<ModelComponent>(e, modelID);
					AddComponent<GridLight>(e);

					auto& pl = AddComponent<PointLightComponent>(e);
					pl.color = AgentManager::GenerateRandomVector3(0, 2.0f, 0.333f);
					pl.radius = lightRadius;
					pl.strength = 1.0f;
					PointLightDesc plDesc;
					plDesc.color = pl.color;
					plDesc.strength = pl.strength;
					plDesc.position = p;
					plDesc.radius = pl.radius;
					pl.handle = LightManager::Get().AddPointLight(plDesc, LightUpdateFrequency::PerFrame);
				}

			}
			if (ImGui::Button("Remove Grid"))
			{
				EntityManager::Get().Collect<GridLight>().Do([&](entity e, GridLight&)
					{
						em.DeferredEntityDestruction(e);
					});
			}

			if (ImGui::Button("Set camera"))
			{
				entity camera = GetCamera();
				auto& camTr = em.GetComponent<TransformComponent>(camera);
				auto& camCa = em.GetComponent<CameraComponent>(camera);
				camTr.SetPosition(Vector3(-27, 1.8, -2.6));
				camTr.SetRotation(Vector3(0, XMConvertToRadians(80), 0));
				camCa.viewMatrix = camTr.worldMatrix.Invert();
			}
		}

		ImGui::End();
	}
}
