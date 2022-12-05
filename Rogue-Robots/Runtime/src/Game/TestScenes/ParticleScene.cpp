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
	SpawnParticleSystem();

	DOG::ImGuiMenuLayer::RegisterDebugWindow("ParticleSystemMenu", [this](bool& open) { ParticleSystemMenu(open); });
}

ParticleScene::~ParticleScene()
{
	DOG::ImGuiMenuLayer::UnRegisterDebugWindow("ParticleSystemMenu");
}

void ParticleScene::ParticleSystemMenu(bool& open)
{
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
			static bool toggled = true;
			if (ImGui::Button("Toggle System"))
			{
				toggled = !toggled;
				if (toggled)
				{
					SpawnParticleSystem();
				}
				else
				{
					AddComponent<DeferredDeletionComponent>(m_particleSystem);
				}
			}
			ImGui::SameLine();
			ImGui::Text(std::to_string(toggled).c_str());

			if (!toggled)
			{
				ImGui::End();
				return;
			}
			auto& emitter = EntityManager::Get().GetComponent<ParticleEmitterComponent>(m_particleSystem);


			// Spawn rate setting
			static float rate = emitter.spawnRate;
			ImGui::InputFloat("Rate", &rate);
			emitter.spawnRate = rate;

			// Particle lifetime slider
			static float lifetime = emitter.particleLifetime;
			ImGui::SliderFloat("Lifetime", &lifetime, 0.f, 5.f);
			emitter.particleLifetime = lifetime;

			// Particle size slider
			static float size = emitter.particleSize;
			ImGui::SliderFloat("Size", &size, 0.f, 1.f);
			emitter.particleSize = size;

			// Color settings
			static char startColorBuf[9];
			static char endColorBuf[9];

			ImGui::Text("  Start Color");
			ImGui::SameLine();
			ImGui::Text("   End Color");

			ImGui::Text("#");
			ImGui::SameLine();
			ImGui::PushItemWidth(80.f);
			ImGui::InputText("##Start Color", startColorBuf, 9, ImGuiInputTextFlags_CharsHexadecimal);

			ImGui::SameLine();
			ImGui::Text("#");
			ImGui::SameLine();
			ImGui::InputText("##End Color", endColorBuf, 9, ImGuiInputTextFlags_CharsHexadecimal);
			ImGui::PopItemWidth();


			if (strlen(startColorBuf) == 8)
				emitter.startColor = CharsToColor(startColorBuf);

			if (strlen(endColorBuf) == 8)
				emitter.endColor = CharsToColor(endColorBuf);

			// Spawn type settings
			static int spawnType = 0;
			bool clicked = false;

			ImGui::NewLine();
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

			static char buf[256] = "Assets/Models/Textures/Flipbook/smoke_4x4.png";
			static std::string currentTexture(buf);
			
			ImGui::NewLine();
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

			static int behavior = 0;
			constexpr const char* behaviors[] = { "Default", "Gravity", "No Gravity", "Gravity Point", "Gravity Direction", "Constant Velocity" };
			ImGui::NewLine();
			ImGui::LabelText("", "Behavior");
			bool switched = ImGui::Combo("", &behavior, behaviors, _countof(behaviors), -1);
			if (switched)
			{
				switch (behavior)
				{
				case 0: SwitchToBehaviorComponent<nullptr_t>(); break;
				case 1: SwitchToBehaviorComponent<GravityBehaviorComponent>(); break;
				case 2: SwitchToBehaviorComponent<NoGravityBehaviorComponent>(); break;
				case 3: SwitchToBehaviorComponent<GravityPointBehaviorComponent>(); break;
				case 4: SwitchToBehaviorComponent<GravityDirectionBehaviorComponent>(); break;
				case 5: SwitchToBehaviorComponent<ConstVelocityBehaviorComponent>(); break;
				}
			}
			switch (behavior)
			{
			case 1: GravityOptions(); break;
			case 3: GravityPointOptions(); break;
			case 4: GravityDirectionOptions(); break;
			case 5: ConstVelocityOptions(); break;
			}
		}

		ImGui::End();
	}

}

void ParticleScene::SpawnParticleSystem()
{
	m_particleSystem = CreateEntity();
	AddComponent<TransformComponent>(m_particleSystem, Vector3(0, 0, 0));
	auto& em = AddComponent<ParticleEmitterComponent>(m_particleSystem);
	em = {
		.spawnRate = 2048.f,
		.particleSize = 0.1f,
		.particleLifetime = 0.5f,
		.startColor = Vector4(1, 0, 0, 1),
		.endColor = Vector4(0, 0, 1, 1),
	};

	AddComponent<ConeSpawnComponent>(m_particleSystem) = {
		.angle = XM_PIDIV4,
		.speed = 10.f,
	};
}

Vector4 ParticleScene::CharsToColor(const char* color)
{
	f32 r = ((color[0]-'0')*16.f) + (color[1]-'0');
	f32 g = ((color[2]-'0')*16.f) + (color[3]-'0');
	f32 b = ((color[4]-'0')*16.f) + (color[5]-'0');
	f32 a = ((color[6]-'0')*16.f) + (color[7]-'0');
	return Vector4(r, g, b, a) / 255.f;
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

void ParticleScene::GravityOptions()
{
	auto& gravityBehavior = EntityManager::Get().GetComponent<GravityBehaviorComponent>(m_particleSystem);
	ImGui::InputFloat("Gravity", &gravityBehavior.gravity);
}
void ParticleScene::GravityPointOptions()
{
	auto& pointBehavior = EntityManager::Get().GetComponent<GravityPointBehaviorComponent>(m_particleSystem);
	ImGui::InputFloat("Gravity", &pointBehavior.gravity);
	ImGui::InputFloat3("Point", (float*)&pointBehavior.point);

}
void ParticleScene::GravityDirectionOptions()
{
	auto& dirBehavior = EntityManager::Get().GetComponent<GravityDirectionBehaviorComponent>(m_particleSystem);
	ImGui::InputFloat("Gravity", &dirBehavior.gravity);
	ImGui::InputFloat3("Direction", (float*)&dirBehavior.direction);
}
void ParticleScene::ConstVelocityOptions()
{
	auto& velBehavior = EntityManager::Get().GetComponent<ConstVelocityBehaviorComponent>(m_particleSystem);
	ImGui::InputFloat3("Velocity", (float*)&velBehavior.velocity);
}

