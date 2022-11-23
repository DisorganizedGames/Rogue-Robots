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

		std::vector<entity> players = SpawnPlayers(Vector3(0.0f, 0.0f, -3.f), 1, 0.f);
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
		m_particleSystem = CreateEntity();
		AddComponent<TransformComponent>(m_particleSystem, Vector3(0, 0, 0));
		auto& em = AddComponent<ParticleEmitterComponent>(m_particleSystem);
		em = {
			.spawnRate = 64.f,
			.particleLifetime = 0.5f,
		};

		AddComponent<ConeSpawnComponent>(m_particleSystem) = { 
			.angle = XM_PIDIV4,
			.speed = 10.f,
		};
	}

	DOG::ImGuiMenuLayer::RegisterDebugWindow("ParticleSystemMenu", [this](bool& open) { ParticleSystemMenu(open); });
}

ParticleScene::~ParticleScene()
{
	DOG::ImGuiMenuLayer::UnRegisterDebugWindow("ParticleSystemMenu");
}

void ParticleScene::ParticleSystemMenu(bool& open)
{
	auto& emitter = EntityManager::Get().GetComponent<ParticleEmitterComponent>(m_particleSystem);

	if (ImGui::BeginMenu("View"))
	{
		if (ImGui::MenuItem("Particle System"))
		{
			open = true;
		}
		ImGui::EndMenu(); // "View"
	}

	if (open)
	{
		if (ImGui::Begin("Particle System", &open))
		{
			// Spawn rate setting
			static float rate = emitter.spawnRate;
			ImGui::InputFloat("Rate", &rate);
			emitter.spawnRate = rate;

			// Particle lifetime slider
			static float lifetime = emitter.particleLifetime;
			ImGui::SliderFloat("Particle Lifetime", &lifetime, 0.f, 5.f);
			emitter.particleLifetime = lifetime;

			// Spawn type settings
			static int spawnType = 0;
			bool clicked = false;
			clicked |= ImGui::RadioButton("Default", &spawnType, 0);
			clicked |= ImGui::RadioButton("Cone", &spawnType, 1);
			clicked |= ImGui::RadioButton("Cylinder", &spawnType, 2);
			clicked |= ImGui::RadioButton("Box", &spawnType, 3);
			if (clicked)
			{
				switch (spawnType)
				{
				case 0: 
					SwitchToComponent<nullptr_t>();
					break;
				case 1: 
					SwitchToComponent<ConeSpawnComponent>();
					break;
				case 2: 
					SwitchToComponent<CylinderSpawnComponent>();
					break;
				case 3: 
					SwitchToComponent<BoxSpawnComponent>();
					break;
				}
			}
			switch (spawnType)
			{
			case 1:
				ConeSettings();
				break;
			case 2:
				CylinderSettings();
				break;
			case 3:
				BoxSettings();
				break;
			}

			// Texture Settings
			static bool enableTexture = false;

			static char buf[256] = "Assets/Models/Rifle/textures/Base_baseColor.png";
			static std::string currentTexture(buf);

			ImGui::Checkbox("Use texture", &enableTexture);
			if (!enableTexture)
			{
				emitter.textureHandle = 0;
				currentTexture = "";
			}
			else
			{
				ImGui::InputText("Texture path", buf, 256);
				std::string newTexture(buf);
				if (std::filesystem::is_regular_file(newTexture))
				{
					if (newTexture != currentTexture)
					{
						auto textureAsset = AssetManager::Get().LoadTexture(newTexture, AssetLoadFlag::GPUMemory);
						emitter.textureHandle = AssetManager::Get().GetAsset<TextureAsset>(textureAsset)->textureViewRawHandle;
						currentTexture = newTexture;
					}
				}

				static int x = 1, y = 1;
				ImGui::SetNextItemWidth(80.f);
				ImGui::InputInt("x", &x);
				ImGui::SameLine();
				ImGui::SetNextItemWidth(80.f);
				ImGui::InputInt("y", &y);

				x = std::max(1, x);
				y = std::max(1, y);

				emitter.textureSegmentsX = x;
				emitter.textureSegmentsY = y;
			}
		}

		ImGui::End();
	}

}

void ParticleScene::ConeSettings()
{
	auto& cone = EntityManager::Get().GetComponent<ConeSpawnComponent>(m_particleSystem);
	ImGui::SliderFloat("Angle", &cone.angle, 0.f, DirectX::XM_PIDIV2 - std::numeric_limits<float>::epsilon());
	ImGui::InputFloat("Speed", &cone.speed);
}

void ParticleScene::CylinderSettings()
{
	auto& cylinder = EntityManager::Get().GetComponent<CylinderSpawnComponent>(m_particleSystem);
	ImGui::InputFloat("Radius", &cylinder.radius);
	ImGui::InputFloat("Height", &cylinder.height);
}

void ParticleScene::BoxSettings()
{
	auto& box = EntityManager::Get().GetComponent<BoxSpawnComponent>(m_particleSystem);
	ImGui::InputFloat("X", &box.x);
	ImGui::InputFloat("Y", &box.y);
	ImGui::InputFloat("Z", &box.z);
}

