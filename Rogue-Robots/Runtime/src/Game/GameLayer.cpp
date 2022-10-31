#include <DOGEngine.h>
#include "GameLayer.h"
#include "MainScene.h"
#include "TestScene.h"
#include "SimpleAnimationSystems.h"
#include "ExplosionSystems.h"
#include "HomingMissileSystem.h"

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
	
	m_entityManager.RegisterSystem(std::make_unique<MVPFlashlightStateSystem>());
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
	m_mainScene = std::make_unique<MainScene>();
	m_mainScene->SetUpScene({
		[this]() 
		{
			/************************** default scene *********************************/
			//std::vector<entity> players = SpawnPlayers(Vector3(25.0f, 15.0f, 25.0f), m_nrOfPlayers, 10.f);
			
			/************************** tunnel scene *********************************/
			// room 0: small room - maybe nice entry point?
			std::vector<entity> players = SpawnPlayers(Vector3(12.0f, 90.0f, 38.0f), m_nrOfPlayers, 10.f);

			// room 1: a few rooms connected by tunnels
			//std::vector<entity> players = SpawnPlayers(Vector3(2.0f, 80.0f, 13.0f), m_nrOfPlayers, 3.f);		// location 1

			// room 2: a larger, more open room
			//std::vector<entity> players = SpawnPlayers(Vector3(106.0f, 80.0f, 31.0f), m_nrOfPlayers, 5.0f); // locaton 1
			
			// room 3: huge cave system
			//std::vector<entity> players = SpawnPlayers(Vector3(76.5f, 56.0f, 68.0f), m_nrOfPlayers, 2.8f); // locaton 1
			
			std::vector<entity> flashlights = AddFlashlightsToPlayers(players);

			// Add room 0 component to players
			EntityManager::Get().Collect<PlayerStatsComponent>().Do([&](entity e, PlayerStatsComponent&)
				{
					EntityManager::Get().AddComponent<PlayersRoom0Component>(e);
				});

			//For now, just combine them, using the player vector:
			players.insert(players.end(), flashlights.begin(), flashlights.end());
			return players;
		},
		[this]() { return LoadLevel(); },
			/************************** default scene *********************************/
			//[this]() { return SpawnAgents(EntityTypes::Scorpio, Vector3(20, 20, 50), 10, 3.0f); },
			//[this]() { return SpawnAgents(EntityTypes::Scorpio, Vector3(30, 20, 50), 10, 3.0f); },
			//[this]() { return SpawnAgents(EntityTypes::Scorpio, Vector3(40, 20, 50), 10, 3.0f); },

			/************************** tunnel scene *********************************/
			// a few rooms connected by tunnels
			[this]() { return SpawnAgents(EntityTypes::Scorpio1, Vector3(58.f, 80.f, 40.f), 3, 2.5f); },		// location 2
			[this]() { return SpawnAgents(EntityTypes::Scorpio2, Vector3(68.f, 78.f, 27.f), 4, 5.f); },			// location 3
			[this]() { return SpawnAgents(EntityTypes::Scorpio3, Vector3(37.f, 80.f, 8.f), 7, 3.f); },			// location 4

			// a larger, more open room
			[this]() { return SpawnAgents(EntityTypes::Scorpio4, Vector3(78.f, 80.f, 63.f), 4, 2.5f); },		// location 2
			[this]() { return SpawnAgents(EntityTypes::Scorpio4, Vector3(104.f, 80.f, 65.f), 5, 4.f); },		// location 3
			[this]() { return SpawnAgents(EntityTypes::Scorpio5, Vector3(124.f, 80.f, 65.f), 4, 1.5f); },		// location 4
			[this]() { return SpawnAgents(EntityTypes::Scorpio5, Vector3(124.f, 80.f, 24.f), 2, 1.f); },		// location 5

			// huge cave system
			[this]() { return SpawnAgents(EntityTypes::Scorpio6, Vector3(90.f, 55.f, 41.f), 8, 5.f); },			// location 2
			[this]() { return SpawnAgents(EntityTypes::Scorpio7, Vector3(135.f, 58.f, 48.f), 15, 7.5f); },		// location 3
			[this]() { return SpawnAgents(EntityTypes::Scorpio9, Vector3(80.f, 55.f, 5.5f), 10, 2.5f); },		// location 4
			[this]() { return SpawnAgents(EntityTypes::Scorpio10, Vector3(104.5f, 53.f, 5.f), 2, .5f); },		// location 5
			[this]() { return SpawnAgents(EntityTypes::Scorpio10, Vector3(109.5f, 53.f, 5.f), 2, .5f); },		// location 6
			[this]() { return SpawnAgents(EntityTypes::Scorpio10, Vector3(104.5f, 53.f, 5.f), 2, .5f); },		// location 7
			[this]() { return SpawnAgents(EntityTypes::Scorpio10, Vector3(119.f, 53.f, 5.f), 2, .5f); },		// location 8
		
		[this]() {
			m_musicPlayer = m_entityManager.CreateEntity();
			
			m_entityManager.AddComponent<AudioComponent>(m_musicPlayer) = {
				.assetID = AssetManager::Get().LoadAudio("Assets/Audio/RogueRobotsAmbience.wav", AssetLoadFlag::CPUMemory),
				.loopStart = 2.f,
				.loopEnd = 173.f,
				.shouldPlay = true,
				.loop = true,
			};
			return std::vector({ m_musicPlayer });
		},
		});

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
	/*****************************************************************************
	* 
	*				Room Teleport for MVP / internal test
	* 
	******************************************************************************/
	// room 3
	int s6 = 0;
	int s7 = 0;
	int s9 = 0;
	int s10 = 0;
	EntityManager::Get().Collect<AgentIdComponent, TransformComponent>().Do([&](AgentIdComponent& agent, TransformComponent& trans)
		{
			if (agent.type == EntityTypes::Scorpio6) ++s6;
			if (agent.type == EntityTypes::Scorpio7) ++s7;
			if (agent.type == EntityTypes::Scorpio9) ++s9;
			if (agent.type == EntityTypes::Scorpio10) ++s10;
			Vector3 pos = trans.GetPosition();
			if (pos.y < 50.f)
			{
				pos.y = -50.f;
				trans.SetPosition(pos);
			}
		});

	// room 2
	bool nextLevel = false;
	int s4 = 0;
	int s5 = 0;
	EntityManager::Get().Collect<AgentIdComponent, TransformComponent>().Do([&](AgentIdComponent& agent, TransformComponent& trans)
	{
		if (agent.type == EntityTypes::Scorpio4) ++s4;
		if (agent.type == EntityTypes::Scorpio5) ++s5;
		nextLevel = (s4 == 0) && (s5 == 0);
		Vector3 pos = trans.GetPosition();
		if (pos.y < 75.f)
		{
			pos.y = -50.f;
			trans.SetPosition(pos);
		}
	});
	if (nextLevel)
		EntityManager::Get().Collect<PlayersRoom2Component, NetworkPlayerComponent, DOG::TransformComponent>().Do(
			[&](entity e, PlayersRoom2Component&, NetworkPlayerComponent& player, DOG::TransformComponent& trans)
			{
				constexpr f32 spread = 2.8f;
				Vector3 offset = {
				spread * (player.playerId % 2) - (spread / 2.f),
				0,
				spread * (player.playerId / 2) - (spread / 2.f),
				};

				PhysicsEngine::FreePhysicsFromEntity(e);
				EntityManager::Get().RemoveComponent<CapsuleColliderComponent>(e);
				EntityManager::Get().RemoveComponent<RigidbodyComponent>(e);

				trans.SetPosition(Vector3(76.5f, 56.0f, 68.0f) - offset);

				m_entityManager.AddComponent<CapsuleColliderComponent>(e, e, 0.25f, 0.8f, true, 75.f);
				auto& rb = m_entityManager.AddComponent<RigidbodyComponent>(e, e);
				rb.ConstrainRotation(true, true, true);
				rb.disableDeactivation = true;
				rb.getControlOfTransform = true;

				EntityManager::Get().RemoveComponent<PlayersRoom2Component>(e);
				EntityManager::Get().AddComponent<PlayersRoom3Component>(e);
			});

	// room 1
	nextLevel = false;
	int s1 = 0;
	int s2 = 0;
	int s3 = 0;
	EntityManager::Get().Collect<AgentIdComponent, TransformComponent>().Do([&](AgentIdComponent& agent, TransformComponent& trans)
	{
		if (agent.type == EntityTypes::Scorpio1) ++s1;
		if (agent.type == EntityTypes::Scorpio2) ++s2;
		if (agent.type == EntityTypes::Scorpio3) ++s3;
		nextLevel = (s1 == 0) && (s2 == 0) && (s3 == 0);
		Vector3 pos = trans.GetPosition();
		if (pos.y < 70.f)
		{
			pos.y = -50.f;
			trans.SetPosition(pos);
		}
		});
	if (nextLevel)
		EntityManager::Get().Collect<PlayersRoom1Component, NetworkPlayerComponent, DOG::TransformComponent>().Do(
			[&](entity e, PlayersRoom1Component&, NetworkPlayerComponent& player, DOG::TransformComponent& trans)
			{
				constexpr f32 spread = 5.f;
				Vector3 offset = {
				spread * (player.playerId % 2) - (spread / 2.f),
				0,
				spread * (player.playerId / 2) - (spread / 2.f),
				};
				PhysicsEngine::FreePhysicsFromEntity(e);
				EntityManager::Get().RemoveComponent<CapsuleColliderComponent>(e);
				EntityManager::Get().RemoveComponent<RigidbodyComponent>(e);

				trans.SetPosition(Vector3(106.0f, 80.0f, 31.0f) - offset);

				m_entityManager.AddComponent<CapsuleColliderComponent>(e, e, 0.25f, 0.8f, true, 75.f);
				auto& rb = m_entityManager.AddComponent<RigidbodyComponent>(e, e);
				rb.ConstrainRotation(true, true, true);
				rb.disableDeactivation = true;
				rb.getControlOfTransform = true;

				EntityManager::Get().RemoveComponent<PlayersRoom1Component>(e);
				EntityManager::Get().AddComponent<PlayersRoom2Component>(e);
			});

	// room 0
	nextLevel = false;
	EntityManager::Get().Collect<PlayersRoom0Component, DOG::TransformComponent>().Do([&](PlayersRoom0Component&, DOG::TransformComponent& trans)
		{
			nextLevel = nextLevel || trans.GetPosition().y < 79.f;
		});
	if (nextLevel)
	{
		EntityManager::Get().Collect<PlayersRoom0Component, NetworkPlayerComponent, DOG::TransformComponent>().Do(
			[&](entity e, PlayersRoom0Component&, NetworkPlayerComponent& player, DOG::TransformComponent& trans)
			{
				constexpr f32 spread = 3.f;
				Vector3 offset = {
				spread * (player.playerId % 2) - (spread / 2.f),
				0,
				spread * (player.playerId / 2) - (spread / 2.f),
				};
				PhysicsEngine::FreePhysicsFromEntity(e);
				EntityManager::Get().RemoveComponent<CapsuleColliderComponent>(e);
				EntityManager::Get().RemoveComponent<RigidbodyComponent>(e);

				trans.SetPosition(Vector3(2.0f, 80.0f, 13.0f) - offset);

				m_entityManager.AddComponent<CapsuleColliderComponent>(e, e, 0.25f, 0.8f, true, 75.f);
				auto& rb = m_entityManager.AddComponent<RigidbodyComponent>(e, e);
				rb.ConstrainRotation(true, true, true);
				rb.disableDeactivation = true;
				rb.getControlOfTransform = true;

				EntityManager::Get().RemoveComponent<PlayersRoom0Component>(e);
				EntityManager::Get().AddComponent<PlayersRoom1Component>(e);
			});

		auto& audioComp = m_entityManager.GetComponent<AudioComponent>(m_musicPlayer);
		audioComp.assetID = AssetManager::Get().LoadAudio("Assets/Audio/RogueRobotsAction.wav", AssetLoadFlag::CPUMemory);
		audioComp.shouldPlay = true;
		audioComp.loopEnd = 306.f;
		audioComp.loopStart = 0.f;
	}

	// ImGui report
	EntityManager::Get().Collect<PlayersRoom0Component, ThisPlayer>().Do([&](PlayersRoom0Component&, ThisPlayer&)
		{
			ImGui::Text("Room 0");
		});
	EntityManager::Get().Collect<PlayersRoom1Component, ThisPlayer>().Do([&](PlayersRoom1Component&, ThisPlayer&)
		{
			ImGui::Text("Room 1");
			ImGui::Text(("Scorpio1: " + std::to_string(s1)).c_str());
			ImGui::Text(("Scorpio2: " + std::to_string(s2)).c_str());
			ImGui::Text(("Scorpio3: " + std::to_string(s3)).c_str());
		});
	EntityManager::Get().Collect<PlayersRoom2Component, ThisPlayer>().Do([&](PlayersRoom2Component&, ThisPlayer&)
		{
			ImGui::Text("Room 2");
			ImGui::Text(("Scorpio4: " + std::to_string(s4)).c_str());
			ImGui::Text(("Scorpio5: " + std::to_string(s5)).c_str());
		});
	EntityManager::Get().Collect<PlayersRoom3Component, ThisPlayer>().Do([&](PlayersRoom3Component&, ThisPlayer&)
		{
			ImGui::Text("Room 3");
			ImGui::Text(("Scorpio1: " + std::to_string(s1)).c_str());
			ImGui::Text(("Scorpio2: " + std::to_string(s2)).c_str());
			ImGui::Text(("Scorpio3: " + std::to_string(s3)).c_str());
			ImGui::Text(("Scorpio3: " + std::to_string(s3)).c_str());
		});

	/*******************************  END MVP ************************************/


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

