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
	int n = (int)std::ceil(std::cbrt(numPoints));

	for (int i = 0; i < n; i++)
	{
		float y = DOG::Remap(0.0f, (float)(n - 1), minAABB.y, maxAABB.y, (float)i);
		for (int j = 0; j < n; j++)
		{
			float x = DOG::Remap(0.0f, (float)(n - 1), minAABB.x, maxAABB.x, (float)j);
			for (int k = 0; k < n; k++)
			{
				if (points.size() < numPoints)
				{
					float z = DOG::Remap(0.0f, (float)(n - 1), minAABB.z, maxAABB.z, (float)k);
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
	[[maybe_unused]]auto&& Vec3ToStr= [](Vector3 v)
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
			static int gridSize = 4096;
			static float lightRadius = 3.8f;
			ImGui::SliderInt("Grid size", &gridSize, 0, 4096);
			ImGui::SliderFloat("Light raduis", &lightRadius, 0.1f, 10.0f);
			if (ImGui::Button("Spawn Grid"))
			{
				auto points = GetGridPointsInAABB(Vector3(-33, 0, -20), Vector3(33, 25, 22), gridSize);
				for (auto& p : points)
				{
					entity e = CreateEntity();
					AddComponent<TransformComponent>(e, p, Vector3::Zero, 0.2f * Vector3::One);
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
				camTr.SetPosition(Vector3(-27, 1.8f, -2.6f));
				camTr.SetRotation(Vector3(0, XMConvertToRadians(80), 0));
				camCa.viewMatrix = camTr.worldMatrix.Invert();
			}

			ImGui::Separator();

			static double time = 0;
			static double avgDelta = 0;
			static u32 frameCounter = 1;
			static bool profiling = false;
			static std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
			if (ImGui::Button("Start timer"))
			{
				if (!profiling)
				{
					t1 = std::chrono::high_resolution_clock::now();
					frameCounter = Time::FrameCount();
					time = 0;
					profiling = true;
				}
			}

			if (ImGui::Button("Stop timer"))
			{
				if (profiling)
				{
					frameCounter = Time::FrameCount() - frameCounter;
					std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
					std::chrono::duration<double> elapsed_time = t2 - t1;
					time = elapsed_time.count();
					avgDelta = time / frameCounter;
					profiling = false;
				}
			}

			if (profiling)
			{
				std::chrono::duration<double> el = std::chrono::high_resolution_clock::now() - t1;
				ImGui::Text(std::format("Total Time: {:.4f}", el.count()).c_str());
				ImGui::Text(std::format("Frame time: {:.4f}, (current: {:.4f})", el.count() / (Time::FrameCount() - frameCounter), Time::DeltaTime()).c_str());
			}
			else
			{
				ImGui::Text(std::format("Total Time: {:.7f}", time).c_str());
				ImGui::Text(std::format("Frame time: {:.7f}", avgDelta).c_str());
			}
		}

		ImGui::End();
	}
}
