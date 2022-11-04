#include <DOGEngine.h>
#include "GameLayer.h"
#include "TestScene.h"
#include "OldDefaultScene.h"
#include "TunnelScenes.h"
#include "SimpleAnimationSystems.h"
#include "ExplosionSystems.h"
#include "HomingMissileSystem.h"
#include "PcgLevelLoader.h"
#include "PrefabInstantiatorFunctions.h"

using namespace DOG;
using namespace DirectX;
using namespace DirectX::SimpleMath;

GameLayer::GameLayer() noexcept
	: Layer("Game layer"), m_entityManager{ DOG::EntityManager::Get() }, m_gameState(GameState::Initializing)
{
	LuaMain::GetScriptManager()->SortOrderScripts();
	//Do startup of lua
	LuaMain::GetScriptManager()->RunLuaFile("LuaStartUp.lua");
	//Register Lua interfaces
	RegisterLuaInterfaces();
	m_entityManager.RegisterSystem(std::make_unique<DoorOpeningSystem>());
	m_entityManager.RegisterSystem(std::make_unique<LerpAnimationSystem>());
	m_entityManager.RegisterSystem(std::make_unique<LerpColorSystem>());
	m_entityManager.RegisterSystem(std::make_unique<MVPFlashlightMoveSystem>());
	m_entityManager.RegisterSystem(std::make_unique<HomingMissileTargetingSystem>());
	m_entityManager.RegisterSystem(std::make_unique<HomingMissileSystem>());
	m_entityManager.RegisterSystem(std::make_unique<HomingMissileImpacteSystem>());
	m_entityManager.RegisterSystem(std::make_unique<ExplosionSystem>());
	m_entityManager.RegisterSystem(std::make_unique<ExplosionEffectSystem>());
	m_entityManager.RegisterSystem(std::make_unique<PlayerMovementSystem>());
	m_entityManager.RegisterSystem(std::make_unique<PlayerJumpRefreshSystem>());
	
	m_entityManager.RegisterSystem(std::make_unique<MVPFlashlightStateSystem>());
	m_entityManager.RegisterSystem(std::make_unique<DeleteNetworkSync>());
	m_agentManager = new AgentManager();
	m_nrOfPlayers = MAX_PLAYER_COUNT;
	m_networkStatus = 0;


	m_keyBindingDescriptions.emplace_back("wasd", "walk");
	m_keyBindingDescriptions.emplace_back("space", "jump");
	m_keyBindingDescriptions.emplace_back("lmb", "shoot");
	m_keyBindingDescriptions.emplace_back("r", "reload");
	m_keyBindingDescriptions.emplace_back("g", "active item");
	m_keyBindingDescriptions.emplace_back("f", "flash light");
	m_keyBindingDescriptions.emplace_back("m", "gun effect");
	m_keyBindingDescriptions.emplace_back("e", "interact");
	m_keyBindingDescriptions.emplace_back("q", "full auto");
	m_keyBindingDescriptions.emplace_back("alt + enter", "fullscreen");
	m_keyBindingDescriptions.emplace_back("h", "debug camera");
	m_keyBindingDescriptions.emplace_back("f1", "debug menu");

	assert(std::filesystem::exists(("Assets/Fonts/Robot Radicals.ttf")));
	ImGui::GetIO().Fonts->AddFontDefault();
	m_imguiFont = ImGui::GetIO().Fonts->AddFontFromFileTTF("Assets/Fonts/Robot Radicals.ttf", 18.0f);
}

GameLayer::~GameLayer()
{
	delete m_agentManager;
}

void GameLayer::OnAttach()
{
	DOG::ImGuiMenuLayer::RegisterDebugWindow("GameManager", std::bind(&GameLayer::GameLayerDebugMenu, this, std::placeholders::_1), false, std::make_pair(DOG::Key::LCtrl, DOG::Key::G));
	DOG::ImGuiMenuLayer::RegisterDebugWindow("Cheats", std::bind(&GameLayer::CheatDebugMenu, this, std::placeholders::_1));

	//m_testScene = std::make_unique<TestScene>();
	//m_testScene->SetUpScene();
}