void GameLayer::RespawnDeadPlayer(DOG::entity e)
{
	if (!m_entityManager.HasComponent<PlayerAliveComponent>(e))
	{
		m_entityManager.AddComponent<PlayerAliveComponent>(e);
	}

	LuaMain::GetScriptManager()->AddScript(e, "Gun.lua");
	auto gunScriptData = LuaMain::GetScriptManager()->GetScript(e, "Gun.lua");
	LuaTable t0(gunScriptData.scriptTable, true);
	t0.CallFunctionOnTable("OnStart");

	LuaMain::GetScriptManager()->AddScript(e, "PassiveItemSystem.lua");
	auto passiveItemScriptData = LuaMain::GetScriptManager()->GetScript(e, "PassiveItemSystem.lua");
	LuaTable t1(passiveItemScriptData.scriptTable, true);
	t1.CallFunctionOnTable("OnStart");

	LuaMain::GetScriptManager()->AddScript(e, "ActiveItemSystem.lua");
	auto activeItemScriptData = LuaMain::GetScriptManager()->GetScript(e, "ActiveItemSystem.lua");
	LuaTable t2(activeItemScriptData.scriptTable, true);
	t2.CallFunctionOnTable("OnStart");

	if (m_entityManager.HasComponent<ThisPlayer>(e))
	{
		auto& stats = m_entityManager.GetComponent<PlayerStatsComponent>(e);
		stats.health = stats.maxHealth;
	}
	
	auto& controller = m_entityManager.GetComponent<PlayerControllerComponent>(e);
	m_entityManager.DeferredEntityDestruction(controller.debugCamera);
	controller.debugCamera = 0;
}

