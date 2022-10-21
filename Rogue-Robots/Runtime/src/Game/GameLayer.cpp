#include <DOGEngine.h>
#include "GameLayer.h"
#include "MainScene.h"
#include "TestScene.h"
#include "SimpleAnimationSystems.h"

using namespace DOG;
using namespace DirectX;
using namespace DirectX::SimpleMath;

GameLayer::GameLayer() noexcept
	: Layer("Game layer"), m_entityManager{ DOG::EntityManager::Get() }
{
	LuaMain::GetScriptManager()->SortOrderScripts();
	//Do startup of lua
	LuaMain::GetScriptManager()->RunLuaFile("LuaStartUp.lua");
	//Register Lua interfaces
	RegisterLuaInterfaces();
	
	m_entityManager.RegisterSystem(std::make_unique<DoorOpeningSystem>());
	m_entityManager.RegisterSystem(std::make_unique<LerpAnimationSystem>());
	m_entityManager.RegisterSystem(std::make_unique<LerpColorSystem>());
}

void GameLayer::OnAttach()
{
	DOG::ImGuiMenuLayer::RegisterDebugWindow("GameLayer", std::bind(&GameLayer::GameLayerDebugMenu, this, std::placeholders::_1));
	m_Agent = std::make_shared<Agent>();

	m_testScene = std::make_unique<TestScene>();
	m_testScene->SetUpScene();

	m_mainScene = std::make_unique<MainScene>();
	m_mainScene->SetUpScene({
		[this]() { return SpawnPlayers(Vector3(25, 25, 15), 4, 10.f); },
		[this]() { return LoadLevel(); },
		[this]() { return std::vector<entity>(1, m_Agent->MakeAgent(m_entityManager.CreateEntity())); }
		});

	LuaMain::GetScriptManager()->StartScripts();
}

void GameLayer::OnDetach()
{
	DOG::ImGuiMenuLayer::UnRegisterDebugWindow("GameLayer");
	m_testScene.reset();
	m_testScene = nullptr;

	m_mainScene.reset();
	m_mainScene = nullptr;
}

void GameLayer::OnUpdate()
{
	MINIPROFILE
	for (auto& system : m_entityManager)
	{
		system->EarlyUpdate();
	}
	for (auto& system : m_entityManager)
	{
		system->Update();
	}
	for (auto& system : m_entityManager)
	{
		system->LateUpdate();
	}


	LuaGlobal* global = LuaMain::GetGlobal();
	global->SetNumber("DeltaTime", Time::DeltaTime());

	m_player->OnUpdate();
	m_netCode.OnUpdate();
	LuaMain::GetScriptManager()->UpdateScripts();
	LuaMain::GetScriptManager()->ReloadScripts();
} 
void GameLayer::OnRender()
{
	//...
}


void GameLayer::OnImGuiRender()
{


}

//Place-holder example on how to use event system:
void GameLayer::OnEvent(DOG::IEvent& event)
{
	using namespace DOG;
	switch (event.GetEventType())
	{
	case EventType::LeftMouseButtonPressedEvent:
	{
		EntityManager::Get().Collect<InputController, ThisPlayer>().Do([&](InputController& inputC, ThisPlayer& )
			{
				inputC.shoot = true;
			});
		break;
	}
	case EventType::LeftMouseButtonReleasedEvent:
	{
		EntityManager::Get().Collect<InputController, ThisPlayer>().Do([&](InputController& inputC, ThisPlayer&)
			{
				inputC.shoot = false;
			});
		break;
	}
	case EventType::KeyPressedEvent:
	{
		if (EVENT(KeyPressedEvent).key == DOG::Key::C)
			m_player->m_moveView = !m_player->m_moveView;
		else
			Input(EVENT(KeyPressedEvent).key);
		break;
	}
	case EventType::KeyReleasedEvent:
	{
		Release(EVENT(KeyReleasedEvent).key);
	}
	}
}