void GameLayer::OnDetach()
{
	DOG::ImGuiMenuLayer::UnRegisterDebugWindow("GameManager");
	DOG::ImGuiMenuLayer::UnRegisterDebugWindow("Cheats");
	m_testScene.reset();
	m_testScene = nullptr;

	m_mainScene.reset();
	m_mainScene = nullptr;
}

void GameLayer::OnUpdate()
{
	MINIPROFILE
	switch (m_gameState)
	{
	case GameState::None:
		break;
	case GameState::Initializing:
		m_gameState = GameState::StartPlaying;
		break;
	case GameState::Lobby:
		UpdateLobby();
		break;
	case GameState::StartPlaying:
		StartMainScene();
		break;
	case GameState::Playing:
		UpdateGame();
		break;
	case GameState::Won:
		CloseMainScene();
		m_gameState = GameState::Lobby;
		break;
	case GameState::Lost:
		CloseMainScene();
		m_gameState = GameState::Lobby;
		break;
	case GameState::Exiting:
		CloseMainScene();
		m_gameState = GameState::None;
		break;
	case GameState::Restart:
		CloseMainScene();
		m_gameState = GameState::StartPlaying;
		break;
	default:
		break;
	}

	if (m_networkStatus > 0)
		m_netCode.OnUpdate(m_agentManager);
	LuaGlobal* global = LuaMain::GetGlobal();
	global->SetNumber("DeltaTime", Time::DeltaTime());
	global->SetNumber("ElapsedTime", Time::ElapsedTime());

	KeyBindingDisplayMenu();
}

