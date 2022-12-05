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

			static char buf[256] = "Assets/Textures/Flipbook/smoke_4x4.png";
			
			ImGui::NewLine();
			ImGui::Checkbox("Use texture", &enableTexture);
			if (!enableTexture)
			{
				emitter.textureHandle = 0;
				m_texturePath = "";
			}
			else
			{
				ImGui::InputText("Texture Path", buf, 256);
				std::string newTexture(buf);
				if (std::filesystem::is_regular_file(newTexture))
				{
					if (newTexture != m_texturePath)
					{
						auto textureAsset = AssetManager::Get().LoadTexture(newTexture, AssetLoadFlag::GPUMemory);
						emitter.textureHandle = AssetManager::Get().GetAsset<TextureAsset>(textureAsset)->textureViewRawHandle;
						m_texturePath = newTexture;
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
			
			// Baking
			static char bakePath[255] = { 0 };
			ImGui::NewLine();
			ImGui::InputText("System Path", bakePath, 255);
			if (ImGui::Button("Bake To File"))
			{
				BakeSystemToFile(bakePath);
			}
			if (ImGui::Button("Load From File"))
			{
				UnbakeSystemFromFile(bakePath);
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
	f32 colorOut[4];
	for (int i = 0; i < 8; i += 2)
	{
		colorOut[i/2] = color[i] < 'A'
			? 16.f * (color[i]-'0')
			: (color[i] < 'a')
			? 16.f * (color[i]-'A'+10)
			: 16.f * (color[i]-'a'+10);

		colorOut[i/2] += color[i+1] < 'A'
			? (color[i+1]-'0')
			: (color[i+1] < 'a')
			? (color[i+1]-'A'+10)
			: (color[i+1]-'a'+10);
	}
	return Vector4(colorOut) / 255.f;
}

void ParticleScene::BakeSystemToFile(const std::filesystem::path& path)
{
	auto& emitter = EntityManager::Get().GetComponent<ParticleEmitterComponent>(m_particleSystem);
	LuaTable system;
	LuaTable startColor;
	LuaTable endColor;

	std::ofstream bakeFile(path);
	ASSERT(bakeFile, "Failed to open file for baking particle system");
	bakeFile << "system={\n";
	bakeFile << "\trate=" << emitter.spawnRate << ",\n";
	bakeFile << "\tlifetime=" << emitter.particleLifetime << ",\n";
	bakeFile << "\tsize=" << emitter.particleSize << ",\n";
	bakeFile << "\ttexture=\"" << m_texturePath << "\",\n";
	bakeFile << "\ttextureSegmentsX=" << emitter.textureSegmentsX << ",\n";
	bakeFile << "\ttextureSegmentsY=" << emitter.textureSegmentsY << ",\n";
	bakeFile << "\tstartColor={\n";
	bakeFile << "\t\tr=" << emitter.startColor.x << ",\n";
	bakeFile << "\t\tg=" << emitter.startColor.y << ",\n";
	bakeFile << "\t\tb=" << emitter.startColor.z << ",\n";
	bakeFile << "\t\ta=" << emitter.startColor.w << "\n";
	bakeFile << "\t},\n";
	bakeFile << "\tendColor={\n";
	bakeFile << "\t\tr=" << emitter.endColor.x << ",\n";
	bakeFile << "\t\tg=" << emitter.endColor.y << ",\n";
	bakeFile << "\t\tb=" << emitter.endColor.z << ",\n";
	bakeFile << "\t\ta=" << emitter.endColor.w << "\n";
	bakeFile << "\t},\n";

	WriteSpawnTable(bakeFile);
	WriteBehaviorTable(bakeFile);

	bakeFile << "}\n";
}

void ParticleScene::UnbakeSystemFromFile(const std::filesystem::path& path)
{
	
}

void ParticleScene::WriteSpawnTable(std::ofstream& file)
{
	auto& entityManager = EntityManager::Get();

	file << "\tspawn={\n";
	if (auto opt = entityManager.TryGetComponent<ConeSpawnComponent>(m_particleSystem))
	{
		auto& comp = opt.value().get();
		file << "\t\ttype=\"cone\",\n";
		file << "\t\tangle=" << comp.angle <<",\n";
		file << "\t\tspeed=" << comp.speed <<"\n\t},\n";
		return;
	}
	if (auto opt = entityManager.TryGetComponent<CylinderSpawnComponent>(m_particleSystem))
	{
		auto& comp = opt.value().get();
		file << "\t\ttype=\"cylinder\",\n";
		file << "\t\tangle=" << comp.radius <<",\n";
		file << "\t\tspeed=" << comp.height <<"\n\t},\n";
		return;
	}
	if (auto opt = entityManager.TryGetComponent<BoxSpawnComponent>(m_particleSystem))
	{
		auto& comp = opt.value().get();
		file << "\t\ttype=\"box\",\n";
		file << "\t\tx=" << comp.x <<",\n";
		file << "\t\ty=" << comp.y <<",\n";
		file << "\t\tz=" << comp.z <<"\n\t},\n";
		return;
	}

	file << "\t\ttype=\"default\"\n},\n";
}
void ParticleScene::WriteBehaviorTable(std::ofstream& file)
{
	auto& entityManager = EntityManager::Get();

	file << "\tbehavior={\n";

	if (auto opt = entityManager.TryGetComponent<GravityBehaviorComponent>(m_particleSystem))
	{
		auto& comp = opt.value().get();
		file << "\t\ttype=\"gravity\",\n";
		file << "\t\tg=" << comp.gravity<<"\n\t},\n";
		return;
	}
	if (auto opt = entityManager.TryGetComponent<NoGravityBehaviorComponent>(m_particleSystem))
	{
		file << "\t\ttype=\"noGravity\"\n\t},\n";
		return;
	}
	if (auto opt = entityManager.TryGetComponent<GravityDirectionBehaviorComponent>(m_particleSystem))
	{
		auto& comp = opt.value().get();
		file << "\t\ttype=\"gravityDirection\",\n";
		file << "\t\tx=" << comp.direction.x <<",\n";
		file << "\t\ty=" << comp.direction.y <<",\n";
		file << "\t\tz=" << comp.direction.z <<"\n\t},\n";
		return;
	}
	if (auto opt = entityManager.TryGetComponent<GravityPointBehaviorComponent>(m_particleSystem))
	{
		auto& comp = opt.value().get();
		file << "\t\ttype=\"gravityPoint\",\n";
		file << "\t\tx=" << comp.point.x <<",\n";
		file << "\t\ty=" << comp.point.y <<",\n";
		file << "\t\tz=" << comp.point.z <<"\n\t},\n";
		return;
	}
	if (auto opt = entityManager.TryGetComponent<ConstVelocityBehaviorComponent>(m_particleSystem))
	{
		auto& comp = opt.value().get();
		file << "\t\ttype=\"constVelocity\",\n";
		file << "\t\tx=" << comp.velocity.x <<",\n";
		file << "\t\ty=" << comp.velocity.y <<",\n";
		file << "\t\tz=" << comp.velocity.z <<"\n\t},\n";
		return;
	}
	
	file << "\t\ttype=\"default\"\n\t},\n";
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

