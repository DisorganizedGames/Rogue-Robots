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
		AddComponent<TransformComponent>(m_particleSystem, Vector3(0, -1.f, 0));
		auto& em = AddComponent<ParticleEmitterComponent>(m_particleSystem);
		em = {
			.spawnRate = 64.f,
			.particleLifetime = 0.5f,
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
			static float rate = emitter.spawnRate;
			ImGui::SliderFloat("Rate", &rate, 0.f, 1024.f);
			emitter.spawnRate = rate;

			static float lifetime = emitter.particleLifetime;
			ImGui::SliderFloat("Particle Lifetime", &lifetime, 0.f, 5.f);
			emitter.particleLifetime = lifetime;

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
				std::string newTexture = "";
				newTexture += buf;
				auto test = std::filesystem::absolute(newTexture);
				if (std::filesystem::is_regular_file(newTexture))
				{
					if (newTexture != currentTexture)
					{
						auto textureAsset = AssetManager::Get().LoadTexture(newTexture, AssetLoadFlag::GPUMemory);
						emitter.textureHandle = AssetManager::Get().GetAsset<TextureAsset>(textureAsset)->textureViewRawHandle;
						currentTexture = newTexture;
					}
				}
			}
		}

		ImGui::End();
	}

}