void GameLayer::KillPlayer(DOG::entity e)
{
	m_entityManager.RemoveComponent<PlayerAliveComponent>(e);
	LuaMain::GetScriptManager()->RemoveScript(e, "Gun.lua");
	LuaMain::GetScriptManager()->RemoveScript(e, "PassiveItemSystem.lua");
	LuaMain::GetScriptManager()->RemoveScript(e, "ActiveItemSystem.lua");
	m_entityManager.RemoveComponent<ScriptComponent>(e);

	auto& controller = m_entityManager.GetComponent<PlayerControllerComponent>(e);
	controller.debugCamera = m_mainScene->CreateEntity();

	m_entityManager.AddComponent<TransformComponent>(controller.debugCamera)
		.worldMatrix = m_entityManager.GetComponent<TransformComponent>(controller.cameraEntity);

	m_entityManager.AddComponent<CameraComponent>(controller.debugCamera).isMainCamera = true;
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


	EntityManager::Get().Collect<TransformComponent, RigidbodyComponent>().Do([](entity id, TransformComponent& transform, RigidbodyComponent&)
		{
			if (Vector3 pos = transform.GetPosition(); pos.y < -20.0f)
			{
				if (EntityManager::Get().HasComponent<PlayerStatsComponent>(id))
					EntityManager::Get().GetComponent<PlayerStatsComponent>(id).health = 0;
				if (EntityManager::Get().HasComponent<AgentHPComponent>(id))
					EntityManager::Get().GetComponent<AgentHPComponent>(id).hp = 0;
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
	luaInterface.AddFunction<EntityInterface, &EntityInterface::ModifyAnimationComponent>("ModifyAnimationComponent");


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

std::vector<entity> GameLayer::LoadLevel()
{
	float blockDim = 5.0f;

	std::string line;
	//testRooms
	//Tunnels
	//showOff
	//std::ifstream inputFile("..\\Offline-Tools\\PCG\\showOff_generatedLevel.txt");
	std::ifstream inputFile("..\\Offline-Tools\\PCG\\Tunnels_generatedLevel.txt");

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
						m_entityManager.AddComponent<TransformComponent>(blockEntity,
							Vector3(x * blockDim, y * blockDim, z * blockDim),
							Vector3(piDiv2, blockRot * piDiv2 - piDiv2, 0.0f),
							Vector3(xFlip, -yFlip, 1.0f));

						m_entityManager.AddComponent<ModularBlockComponent>(blockEntity);
						m_entityManager.AddComponent<MeshColliderComponent>(blockEntity,
							blockEntity, 
							aManager.LoadModelAsset("Assets/Models/ModularBlocks/" + blockName + "_Col.fbx", (DOG::AssetLoadFlag)((DOG::AssetLoadFlag::Async) | (DOG::AssetLoadFlag)(DOG::AssetLoadFlag::CPUMemory | DOG::AssetLoadFlag::GPUMemory))),
							localMeshColliderScale,
							false);		// Set this to true if you want to see colliders only in wireframe
						m_entityManager.AddComponent<ShadowReceiverComponent>(blockEntity);
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
	EntityManager::Get().Collect<InputController, AnimationComponent, ThisPlayer>().Do([&](InputController& inputC, AnimationComponent& ac, ThisPlayer&)
		{
			if (key == DOG::Key::W)
				ac.input[0] = 1;
			if (key == DOG::Key::A)
				ac.input[1] = 1;
			if (key == DOG::Key::S)
				ac.input[3] = 1;
			if (key == DOG::Key::D)
				ac.input[2] = 1;
		});
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
	EntityManager::Get().Collect<InputController, AnimationComponent, ThisPlayer>().Do([&](InputController& inputC, AnimationComponent& ac, ThisPlayer&)
		{
			if (key == DOG::Key::W)
				ac.input[0] = 0;
			if (key == DOG::Key::A)
				ac.input[1] = 0;
			if (key == DOG::Key::S)
				ac.input[3] = 0;
			if (key == DOG::Key::D)
				ac.input[2] = 0;
		});
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

std::vector<entity> GameLayer::SpawnPlayers(const Vector3& pos, u8 playerCount, f32 spread)
{
	ASSERT(playerCount > 0, "Need to at least spawn ThisPlayer. I.e. playerCount has to exceed 0");
	ASSERT(playerCount <= MAX_PLAYER_COUNT, "No more than 4 players can be spawned. I.e. playerCount can't exceed 4");

	auto* scriptManager = LuaMain::GetScriptManager();
	//// Add persistent material prefab lua
	//{
	//	entity e = m_entityManager.CreateEntity();
	//	m_entityManager.AddComponent<TransformComponent>(e);
	//	scriptManager->AddScript(e, "MaterialPrefabs.lua");
	//}

	//LuaMain::GetGlobal().
	scriptManager->RunLuaFile("MaterialPrefabs.lua");
	LuaMain::GetGlobal()->GetTable("MaterialPrefabs").CallFunctionOnTable("OnStart");

	auto& am = DOG::AssetManager::Get();
	//m_playerModels[0] = am.LoadModelAsset("Assets/Models/Temporary_Assets/magenta_cube.glb");
	
	m_playerModels[0] = am.LoadModelAsset("Assets/Models/Players/Test/Red/player_red.gltf");
	m_playerModels[3] = am.LoadModelAsset("Assets/Models/Players/Test/Green/player_green.gltf");
	m_playerModels[1] = am.LoadModelAsset("Assets/Models/Players/Test/Blue/player_blue.gltf", (DOG::AssetLoadFlag)((DOG::AssetLoadFlag::Async) | (DOG::AssetLoadFlag)(DOG::AssetLoadFlag::GPUMemory | DOG::AssetLoadFlag::CPUMemory)));
	m_playerModels[2] = am.LoadModelAsset("Assets/Models/Players/Test/Yellow/player_yellow.gltf");
	std::vector<entity> players;
	for (auto i = 0; i < playerCount; ++i)
	{
		entity playerI = players.emplace_back(m_entityManager.CreateEntity());
		Vector3 offset = {
			spread * (i % 2) - (spread / 2.f),
			0,
			spread * (i / 2) - (spread / 2.f),
		};
		auto& tf = m_entityManager.AddComponent<TransformComponent>(playerI, pos - offset);
		m_entityManager.AddComponent<ModelComponent>(playerI, m_playerModels[i]);
		m_entityManager.AddComponent<CapsuleColliderComponent>(playerI, playerI, 0.25f, 2.75f, true, 75.f);
		auto& rb = m_entityManager.AddComponent<RigidbodyComponent>(playerI, playerI);
		rb.ConstrainRotation(true, true, true);
		rb.disableDeactivation = true;
		rb.getControlOfTransform = true;

		m_entityManager.AddComponent<PlayerStatsComponent>(playerI);
		m_entityManager.AddComponent<PlayerControllerComponent>(playerI);
		m_entityManager.AddComponent<NetworkPlayerComponent>(playerI).playerId = static_cast<i8>(i);
		m_entityManager.AddComponent<InputController>(playerI);
		m_entityManager.AddComponent<ShadowReceiverComponent>(playerI);
		m_entityManager.AddComponent<PlayerAliveComponent>(playerI);
		scriptManager->AddScript(playerI, "Gun.lua");
		scriptManager->AddScript(playerI, "PassiveItemSystem.lua");
		scriptManager->AddScript(playerI, "ActiveItemSystem.lua");

		
		if (i == 0) // Only for this player
		{
			m_entityManager.AddComponent<ThisPlayer>(playerI);
			m_entityManager.AddComponent<AudioListenerComponent>(playerI);
			/*auto& ac = m_entityManager.AddComponent<AnimationComponent>(playerI);
			ac.animatorID = i;
			ac.rigID = 0;*/
		}
		else
		{
			auto& ac = m_entityManager.AddComponent<AnimationComponent>(playerI);
			ac.rigID = 0;
			ac.animatorID = i;
			m_entityManager.AddComponent<OnlinePlayer>(playerI);
		}
	}
	return players;
}

std::vector<entity> GameLayer::AddFlashlightsToPlayers(const std::vector<entity>& players)
{
	std::vector<entity> flashlights;
	for (auto i = 0; i < players.size(); ++i)
	{
		auto& playerTransformComponent = m_entityManager.GetComponent<TransformComponent>(players[i]);

		entity flashLightEntity = m_entityManager.CreateEntity();
		auto& tc = m_entityManager.AddComponent<DOG::TransformComponent>(flashLightEntity);
		tc.SetPosition(playerTransformComponent.GetPosition() + DirectX::SimpleMath::Vector3(0.2f, 0.2f, 0.0f));

		auto up = tc.worldMatrix.Up();
		up.Normalize();

		auto& cc = m_entityManager.AddComponent<DOG::CameraComponent>(flashLightEntity);
		cc.isMainCamera = false;
		cc.viewMatrix = DirectX::XMMatrixLookAtLH
		(
			{ tc.GetPosition().x, tc.GetPosition().y, tc.GetPosition().z },
			{ tc.GetPosition().x + tc.GetForward().x, tc.GetPosition().y + tc.GetForward().y, tc.GetPosition().z + tc.GetForward().z },
			{ up.x, up.y, up.z }
		);

		auto dd = DOG::SpotLightDesc();
		dd.color = { 1.0f, 1.0f, 1.0f };
		dd.direction = tc.GetForward();
		dd.strength = 0.6f;
		dd.cutoffAngle = 35.0f;

		auto lh = DOG::LightManager::Get().AddSpotLight(dd, DOG::LightUpdateFrequency::PerFrame);

		auto& slc = m_entityManager.AddComponent<DOG::SpotLightComponent>(flashLightEntity);
		slc.color = dd.color;
		slc.direction = tc.GetForward();
		slc.strength = dd.strength;
		slc.cutoffAngle = dd.cutoffAngle;
		slc.handle = lh;
		slc.owningPlayer = players[i];

		float fov = ((slc.cutoffAngle + 0.1f) * 2.0f) * DirectX::XM_PI / 180.f;
		cc.projMatrix = DirectX::XMMatrixPerspectiveFovLH(fov, 1, 800.f, 0.1f);

		m_entityManager.AddComponent<DOG::ShadowCasterComponent>(flashLightEntity);

		if (i == 0) // Only for this/main player
			slc.isMainPlayerSpotlight = true;
		else
			slc.isMainPlayerSpotlight = false;

		flashlights.push_back(flashLightEntity);
	}
	return flashlights;
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
	m_isCheating = m_godModeCheat || m_unlimitedAmmoCheat || m_noClipCheat;

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

	entity player = GetPlayer();
	assert(player != NULL_ENTITY && EntityManager::Get().Exists(player));
	
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

			// Commented out until a propper debug camera is implemented.
			/*bool checkboxMainScene = m_mainScene != nullptr;
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
			}*/

			std::vector<entity> players;
			EntityManager::Get().Collect<PlayerStatsComponent>().Do([&](entity e, PlayerStatsComponent&)
				{
					players.push_back(e);
				});

			auto&& playerToString = [&](entity e) -> std::pair<std::string, std::string>
			{
				auto& stats = EntityManager::Get().GetComponent<PlayerStatsComponent>(e);
				std::string str2 = "hp: " + std::to_string(stats.health);
				auto pos = EntityManager::Get().GetComponent<DOG::TransformComponent>(e).GetPosition();
				str2 += "   pos: (" + std::to_string(static_cast<int>(std::round(pos.x))) + ", " + std::to_string(static_cast<int>(std::round(pos.y))) + ", " + std::to_string(static_cast<int>(std::round(pos.z))) + ")";
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