void GameLayer::RegisterLuaInterfaces()
{
	LuaGlobal* global = LuaMain::GetGlobal();

	global->SetNumber("DeltaTime", Time::DeltaTime());

	//-----------------------------------------------------------------------------------------------
	//Input
	//Create a luaInterface variable that holds the interface object (is reused for all interfaces)
	std::shared_ptr<LuaInterface> luaInterfaceObject = std::make_shared<InputInterface>();
	m_luaInterfaces.push_back(luaInterfaceObject); //Add it to the gamelayer's interfaces.
	
	auto luaInterface = global->CreateLuaInterface("InputInterface"); //Register a new interface in lua.
	//Add all functions that are needed from the interface class.
	luaInterface.AddFunction<InputInterface, &InputInterface::IsLeftPressed>("IsLeftPressed");
	luaInterface.AddFunction<InputInterface, &InputInterface::IsRightPressed>("IsRightPressed");
	luaInterface.AddFunction<InputInterface, &InputInterface::IsKeyPressed>("IsKeyPressed");
	luaInterface.AddFunction<InputInterface, &InputInterface::GetMouseDelta>("MouseDelta");
	global->SetLuaInterface(luaInterface);
	//Make the object accessible from lua. Is used by: Input.FunctionName()
	global->SetUserData<LuaInterface>(luaInterfaceObject.get(), "Input", "InputInterface");

	//-----------------------------------------------------------------------------------------------
	//Entities
	luaInterfaceObject = std::make_shared<EntityInterface>();
	m_luaInterfaces.push_back(luaInterfaceObject);

	luaInterface = global->CreateLuaInterface("EntityInterface");
	luaInterface.AddFunction<EntityInterface, &EntityInterface::CreateEntity>("CreateEntity");
	luaInterface.AddFunction<EntityInterface, &EntityInterface::DestroyEntity>("DestroyEntity");
	luaInterface.AddFunction<EntityInterface, &EntityInterface::AddComponent>("AddComponent");
	luaInterface.AddFunction<EntityInterface, &EntityInterface::ModifyComponent>("ModifyComponent");
	luaInterface.AddFunction<EntityInterface, &EntityInterface::GetTransformPosData>("GetTransformPosData");
	luaInterface.AddFunction<EntityInterface, &EntityInterface::SetRotationForwardUp>("SetRotationForwardUp");
	luaInterface.AddFunction<EntityInterface, &EntityInterface::GetPlayerStats>("GetPlayerStats");
	luaInterface.AddFunction<EntityInterface, &EntityInterface::GetUp>("GetUp");
	luaInterface.AddFunction<EntityInterface, &EntityInterface::GetForward>("GetForward");
	luaInterface.AddFunction<EntityInterface, &EntityInterface::GetRight>("GetRight");
	luaInterface.AddFunction<EntityInterface, &EntityInterface::GetAction>("GetAction");
	luaInterface.AddFunction<EntityInterface, &EntityInterface::SetAction>("SetAction");
	luaInterface.AddFunction<EntityInterface, &EntityInterface::HasComponent>("HasComponent");
	luaInterface.AddFunction<EntityInterface, &EntityInterface::PlayAudio>("PlayAudio");

	global->SetLuaInterface(luaInterface);

	global->SetUserData<LuaInterface>(luaInterfaceObject.get(), "Entity", "EntityInterface");


	//-----------------------------------------------------------------------------------------------
	//Scene
	luaInterfaceObject = std::make_shared<SceneInterface>();
	m_luaInterfaces.push_back(luaInterfaceObject);

	luaInterface = global->CreateLuaInterface("SceneInterface");

	luaInterface.AddFunction<SceneInterface, &SceneInterface::CreateEntity>("CreateEntity");

	global->SetLuaInterface(luaInterface);
	global->SetUserData<LuaInterface>(luaInterfaceObject.get(), "Scene", "SceneInterface");


	//-----------------------------------------------------------------------------------------------
	//Assets
	luaInterfaceObject = std::make_shared<AssetInterface>();
	m_luaInterfaces.push_back(luaInterfaceObject);

	luaInterface = global->CreateLuaInterface("AssetInterface");
	luaInterface.AddFunction<AssetInterface, &AssetInterface::LoadModel>("LoadModel");
	luaInterface.AddFunction<AssetInterface, &AssetInterface::LoadAudio>("LoadAudio");
	global->SetLuaInterface(luaInterface);

	global->SetUserData<LuaInterface>(luaInterfaceObject.get(), "Asset", "AssetInterface");

	//-----------------------------------------------------------------------------------------------
	//Host
	m_player = std::make_shared<MainPlayer>();
	luaInterfaceObject = std::make_shared<HostInterface>();
	m_luaInterfaces.push_back(luaInterfaceObject);

	luaInterface = global->CreateLuaInterface("HostInterface");
	luaInterface.AddFunction<HostInterface, &HostInterface::DistanceToPlayers>("DistanceToPlayers");
	global->SetLuaInterface(luaInterface);

	global->SetUserData<LuaInterface>(luaInterfaceObject.get(), "Host", "HostInterface");

	//-----------------------------------------------------------------------------------------------
	//Physics
	luaInterfaceObject = std::make_shared<PhysicsInterface>();
	m_luaInterfaces.push_back(luaInterfaceObject);

	luaInterface = global->CreateLuaInterface("PhysicsInterface");
	luaInterface.AddFunction<PhysicsInterface, &PhysicsInterface::RBSetVelocity>("RBSetVelocity");
	luaInterface.AddFunction<PhysicsInterface, &PhysicsInterface::Explosion>("Explosion");
	
	global->SetLuaInterface(luaInterface);
	global->SetUserData<LuaInterface>(luaInterfaceObject.get(), "Physics", "PhysicsInterface");
}