void GameLayer::StartMainScene()
{
	assert(m_mainScene == nullptr);
	
	switch (m_selectedScene)
	{
	case SceneComponent::Type::TunnelRoom0Scene:
		/************************** tunnel scene *********************************/
		m_mainScene = std::make_unique<TunnelRoom0Scene>(m_nrOfPlayers, std::bind(&GameLayer::SpawnAgents, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
		m_mainScene->SetUpScene();
		break;
	case SceneComponent::Type::TunnelRoom1Scene:
		/************************** tunnel scene *********************************/
		m_mainScene = std::make_unique<TunnelRoom1Scene>(m_nrOfPlayers, std::bind(&GameLayer::SpawnAgents, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
		m_mainScene->SetUpScene();
		break;
	case SceneComponent::Type::TunnelRoom2Scene:
		/************************** tunnel scene *********************************/
		m_mainScene = std::make_unique<TunnelRoom2Scene>(m_nrOfPlayers, std::bind(&GameLayer::SpawnAgents, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
		m_mainScene->SetUpScene();
		break;
	case SceneComponent::Type::TunnelRoom3Scene:
		/************************** tunnel scene *********************************/
		m_mainScene = std::make_unique<TunnelRoom3Scene>(m_nrOfPlayers, std::bind(&GameLayer::SpawnAgents, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
		m_mainScene->SetUpScene();
		break;
	case SceneComponent::Type::OldDefaultScene:
		/************************** old default scene *********************************/
		m_mainScene = std::make_unique<OldDefaultScene>(m_nrOfPlayers, std::bind(&GameLayer::SpawnAgents, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
		m_mainScene->SetUpScene();
		break;
	default:
		break;
	}

	LuaMain::GetScriptManager()->StartScripts();
	m_netCode.OnStartup();
	m_gameState = GameState::Playing;
}

void GameLayer::CloseMainScene()
{
	m_mainScene.reset();
}

void GameLayer::EvaluateWinCondition()
{
	if (m_noWinLose) return;
	bool agentsAlive = false;
	EntityManager::Get().Collect<AgentIdComponent>().Do([&agentsAlive](AgentIdComponent&) { agentsAlive = true; });

	static f64 freeRoamTimeAfterWin = 0;
	if (agentsAlive)
		freeRoamTimeAfterWin = 0;
	else
		freeRoamTimeAfterWin += Time::DeltaTime();

	if (freeRoamTimeAfterWin > 8.0)
		m_gameState = GameState::Won;


	if (freeRoamTimeAfterWin)
	{
		auto r = Window::GetWindowRect();
		ImVec2 pos(r.left + abs(r.left - r.right) / 2.0f, r.top + abs(r.top - r.bottom) / 2.8f);
		ImGui::SetNextWindowPos(pos, 0, ImVec2(0.5f, 0.5f));
		const char text[] = "victory";
		if (ImGui::Begin("WinScreen", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground))
		{
			ImGui::SetWindowFontScale(3.5f);
			ImGui::PushFont(m_imguiFont);
			ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 165, 0, 255));
			ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize(text).x) / 2);
			ImGui::SetCursorPosY((ImGui::GetWindowSize().y - ImGui::CalcTextSize(text).y) / 2);
			ImGui::Text(text);
			ImGui::PopStyleColor();
			ImGui::PopFont();
		}
		ImGui::End();
	}
}

void GameLayer::EvaluateLoseCondition()
{
	if (m_noWinLose) return;
	bool playersAlive = false;
	EntityManager::Get().Collect<PlayerAliveComponent>().Do([&playersAlive](PlayerAliveComponent&) { playersAlive = true; });
	if (!playersAlive) m_gameState = GameState::Lost;
}

void GameLayer::CheckIfPlayersIAreDead()
{
	EntityManager::Get().Collect<PlayerStatsComponent, PlayerAliveComponent>().Do([&](entity e, PlayerStatsComponent& stats, PlayerAliveComponent&)
		{
			if (stats.health <= 0.0f)
			{
				// Player died
				KillPlayer(e);
			}
		});
}

void GameLayer::RespawnDeadPlayer(DOG::entity e) // TODO RespawnDeadPlayer will not be called for online players, this needs to be fixed later on.
{
	if (m_entityManager.HasComponent<PlayerAliveComponent>(e))
	{
		KillPlayer(e);
	}

	m_entityManager.AddComponent<PlayerAliveComponent>(e);
	LuaMain::GetScriptManager()->AddScript(e, "Gun.lua");

	LuaMain::GetScriptManager()->AddScript(e, "PassiveItemSystem.lua");

	LuaMain::GetScriptManager()->AddScript(e, "ActiveItemSystem.lua");

	if (m_entityManager.HasComponent<ThisPlayer>(e))
	{
		auto& stats = m_entityManager.GetComponent<PlayerStatsComponent>(e);
		stats.health = stats.maxHealth;
	}
	
	auto& controller = m_entityManager.GetComponent<PlayerControllerComponent>(e);
	m_entityManager.DeferredEntityDestruction(controller.debugCamera);
	controller.debugCamera = DOG::NULL_ENTITY;
}

void GameLayer::KillPlayer(DOG::entity e)
{
	m_entityManager.RemoveComponent<PlayerAliveComponent>(e);
	
	LuaMain::GetScriptManager()->RemoveScript(e, "Gun.lua");
	LuaMain::GetScriptManager()->RemoveScript(e, "PassiveItemSystem.lua");
	LuaMain::GetScriptManager()->RemoveScript(e, "ActiveItemSystem.lua");
	m_entityManager.RemoveComponent<ScriptComponent>(e);
	
	if (m_entityManager.HasComponent<ThisPlayer>(e))
	{
		auto& controller = m_entityManager.GetComponent<PlayerControllerComponent>(e);
		controller.debugCamera = m_mainScene->CreateEntity();

		m_entityManager.AddComponent<TransformComponent>(controller.debugCamera)
			.worldMatrix = m_entityManager.GetComponent<TransformComponent>(controller.cameraEntity);

		m_entityManager.AddComponent<CameraComponent>(controller.debugCamera).isMainCamera = true;
	}
}

void GameLayer::UpdateGame()
{
	LuaMain::GetScriptManager()->UpdateScripts();
	LuaMain::GetScriptManager()->ReloadScripts();

	HandleCheats();
	HpBarMVP();
	CheckIfPlayersIAreDead();

	EvaluateWinCondition();
	EvaluateLoseCondition();


	EntityManager::Get().Collect<TransformComponent, RigidbodyComponent>().Do([](TransformComponent& transform, RigidbodyComponent&)
		{
			if (Vector3 pos = transform.GetPosition(); pos.y < -20.0f)
			{
				pos.y = 10;
				transform.SetPosition(pos);
			}
		});
}

void GameLayer::OnRender()
{
	//...
}

void GameLayer::OnImGuiRender()
{


}

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
		Input(EVENT(KeyPressedEvent).key);	
		break;
	}
	case EventType::KeyReleasedEvent:
	{
		Release(EVENT(KeyReleasedEvent).key);
	}
	}
}

void GameLayer::UpdateLobby()
{
	bool inLobby = m_gameState == GameState::Lobby;
	static char input[64]{};

	ImGui::Text("Nr of players connected: %d", m_netCode.GetNrOfPlayers());
	if(m_networkStatus == 0)
	{
		ImGui::Text("Enter Ip address of host\n if you are host leave empty");
		ImGui::InputText("Ip", input, 64);
		if (ImGui::Button("Host"))
		{
			if (m_netCode.Host())
			{
				m_networkStatus = 1;
			}

		}
		if (ImGui::Button("Join"))
		{
			if (m_netCode.Join(input))
			{
				m_networkStatus = 2;
			}
		}
		if (ImGui::Button("Play"))
		{
			if (m_networkStatus == 1)
			{
				m_netCode.Play();
				inLobby = false;
			}
			else
			{
				m_nrOfPlayers = m_netCode.Play();
				inLobby = false;
			}

	}
	}
	else if (m_networkStatus == 1)
	{
		char ip[64];
		strcpy_s(ip, m_netCode.GetIpAdress().c_str());
		ImGui::Text("Youre ip adress: %s", ip);
		if (ImGui::Button("Play"))
		{
			if (m_networkStatus == 1)
			{
				m_netCode.Play();
				inLobby = false;
			}
			else
			{
				m_nrOfPlayers = m_netCode.Play();
				inLobby = false;
			}

		}
	}
	else if (m_networkStatus == 2)
	{
		ImGui::Text("Waiting for Host to press Play...");
	
	}
	inLobby = m_netCode.IsLobbyAlive();
	if(!inLobby)
		m_gameState = GameState::StartPlaying;
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
	luaInterface.AddFunction<EntityInterface, &EntityInterface::GetTransformScaleData>("GetTransformScaleData");
	luaInterface.AddFunction<EntityInterface, &EntityInterface::SetRotationForwardUp>("SetRotationForwardUp");
	luaInterface.AddFunction<EntityInterface, &EntityInterface::GetPlayerStats>("GetPlayerStats");
	luaInterface.AddFunction<EntityInterface, &EntityInterface::GetPlayerStat>("GetPlayerStat");
	luaInterface.AddFunction<EntityInterface, &EntityInterface::SetPlayerStats>("SetPlayerStats");
	luaInterface.AddFunction<EntityInterface, &EntityInterface::SetPlayerStat>("SetPlayerStat");
	luaInterface.AddFunction<EntityInterface, &EntityInterface::GetUp>("GetUp");
	luaInterface.AddFunction<EntityInterface, &EntityInterface::GetForward>("GetForward");
	luaInterface.AddFunction<EntityInterface, &EntityInterface::GetRight>("GetRight");
	luaInterface.AddFunction<EntityInterface, &EntityInterface::GetPlayerControllerCamera>("GetPlayerControllerCamera");
	luaInterface.AddFunction<EntityInterface, &EntityInterface::GetAction>("GetAction");
	luaInterface.AddFunction<EntityInterface, &EntityInterface::SetAction>("SetAction");
	luaInterface.AddFunction<EntityInterface, &EntityInterface::HasComponent>("HasComponent");
	luaInterface.AddFunction<EntityInterface, &EntityInterface::PlayAudio>("PlayAudio");
	luaInterface.AddFunction<EntityInterface, &EntityInterface::GetPassiveType>("GetPassiveType");
	luaInterface.AddFunction<EntityInterface, &EntityInterface::IsBulletLocal>("IsBulletLocal");
	luaInterface.AddFunction<EntityInterface, &EntityInterface::Exists>("Exists");
	

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
	luaInterface.AddFunction<PhysicsInterface, &PhysicsInterface::RBConstrainRotation>("RBConstrainRotation");
	luaInterface.AddFunction<PhysicsInterface, &PhysicsInterface::RBConstrainPosition>("RBConstrainPosition");
	
	global->SetLuaInterface(luaInterface);
	global->SetUserData<LuaInterface>(luaInterfaceObject.get(), "Physics", "PhysicsInterface");

	//-----------------------------------------------------------------------------------------------
	//Render
	luaInterfaceObject = std::make_shared<RenderInterface>();
	m_luaInterfaces.push_back(luaInterfaceObject);

	luaInterface = global->CreateLuaInterface("RenderInterface");
	luaInterface.AddFunction<RenderInterface, &RenderInterface::CreateMaterial>("CreateMaterial");

	global->SetLuaInterface(luaInterface);
	global->SetUserData<LuaInterface>(luaInterfaceObject.get(), "Render", "RenderInterface");

	//-----------------------------------------------------------------------------------------------
	//Game
	luaInterfaceObject = std::make_shared<GameInterface>();
	m_luaInterfaces.push_back(luaInterfaceObject);

	luaInterface = global->CreateLuaInterface("GameInterface");
	luaInterface.AddFunction<GameInterface, &GameInterface::ExplosionEffect>("ExplosionEffect");
	luaInterface.AddFunction<GameInterface, &GameInterface::AmmoUI>("AmmoUI");

	global->SetLuaInterface(luaInterface);
	global->SetUserData<LuaInterface>(luaInterfaceObject.get(), "Game", "GameInterface");
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
			if (key == DOG::Key::E)
				inputC.switchBarrelComp = true;
			if (key == DOG::Key::M)
				inputC.switchMagazineComp = true;
			if (key == DOG::Key::G)
				inputC.activateActiveItem = true;
			if (key == DOG::Key::R)
				inputC.reload = true;
			if (key == DOG::Key::H)
				inputC.toggleDebug = true;
			if (key == DOG::Key::C)
				inputC.toggleMoveView = true;
			if (key == DOG::Key::F)
				inputC.flashlight = !inputC.flashlight;
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
			if (key == DOG::Key::E)
				inputC.switchBarrelComp = false;
			if (key == DOG::Key::M)
				inputC.switchMagazineComp = false;
			if (key == DOG::Key::G)
				inputC.activateActiveItem = false;
			if (key == DOG::Key::R)
				inputC.reload = false;
			if (key == DOG::Key::H)
				inputC.toggleDebug = false;
			if(key == DOG::Key::C)
				inputC.toggleMoveView = false;
		});
}

std::vector<entity> GameLayer::SpawnAgents(const EntityTypes type, const Vector3& pos, u8 agentCount, f32 spread)
{
	ASSERT(EntityTypes::AgentsBegin <= type && type < EntityTypes::Agents, "type must be of in range EntityTypes::AgentBegin - EntityTypes::Agents");

	std::vector<entity> agents;
	for (auto i = 0; i < agentCount; ++i)
	{
		Vector3 offset = {
			spread * (i % 2) - (spread / 2.f),
			0,
			spread * (i / 2) - (spread / 2.f),
		};
		agents.emplace_back(m_agentManager->CreateAgent(type, pos - offset));
	}
	return agents;
}

void GameLayer::HandleCheats()
{
	entity player = GetPlayer();
	if (player == NULL_ENTITY || !EntityManager::Get().Exists(player)) return;

	m_isCheating = m_godModeCheat || m_unlimitedAmmoCheat || m_noClipCheat || m_noWinLose;

	static bool cheatWindowOpen = false;
	cheatWindowOpen |= m_isCheating;
	if (cheatWindowOpen && !ImGuiMenuLayer::IsAttached())
	{
		if (m_isCheating)
		{
			ImGui::PushStyleColor(ImGuiCol_TitleBg, IM_COL32(80, 0, 0, 255));
			ImGui::PushStyleColor(ImGuiCol_TitleBgActive, IM_COL32(255, 0, 0, 255));
			ImGui::PushStyleColor(ImGuiCol_CheckMark, IM_COL32(255, 0, 0, 255));
			ImGui::PushStyleColor(ImGuiCol_TitleBgCollapsed, IM_COL32(170, 0, 0, 255));
		}
		if (ImGui::Begin("Cheats", &cheatWindowOpen, ImGuiWindowFlags_NoFocusOnAppearing))
		{
			CheatSettingsImGuiMenu();
		}
		ImGui::End();
		if (m_isCheating)
		{
			ImGui::PopStyleColor(4);
		}
	}
	
	if (m_godModeCheat)
	{
		assert(EntityManager::Get().HasComponent<PlayerStatsComponent>(player));
		auto& stats = m_entityManager.GetComponent<PlayerStatsComponent>(player);
		stats.health = stats.maxHealth;
	}

	assert(EntityManager::Get().HasComponent<RigidbodyComponent>(player));
	m_entityManager.GetComponent<RigidbodyComponent>(player).noCollisionResponse = m_noClipCheat;
}

void GameLayer::HpBarMVP()
{
	ImVec2 size;
	size.x = 300;
	size.y = 50;

	auto r = Window::GetWindowRect();
	ImVec2 pos;
	pos.x = r.left + 50.0f;
	pos.y = r.bottom - 100.0f;

	ImGui::SetNextWindowPos(pos);
	ImGui::SetNextWindowSize(size);
	if (ImGui::Begin("HpBar", nullptr, ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground))
	{
		auto& playerStats = m_entityManager.GetComponent<PlayerStatsComponent>(GetPlayer());
		float hp = playerStats.health / playerStats.maxHealth;
		ImVec2 bottomLeft = ImGui::GetCursorScreenPos();
		ImVec2 hpTopRight;
		ImVec2 borderTopRight;
		borderTopRight.x = bottomLeft.x + Window::GetWidth() * 0.2f;
		borderTopRight.y = bottomLeft.y + Window::GetHeight() * 0.03f;
		hpTopRight.x = borderTopRight.x * hp;
		hpTopRight.y = borderTopRight.y;
		if (hp > 0.0f)
		{
			ImGui::GetWindowDrawList()->AddRectFilled(bottomLeft, hpTopRight, IM_COL32(255, 30, 0, 255));
		}
		ImGui::GetWindowDrawList()->AddRect(bottomLeft, borderTopRight, IM_COL32(30, 30, 30, 200)); // border
	}
	ImGui::End();
}

void GameLayer::KeyBindingDisplayMenu()
{
	if (!m_displayKeyBindings) return;
	ImVec2 size;
	size.x = 280;
	size.y = 300;

	auto r = Window::GetWindowRect();
	ImVec2 pos;
	pos.x = r.right - size.x - 20.0f;
	pos.y = r.top + 50.0f;
	
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
	ImGui::SetNextWindowPos(pos);
	ImGui::SetNextWindowSize(size);
	if (ImGui::Begin("KeyBindings", nullptr, ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground))
	{
		if (ImGui::BeginTable("KeyBindings", 2))
		{
			ImGui::PushFont(m_imguiFont);
			for (auto& [key, action] : m_keyBindingDescriptions)
			{
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, 200));
				ImGui::Text(action.c_str());
				ImGui::TableSetColumnIndex(1);
				ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 165, 0, 200));
				ImGui::Text(key.c_str());
				ImGui::PopStyleColor(2);
			}
			ImGui::PopFont();
			ImGui::EndTable();
		}
	}
	ImGui::End();
	ImGui::PopStyleColor();
}

