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
	m_entityManager.RegisterSystem(std::make_unique<PickupLerpAnimationSystem>());
	m_entityManager.RegisterSystem(std::make_unique<MVPPickupItemInteractionSystem>());
	m_entityManager.RegisterSystem(std::make_unique<PlayerMovementSystem>());
	m_entityManager.RegisterSystem(std::make_unique<PlayerJumpRefreshSystem>());

	m_entityManager.RegisterSystem(std::make_unique<MVPFlashlightStateSystem>());
	m_entityManager.RegisterSystem(std::make_unique<MVPRenderPickupItemUIText>());
	m_entityManager.RegisterSystem(std::make_unique<PickUpTranslateToPlayerSystem>());
	m_entityManager.RegisterSystem(std::make_unique<MVPRenderAmmunitionTextSystem>());
	m_entityManager.RegisterSystem(std::make_unique<MVPRenderReloadHintTextSystem>());
	m_entityManager.RegisterSystem(std::make_unique<DeleteNetworkSync>());
	m_agentManager = new AgentManager();
	m_nrOfPlayers = 1;
	m_networkStatus = NetworkStatus::Offline;

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
	Window::SetFont(m_imguiFont);
}

GameLayer::~GameLayer()
{
	delete m_agentManager;
}

void GameLayer::OnAttach()
{
	DOG::ImGuiMenuLayer::RegisterDebugWindow("GameManager", std::bind(&GameLayer::GameLayerDebugMenu, this, std::placeholders::_1), true, std::make_pair(DOG::Key::LCtrl, DOG::Key::G));
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
			m_gameState = GameState::StartPlaying;
			break;
		case GameState::Lost:
			CloseMainScene();
			m_gameState = GameState::StartPlaying;
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

	if (m_networkStatus != NetworkStatus::Offline)
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
	std::string luaEventName = std::string("ItemPickup") + std::to_string(e);
	LuaMain::GetEventSystem()->RemoveEvent(luaEventName);
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

void GameLayer::ToggleFlashlight()
{
	m_entityManager.Collect<DOG::SpotLightComponent>().Do([](DOG::SpotLightComponent& slc)
		{
			if (slc.isMainPlayerSpotlight)
			{
				if (slc.strength == 0.6f)
					slc.strength = 0.0f;
				else
					slc.strength = 0.6f;
			}
		});
}

void GameLayer::PickUpItem()
{
	m_entityManager.Collect<EligibleActiveItemComponent>().Do([this](DOG::entity player, EligibleActiveItemComponent& eligiblePickUp)
		{
			if (m_entityManager.HasComponent<ActiveItemComponent>(player))
			{
				m_entityManager.RemoveComponent<ActiveItemComponent>(player);
			}
			m_entityManager.AddComponent<ActiveItemComponent>(player).type = eligiblePickUp.type;

			m_entityManager.RemoveComponent<EligibleActiveItemComponent>(player);

			if (!m_entityManager.HasComponent<PickedUpItemComponent>(eligiblePickUp.activeItemEntity))
			{
				m_entityManager.AddComponent<PickedUpItemComponent>(eligiblePickUp.activeItemEntity);
				auto& ac = m_entityManager.GetComponent<DOG::PickupLerpAnimateComponent>(eligiblePickUp.activeItemEntity);
				ac.origin = m_entityManager.GetComponent<DOG::TransformComponent>(eligiblePickUp.activeItemEntity).GetPosition();
				ac.target = m_entityManager.GetComponent<DOG::TransformComponent>(player).GetPosition();
				ac.target.y -= 1.0f;
			}
		});

	m_entityManager.Collect<EligibleBarrelComponent>().Do([this](DOG::entity player, EligibleBarrelComponent& eligiblePickUp)
		{
			m_entityManager.RemoveComponent<EligibleBarrelComponent>(player);

			if (!m_entityManager.HasComponent<PickedUpItemComponent>(eligiblePickUp.barrelComponentEntity))
			{
				m_entityManager.AddComponent<PickedUpItemComponent>(eligiblePickUp.barrelComponentEntity);
				auto& ac = m_entityManager.GetComponent<DOG::PickupLerpAnimateComponent>(eligiblePickUp.barrelComponentEntity);
				ac.origin = m_entityManager.GetComponent<DOG::TransformComponent>(eligiblePickUp.barrelComponentEntity).GetPosition();
				ac.target = m_entityManager.GetComponent<DOG::TransformComponent>(player).GetPosition();
				ac.target.y -= 1.0f;
			}
		});

	m_entityManager.Collect<EligiblePassiveItemComponent>().Do([this](DOG::entity player, EligiblePassiveItemComponent& eligiblePickUp)
		{
			m_entityManager.RemoveComponent<EligiblePassiveItemComponent>(player);

			if (!m_entityManager.HasComponent<PickedUpItemComponent>(eligiblePickUp.passiveItemEntity))
			{
				m_entityManager.AddComponent<PickedUpItemComponent>(eligiblePickUp.passiveItemEntity);
				auto& ac = m_entityManager.GetComponent<DOG::PickupLerpAnimateComponent>(eligiblePickUp.passiveItemEntity);
				ac.origin = m_entityManager.GetComponent<DOG::TransformComponent>(eligiblePickUp.passiveItemEntity).GetPosition();
				ac.target = m_entityManager.GetComponent<DOG::TransformComponent>(player).GetPosition();
				ac.target.y -= 1.0f;
			}
		});

	m_entityManager.Collect<EligibleMagazineModificationComponent>().Do([this](DOG::entity player, EligibleMagazineModificationComponent& eligiblePickUp)
		{
			m_entityManager.RemoveComponent<EligibleMagazineModificationComponent>(player);

			if (!m_entityManager.HasComponent<PickedUpItemComponent>(eligiblePickUp.magazineModificationEntity))
			{
				m_entityManager.AddComponent<PickedUpItemComponent>(eligiblePickUp.magazineModificationEntity);
				auto& ac = m_entityManager.GetComponent<DOG::PickupLerpAnimateComponent>(eligiblePickUp.magazineModificationEntity);
				ac.origin = m_entityManager.GetComponent<DOG::TransformComponent>(eligiblePickUp.magazineModificationEntity).GetPosition();
				ac.target = m_entityManager.GetComponent<DOG::TransformComponent>(player).GetPosition();
				ac.target.y -= 1.0f;
			}
		});
}

void GameLayer::OnEvent(DOG::IEvent& event)
{
	using namespace DOG;
	switch (event.GetEventType())
	{
	case EventType::LeftMouseButtonPressedEvent:
	{
		EntityManager::Get().Collect<InputController, ThisPlayer>().Do([&](InputController& inputC, ThisPlayer&)
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
		if (EVENT(KeyPressedEvent).key == DOG::Key::E)
		{
			PickUpItem();
		}
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

void GameLayer::UpdateLobby()
{
	bool inLobby = m_gameState == GameState::Lobby;
	if (ImGui::Begin("Lobby", &inLobby))
	{
		switch (m_networkStatus)
		{
		case NetworkStatus::Offline:
		{
			ImGui::Text("Host to host, join to join, play to play offline");
			if (ImGui::Button("Host"))
			{
				m_networkStatus = NetworkStatus::HostLobby;

			}
			if (ImGui::Button("Join"))
			{
				m_networkStatus = NetworkStatus::JoinLobby;
			}
			if (ImGui::Button("Play"))
			{
				inLobby = false;
			}
			break;
		}
		case NetworkStatus::HostLobby:
		{
			ImGui::Text("Press host to host on any other computer then thoose defined");
			if (ImGui::Button("Host"))
			{
				m_netCode.SetMulticastAdress("239.255.255.0");
				if (m_netCode.Host())
				{
					m_networkStatus = NetworkStatus::Hosting;
				}
			}
			if (ImGui::Button("Host Sam"))
			{
				m_netCode.SetMulticastAdress("239.255.255.1");
				if (m_netCode.Host())
				{
					m_networkStatus = NetworkStatus::Hosting;
				}
			}
			if (ImGui::Button("Host Filip"))
			{
				m_netCode.SetMulticastAdress("239.255.255.2");
				if (m_netCode.Host())
				{
					m_networkStatus = NetworkStatus::Hosting;
				}
			}
			if (ImGui::Button("Host Nad"))
			{
				m_netCode.SetMulticastAdress("239.255.255.3");
				if (m_netCode.Host())
				{
					m_networkStatus = NetworkStatus::Hosting;
				}
			}
			if (ImGui::Button("Host Axel"))
			{
				m_netCode.SetMulticastAdress("239.255.255.4");
				if (m_netCode.Host())
				{
					m_networkStatus = NetworkStatus::Hosting;
				}
			}
			if (ImGui::Button("Host Ove"))
			{
				m_netCode.SetMulticastAdress("239.255.255.5");
				if (m_netCode.Host())
				{
					m_networkStatus = NetworkStatus::Hosting;
				}
			}
			if (ImGui::Button("Host Gunnar"))
			{
				m_netCode.SetMulticastAdress("239.255.255.6");
				if (m_netCode.Host())
				{
					m_networkStatus = NetworkStatus::Hosting;
				}
			}
			if (ImGui::Button("Host Emil F"))
			{
				m_netCode.SetMulticastAdress("239.255.255.7");
				if (m_netCode.Host())
				{
					m_networkStatus = NetworkStatus::Hosting;
				}
			}
			if (ImGui::Button("Host Jonatan"))
			{
				m_netCode.SetMulticastAdress("239.255.255.8");
				if (m_netCode.Host())
				{
					m_networkStatus = NetworkStatus::Hosting;
				}
			}
			break;
		}
		case NetworkStatus::Hosting:
		{
			char ip[64];
			strcpy_s(ip, m_netCode.GetIpAdress().c_str());
			ImGui::Text("Nr of players connected: %d", m_netCode.GetNrOfPlayers());
			ImGui::Text("Youre ip adress: %s", ip);
			if (ImGui::Button("Play"))
			{
				m_nrOfPlayers = m_netCode.Play();
				inLobby = false;
			}
			break;
		}
		case NetworkStatus::JoinLobby:
		{
			static char input[64]{};
			ImGui::Text("Write ip address and then join or select premade lobby");
			ImGui::InputText("Ip", input, 64);

			if (ImGui::Button("Join"))
			{
				m_netCode.SetMulticastAdress("239.255.255.0");
				if (m_netCode.Join(input))
				{
					m_networkStatus = NetworkStatus::Joining;
				}
			}
			if (ImGui::Button("Join Sam"))
			{
				m_netCode.SetMulticastAdress("239.255.255.1");
				input[0] = 'a';
				if (m_netCode.Join(input))
				{
					m_networkStatus = NetworkStatus::Joining;
				}
			}
			if (ImGui::Button("Join Filip"))
			{
				m_netCode.SetMulticastAdress("239.255.255.2");
				input[0] = 'b';
				if (m_netCode.Join(input))
				{
					m_networkStatus = NetworkStatus::Joining;
				}
			}
			if (ImGui::Button("Join Nad"))
			{
				m_netCode.SetMulticastAdress("239.255.255.3");
				input[0] = 'c';
				if (m_netCode.Join(input))
				{
					m_networkStatus = NetworkStatus::Joining;
				}
			}
			if (ImGui::Button("Join Axel"))
			{
				m_netCode.SetMulticastAdress("239.255.255.4");
				input[0] = 'd';
				if (m_netCode.Join(input))
				{
					m_networkStatus = NetworkStatus::Joining;
				}
			}
			if (ImGui::Button("Join Ove"))
			{
				m_netCode.SetMulticastAdress("239.255.255.5");
				input[0] = 'e';
				if (m_netCode.Join(input))
				{
					m_networkStatus = NetworkStatus::Joining;
				}
			}
			if (ImGui::Button("Join Gunnar"))
			{
				m_netCode.SetMulticastAdress("239.255.255.6");
				input[0] = 'f';
				if (m_netCode.Join(input))
				{
					m_networkStatus = NetworkStatus::Joining;
				}
			}
			if (ImGui::Button("Join Emil F"))
			{
				m_netCode.SetMulticastAdress("239.255.255.7");
				input[0] = 'g';
				if (m_netCode.Join(input))
				{
					m_networkStatus = NetworkStatus::Joining;
				}
			}
			if (ImGui::Button("Join Jonatan"))
			{
				m_netCode.SetMulticastAdress("239.255.255.8");
				input[0] = 'h';
				if (m_netCode.Join(input))
				{
					m_networkStatus = NetworkStatus::Joining;
				}
			}
			break;
		}
		case NetworkStatus::Joining:
		{
			m_nrOfPlayers = m_netCode.GetNrOfPlayers();
			ImGui::Text("Nr of players connected: %d", m_netCode.GetNrOfPlayers());
			ImGui::Text("Waiting for Host to press Play...");
			inLobby = m_netCode.IsLobbyAlive();
			break;
		}
		default:
			break;
		}
		if (!inLobby)
			m_gameState = GameState::StartPlaying;
	}
	ImGui::End();
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
	luaInterface.AddFunction<EntityInterface, &EntityInterface::RemoveComponent>("RemoveComponent");
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
	luaInterface.AddFunction<EntityInterface, &EntityInterface::GetActiveType>("GetActiveType");
	luaInterface.AddFunction<EntityInterface, &EntityInterface::GetBarrelType>("GetBarrelType");
	luaInterface.AddFunction<EntityInterface, &EntityInterface::GetModificationType>("GetModificationType");
	luaInterface.AddFunction<EntityInterface, &EntityInterface::GetAmmoCapacityForBarrelType>("GetAmmoCapacityForBarrelType");
	luaInterface.AddFunction<EntityInterface, &EntityInterface::GetAmmoCountPerPickup>("GetAmmoCountPerPickup");
	luaInterface.AddFunction<EntityInterface, &EntityInterface::UpdateMagazine>("UpdateMagazine");
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
			if (key == DOG::Key::T)
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
			if (key == DOG::Key::T)
				inputC.switchBarrelComp = false;
			if (key == DOG::Key::M)
				inputC.switchMagazineComp = false;
			if (key == DOG::Key::G)
				inputC.activateActiveItem = false;
			if (key == DOG::Key::R)
				inputC.reload = false;
			if (key == DOG::Key::H)
				inputC.toggleDebug = false;
			if (key == DOG::Key::C)
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
	auto ui = UI::Get();
	auto hbar = ui->GetUI<UIHealthBar>(hID);
	EntityManager& em = EntityManager::Get();
	em.Collect<PlayerStatsComponent, ThisPlayer>().Do([&](PlayerStatsComponent& stats, ThisPlayer&)
		{
			hbar->SetBarValue(stats.health / stats.maxHealth);
		});
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
					Vector3 v = EntityManager::Get().GetComponent<TransformComponent>(players[i]).GetPosition();
					std::string s = "x:" + std::to_string(v.x) + " y:" + std::to_string(v.y) + " z:" + std::to_string(v.z);
					ImGui::Text(s.c_str());
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