std::vector<entity> GameLayer::LoadLevel()
{
	float blockDim = 5.0f;

	std::string line;
	std::ifstream inputFile("..\\Offline-Tools\\PCG\\showOff_generatedLevel.txt");

	AssetManager& aManager = AssetManager::Get();

	std::vector<entity> levelBlocks;

	unsigned x = 0;
	unsigned y = 0;
	unsigned z = 0;
	float piDiv2 = DirectX::XM_PIDIV2;
	if (inputFile.is_open())
	{
		while (std::getline(inputFile, line))
		{
			if (line[0] != '-')
			{
				while (line.find(' ') != std::string::npos)
				{
					size_t delimPos = line.find(' ');	
					std::string block = line.substr(0, delimPos);
					line.erase(0, delimPos + 1);
					if (block != "Empty" && block != "Void" && block != "q")
					{
						size_t firstUnderscore = block.find('_');
						size_t secondUnderscore = block.find('_', firstUnderscore + 1);
						std::string blockName = block.substr(0, firstUnderscore);
						int blockRot = std::stoi(block.substr(firstUnderscore + 2, secondUnderscore - firstUnderscore - 2));
						std::string blockFlip = block.substr(secondUnderscore + 1, block.size() - secondUnderscore - 1);

						float xFlip = 1.0f;
						float yFlip = 1.0f;
						if (blockFlip.find('x') != std::string::npos)
						{
							xFlip = -1.0f;
						}
						if (blockFlip.find('y') != std::string::npos)
						{
							yFlip = -1.0f;
						}

						//Correct scaling for the mesh colliders (I think)
						Vector3 localMeshColliderScale = Vector3(-xFlip, yFlip, 1.0f);

						entity blockEntity = levelBlocks.emplace_back(m_entityManager.CreateEntity());
						m_entityManager.AddComponent<ModelComponent>(blockEntity, aManager.LoadModelAsset("Assets/Models/ModularBlocks/" + blockName + ".fbx"));
						TransformComponent& transform = m_entityManager.AddComponent<TransformComponent>(blockEntity,
							Vector3(x * blockDim, y * blockDim, z * blockDim),
							Vector3(piDiv2, piDiv2 * blockRot - piDiv2, 0.0f),
							Vector3(1.0f, 1.0f, 1.0f)); //Moved the scaling to later so the mesh collider is not confused by the scaling

						m_entityManager.AddComponent<ModularBlockComponent>(blockEntity);
						m_entityManager.AddComponent<MeshColliderComponent>(blockEntity,
							blockEntity, 
							aManager.LoadModelAsset("Assets/Models/ModularBlocks/" + blockName + "_Col.fbx", (DOG::AssetLoadFlag)((DOG::AssetLoadFlag::Async) | (DOG::AssetLoadFlag)(DOG::AssetLoadFlag::CPUMemory | DOG::AssetLoadFlag::GPUMemory))),
							localMeshColliderScale,
							false);		// Set this to true if you want to see colliders only in wireframe

						//Sets the stupid scaling last seems to fix our problems!
						transform.SetScale(Vector3(xFlip, -1.0f * yFlip, 1.0f)); //yFlip is on Z because of left-hand/right-hand.
					}

					++x;
				}
				x = 0;
				++y;
			}
			else
			{
				x = 0;
				y = 0;
				++z;
			}
		}
	}
	return levelBlocks;
}

void GameLayer::Input(DOG::Key key)
{
	EntityManager::Get().Collect<InputController, ThisPlayer>().Do([&](InputController& inputC, ThisPlayer&)
	{
			if (key == DOG::Key::W)
				inputC.forward = true;
			if (key == DOG::Key::A)
				inputC.left = true;
			if (key == DOG::Key::S)
				inputC.backwards = true;
			if (key == DOG::Key::D)
				inputC.right = true;
			if (key == DOG::Key::LShift)
				inputC.down = true;
			if (key == DOG::Key::Spacebar)
				inputC.up = true;
			if (key == DOG::Key::Q)
				inputC.switchComp = true;

	});
}