void GameLayer::GameLayerDebugMenu(bool& open)
{
	if (ImGui::BeginMenu("View"))
	{
		if (ImGui::MenuItem("GameManager", "Ctrl+G"))
		{
			open = true;
		}
		ImGui::EndMenu(); // "View"
	}

	if (open)
	{
		if (ImGui::Begin("GameManager", &open))
		{
			if (ImGui::Button("Lobby"))
			{
				CloseMainScene();
				m_gameState = GameState::Lobby;
			}


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

			if (ImGui::RadioButton("Room0", (int*)&m_selectedScene, (int)SceneComponent::Type::TunnelRoom0Scene)) m_gameState = GameState::Restart;
			if (ImGui::RadioButton("Room1", (int*)&m_selectedScene, (int)SceneComponent::Type::TunnelRoom1Scene)) m_gameState = GameState::Restart;
			if (ImGui::RadioButton("Room2", (int*)&m_selectedScene, (int)SceneComponent::Type::TunnelRoom2Scene)) m_gameState = GameState::Restart;
			if (ImGui::RadioButton("Room3", (int*)&m_selectedScene, (int)SceneComponent::Type::TunnelRoom3Scene)) m_gameState = GameState::Restart;
			if (ImGui::RadioButton("OldBox", (int*)&m_selectedScene, (int)SceneComponent::Type::OldDefaultScene)) m_gameState = GameState::Restart;

			std::vector<entity> players;
			EntityManager::Get().Collect<PlayerStatsComponent>().Do([&](entity e, PlayerStatsComponent&)
				{
					players.push_back(e);
				});

			auto&& playerToString = [&](entity e) -> std::pair<std::string, std::string>
			{
				auto& stats = EntityManager::Get().GetComponent<PlayerStatsComponent>(e);
				std::string str2 = "hp: " + std::to_string(stats.health);
				std::string str1;
				if (EntityManager::Get().HasComponent<NetworkPlayerComponent>(e))
					str1 = "Player: " + std::to_string(EntityManager::Get().GetComponent<NetworkPlayerComponent>(e).playerId);
				return std::make_pair(str1, str2);
			};

			if (ImGui::BeginTable("Players", 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV))
			{
				for (int i = 0; i < players.size(); i++)
				{
					auto row = playerToString(players[i]);
					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::Selectable((row.first + "##" + std::to_string(i)).c_str(), false, ImGuiSelectableFlags_SpanAllColumns);
					if (ImGui::BeginPopupContextItem())
					{
						if (ImGui::Button("Kill player"))
						{
							EntityManager::Get().GetComponent<PlayerStatsComponent>(players[i]).health = 0;
							ImGui::CloseCurrentPopup();
						}
						ImGui::EndPopup();
					}
					ImGui::TableSetColumnIndex(1);
					ImGui::Text(row.second.c_str());
				}
				ImGui::EndTable();
			}

			ImGui::Checkbox("View KeyBindings", &m_displayKeyBindings);
		}
		ImGui::End(); // "GameManager"
	}
}

void GameLayer::CheatSettingsImGuiMenu()
{
	ImGui::Checkbox("God mode", &m_godModeCheat);
	ImGui::Checkbox("Unlimited ammo", &m_unlimitedAmmoCheat);
	ImGui::Checkbox("No clip", &m_noClipCheat);
	ImGui::Checkbox("No win or lose", &m_noWinLose);
	if (ImGui::Button("Win"))
	{
		m_gameState = GameState::Won;
	}
	if (ImGui::Button("Respawn"))
	{
		RespawnDeadPlayer(GetPlayer());
	}
}

void GameLayer::CheatDebugMenu(bool&)
{
	bool beginMenuCheats;
	if (m_isCheating)
	{
		ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 0, 0, 255));
		beginMenuCheats = ImGui::BeginMenu("Cheats");
		ImGui::PopStyleColor();
	}
	else
	{
		beginMenuCheats = ImGui::BeginMenu("Cheats");
	}

	if (beginMenuCheats)
	{
		CheatSettingsImGuiMenu();
		ImGui::EndMenu(); // "Cheats"
	}
}