void GameLayer::Release(DOG::Key key)
{
	EntityManager::Get().Collect<InputController, ThisPlayer>().Do([&](InputController& inputC, ThisPlayer&)
		{
			if (key == DOG::Key::W)
				inputC.forward = false;
			if (key == DOG::Key::A)
				inputC.left = false;
			if (key == DOG::Key::S)
				inputC.backwards = false;
			if (key == DOG::Key::D)
				inputC.right = false;
			if (key == DOG::Key::LShift)
				inputC.down = false;
			if (key == DOG::Key::Spacebar)
				inputC.up = false;
			if (key == DOG::Key::Q)
				inputC.switchComp = false;
		});
}

std::vector<entity> GameLayer::SpawnPlayers(const Vector3& pos, u8 playerCount, f32 spread)
{
	ASSERT(playerCount > 0, "Need to at least spawn ThisPlayer. I.e. playerCount has to exceed 0");
	ASSERT(playerCount <= MAX_PLAYER_COUNT, "No more than 4 players can be spawned. I.e. playerCount can't exceed 4");

	auto& am = DOG::AssetManager::Get();
	m_playerModels[0] = am.LoadModelAsset("Assets/Models/Temporary_Assets/red_cube.glb");
	m_playerModels[1] = am.LoadModelAsset("Assets/Models/Temporary_Assets/green_cube.glb", (DOG::AssetLoadFlag)((DOG::AssetLoadFlag::Async) | (DOG::AssetLoadFlag)(DOG::AssetLoadFlag::GPUMemory | DOG::AssetLoadFlag::CPUMemory)));
	m_playerModels[2] = am.LoadModelAsset("Assets/Models/Temporary_Assets/blue_cube.glb");
	m_playerModels[3] = am.LoadModelAsset("Assets/Models/Temporary_Assets/magenta_cube.glb");
	std::vector<entity> players;
	auto* scriptManager = LuaMain::GetScriptManager();
	for (auto i = 0; i < playerCount; ++i)
	{
		entity playerI = players.emplace_back(m_entityManager.CreateEntity());
		Vector3 offset = {
			spread * (i % 2) - (spread / 2.f),
			0,
			spread * (i / 2) - (spread / 2.f),
		};
		m_entityManager.AddComponent<TransformComponent>(playerI, pos - offset);
		m_entityManager.AddComponent<ModelComponent>(playerI, m_playerModels[i]);
		m_entityManager.AddComponent<CapsuleColliderComponent>(playerI, playerI, 1.f, 1.8f, true, 75.f);
		auto& rb = m_entityManager.AddComponent<RigidbodyComponent>(playerI, playerI);
		rb.ConstrainRotation(true, true, true);
		rb.disableDeactivation = true;
		rb.getControlOfTransform = false;

		m_entityManager.AddComponent<NetworkPlayerComponent>(playerI).playerId = i;
		m_entityManager.AddComponent<InputController>(playerI);
		scriptManager->AddScript(playerI, "Gun.lua");

		if (i == 0) // Only for this player
		{
			m_entityManager.AddComponent<ThisPlayer>(playerI);
			m_entityManager.AddComponent<CameraComponent>(playerI);
			m_entityManager.AddComponent<AudioListenerComponent>(playerI);
		}
		else
		{
			m_entityManager.AddComponent<OnlinePlayer>(playerI);
		}
	}
	return players;
}

void GameLayer::GameLayerDebugMenu(bool& open)
{
	if (ImGui::BeginMenu("View"))
	{
		if (ImGui::MenuItem("GameLayer"))
		{
			open = true;
		}
		ImGui::EndMenu(); // "View"
	}

	if (open)
	{
		if (ImGui::Begin("GameLayer", &open))
		{
			bool checkboxTestScene = m_testScene != nullptr;
			if (ImGui::Checkbox("TestScene", &checkboxTestScene))
			{
				if (checkboxTestScene)
				{
					m_testScene = std::make_unique<TestScene>();
					m_testScene->SetUpScene();
				}
				else
				{
					m_testScene.reset();
					m_testScene = nullptr;
				}
			}

			bool checkboxMainScene = m_mainScene != nullptr;
			if (ImGui::Checkbox("MainScene", &checkboxMainScene))
			{
				if (checkboxMainScene)
				{
					m_mainScene = std::make_unique<MainScene>();
					m_mainScene->SetUpScene();
				}
				else
				{
					m_mainScene.reset();
					m_mainScene = nullptr;
				}
			}
		}
		ImGui::End(); // "GameLayer"
	}
}


