#include <DOGEngine.h>
#include "GameLayer.h"
#include "TestScene.h"
#include "OldDefaultScene.h"
#include "PCGLevelScenes.h"
#include "LightScene.h"
#include "SimpleAnimationSystems.h"
#include "ExplosionSystems.h"
#include "TurretSystems.h"
#include "HomingMissileSystem.h"
#include "PcgLevelLoader.h"
#include "PrefabInstantiatorFunctions.h"
#include "ItemManager/ItemManager.h"
#include "TestScenes/ParticleScene.h"
#include "PlayerManager/PlayerManager.h"
#include "Pathfinder/Pathfinder.h"
#include "HeartbeatTrackerSystem.h"
#include "InGameMenu.h"
#include "GoalRadarSystem.h"
#include "MusicSystems.h"

using namespace DOG;
using namespace DirectX;
using namespace DirectX::SimpleMath;

NetworkStatus GameLayer::s_networkStatus = NetworkStatus::Offline;
GameState GameLayer::m_gameState = GameState::Initializing;


GameLayer::GameLayer() noexcept
	: Layer("Game layer"), m_entityManager{ DOG::EntityManager::Get() }
{
	LuaMain::GetScriptManager()->SortOrderScripts();
	//Do startup of lua
	LuaMain::GetScriptManager()->RunLuaFile("LuaStartUp.lua");
	//Register Lua interfaces
	RegisterLuaInterfaces();
	
	InGameMenu::Initialize(
		[]() {
			InGameMenu::Close();
			Window::SetCursorMode(CursorMode::Confined);
			EntityManager::Get().Collect<InputController, ThisPlayer>().Do([&](InputController& inputC, ThisPlayer&)
				{
					inputC.toggleMoveView = true;
				});
		},
		[gameLayer = this]() {
			gameLayer->m_gameState = GameState::ExitingToMainMenu;
		},
		[]() {
			Window::CloseWindow();
		}
	);

	m_entityManager.RegisterSystem(std::make_unique<ScuffedSceneGraphSystem>());
	m_entityManager.RegisterSystem(std::make_unique<SetFlashLightToBoneSystem>());
	m_entityManager.RegisterSystem(std::make_unique<DoorOpeningSystem>());
	m_entityManager.RegisterSystem(std::make_unique<LerpAnimationSystem>());
	m_entityManager.RegisterSystem(std::make_unique<LerpColorSystem>());
	m_entityManager.RegisterSystem(std::make_unique<MVPFlashlightMoveSystem>());
	m_entityManager.RegisterSystem(std::make_unique<HomingMissileTargetingSystem>());
	m_entityManager.RegisterSystem(std::make_unique<HomingMissileSystem>());
	m_entityManager.RegisterSystem(std::make_unique<HomingMissileImpacteSystem>());
	m_entityManager.RegisterSystem(std::make_unique<TurretTargetingSystem>());
	m_entityManager.RegisterSystem(std::make_unique<TurretShootingSystem>());
	m_entityManager.RegisterSystem(std::make_unique<TurretProjectileSystem>());
	m_entityManager.RegisterSystem(std::make_unique<TurretProjectileHitSystem>());
	m_entityManager.RegisterSystem(std::make_unique<LaserShootSystem>());
	m_entityManager.RegisterSystem(std::make_unique<LaserBeamSystem>());
	m_entityManager.RegisterSystem(std::make_unique<LaserBeamVFXSystem>());
	m_entityManager.RegisterSystem(std::make_unique<LaserBulletCollisionSystem>());

	m_entityManager.RegisterSystem(std::make_unique<DespawnSystem>());
	m_entityManager.RegisterSystem(std::make_unique<TimedDestructionSystem>());
	m_entityManager.RegisterSystem(std::make_unique<ExplosionSystem>());
	m_entityManager.RegisterSystem(std::make_unique<ExplosionEffectSystem>());
	m_entityManager.RegisterSystem(std::make_unique<PickupLerpAnimationSystem>());
	m_entityManager.RegisterSystem(std::make_unique<PickupItemInteractionSystem>());
	m_entityManager.RegisterSystem(std::make_unique<PlayerMovementSystem>());
	m_entityManager.RegisterSystem(std::make_unique<PlayerJumpRefreshSystem>());

	m_entityManager.RegisterSystem(std::make_unique<MVPFlashlightStateSystem>());
	m_entityManager.RegisterSystem(std::make_unique<MVPRenderPickupItemUIText>());
	m_entityManager.RegisterSystem(std::make_unique<PickUpTranslateToPlayerSystem>());
	m_entityManager.RegisterSystem(std::make_unique<MVPRenderAmmunitionTextSystem>());
	m_entityManager.RegisterSystem(std::make_unique<MVPRenderReloadHintTextSystem>());
	m_entityManager.RegisterSystem(std::make_unique<RenderMiscComponentText>());
	m_entityManager.RegisterSystem(std::make_unique<CleanupItemInteractionSystem>());
	m_entityManager.RegisterSystem(std::make_unique<CleanupPlayerStateSystem>());
	m_entityManager.RegisterSystem(std::make_unique<PlayerHit>());
	m_entityManager.RegisterSystem(std::make_unique<PlaceHolderDeathUISystem>());
	m_entityManager.RegisterSystem(std::make_unique<SpectateSystem>());
	m_entityManager.RegisterSystem(std::make_unique<HeartbeatTrackerSystem>());
	m_entityManager.RegisterSystem(std::make_unique<ReviveSystem>());
	m_entityManager.RegisterSystem(std::make_unique<UpdateSpectatorQueueSystem>());
	m_entityManager.RegisterSystem(std::make_unique<GlowStickSystem>());
	m_entityManager.RegisterSystem(std::make_unique<DeferredSetIgnoreCollisionCheckSystem>());
	m_entityManager.RegisterSystem(std::make_unique<PlayerUseEquipmentSystem>());
	m_entityManager.RegisterSystem(std::make_unique<GoalRadarSystem>());
	m_entityManager.RegisterSystem(std::make_unique<RemoveBulletComponentSystem>());

	m_entityManager.RegisterSystem(std::make_unique<WeaponPointLightSystem>());
	m_entityManager.RegisterSystem(std::make_unique<PlayMusicSystem>());

	m_entityManager.RegisterSystem(std::make_unique<DeleteNetworkSync>());
	m_nrOfPlayers = 1;

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
}

void GameLayer::OnAttach()
{
	DOG::ImGuiMenuLayer::RegisterDebugWindow("GameManager", std::bind(&GameLayer::GameLayerDebugMenu, this, std::placeholders::_1), true, std::make_pair(DOG::Key::LCtrl, DOG::Key::G));
	DOG::ImGuiMenuLayer::RegisterDebugWindow("Cheats", std::bind(&GameLayer::CheatDebugMenu, this, std::placeholders::_1));
}

void GameLayer::OnDetach()
{
	DOG::ImGuiMenuLayer::UnRegisterDebugWindow("GameManager");
	DOG::ImGuiMenuLayer::UnRegisterDebugWindow("Cheats");
	m_testScene.reset();
	m_testScene = nullptr;

	m_lightScene.reset();
	m_lightScene = nullptr;

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
		case GameState::MainMenu:
			m_entityManager.Collect<MusicPlayer>().Do([&](MusicPlayer& musicPlayer) {musicPlayer.inMainMenu = true; });
			break;
		case GameState::Initializing:
			m_gameState = GameState::MainMenu;
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
		{
			EntityManager::Get().Collect<InputController, ThisPlayer>().Do([&](InputController& inputC, ThisPlayer&)
				{
					inputC.winStatus = true;
				});
			UpdateGame();

			auto winText = DOG::UI::Get()->GetUI<UILabel>(lWinTextID);
			winText->SetText(std::wstring(L"Congratulations!\nYou escaped!"));

			m_nrOfFramesToWait--;
			if (m_nrOfFramesToWait <= 0)
			{
				m_gameState = GameState::ExitingToMainMenu;
				winText->SetText(std::wstring(L""));
				UI::Get()->ChangeUIscene(WinScreenID);
				EntityManager::Get().Collect<InputController, NetworkPlayerComponent>().Do([&](InputController& inputC, NetworkPlayerComponent& nPC)
					{
						if (nPC.playerId == 0)
						{
							auto redPlayer = DOG::UI::Get()->GetUI<UILabel>(lredScoreWinID);
							redPlayer->SetText(std::wstring(L"Red player kill score: ") + std::to_wstring(inputC.killScore) + std::wstring(L"\nDamage done: ") + std::to_wstring((int)inputC.damageDoneToEnemies)
								+ std::wstring(L"\nTeam Damage taken: ") + std::to_wstring((int)inputC.teamDamageTaken));
						}
						else if (nPC.playerId == 1)
						{
							auto bluePlayer = DOG::UI::Get()->GetUI<UILabel>(lblueScoreWinID);
							bluePlayer->SetText(std::wstring(L"Blue player kill score: ") + std::to_wstring(inputC.killScore) + std::wstring(L"\nDamage done: ") + std::to_wstring((int)inputC.damageDoneToEnemies)
								+ std::wstring(L"\nTeam Damage taken: ") + std::to_wstring((int)inputC.teamDamageTaken));
						}
						else if (nPC.playerId == 2)
						{
							auto greenPlayer = DOG::UI::Get()->GetUI<UILabel>(lgreenScoreWinID);
							greenPlayer->SetText(std::wstring(L"Green player kill score: ") + std::to_wstring(inputC.killScore) + std::wstring(L"\nDamage done: ") + std::to_wstring((int)inputC.damageDoneToEnemies)
								+ std::wstring(L"\nTeam Damage taken: ") + std::to_wstring((int)inputC.teamDamageTaken));
						}
						else if (nPC.playerId == 3)
						{
							auto yellowPlayer = DOG::UI::Get()->GetUI<UILabel>(lyellowScoreWinID);
							yellowPlayer->SetText(std::wstring(L"Yellow player kill score: ") + std::to_wstring(inputC.killScore) + std::wstring(L"\nDamage done: ") + std::to_wstring((int)inputC.damageDoneToEnemies)
								+ std::wstring(L"\nTeam Damage taken: ") + std::to_wstring((int)inputC.teamDamageTaken));
						}
					});
			}

			break;
		}
		case GameState::Lost:
		{
			if (s_networkStatus != NetworkStatus::Offline)
				NetCode::Get().OnUpdate();
			UI::Get()->ChangeUIscene(GameOverID);
			EntityManager::Get().Collect<InputController, NetworkPlayerComponent>().Do([&](InputController& inputC, NetworkPlayerComponent& nPC)
				{
					if (nPC.playerId == 0)
					{
						auto redPlayer = DOG::UI::Get()->GetUI<UILabel>(lredScoreID);
						redPlayer->SetText(std::wstring(L"Red player kill score: ") + std::to_wstring(inputC.killScore) + std::wstring(L"\nDamage done: ") + std::to_wstring((int)inputC.damageDoneToEnemies)
						+ std::wstring(L"\nTeam Damage taken: ") + std::to_wstring((int)inputC.teamDamageTaken));
					}
					else if (nPC.playerId == 1)
					{
						auto bluePlayer = DOG::UI::Get()->GetUI<UILabel>(lblueScoreID);
						bluePlayer->SetText(std::wstring(L"Blue player kill score: ") + std::to_wstring(inputC.killScore) + std::wstring(L"\nDamage done: ") + std::to_wstring((int)inputC.damageDoneToEnemies)
							+ std::wstring(L"\nTeam Damage taken: ") + std::to_wstring((int)inputC.teamDamageTaken));
					}
					else if (nPC.playerId == 2)
					{
						auto greenPlayer = DOG::UI::Get()->GetUI<UILabel>(lgreenScoreID);
						greenPlayer->SetText(std::wstring(L"Green player kill score: ") + std::to_wstring(inputC.killScore) + std::wstring(L"\nDamage done: ") + std::to_wstring((int)inputC.damageDoneToEnemies)
							+ std::wstring(L"\nTeam Damage taken: ") + std::to_wstring((int)inputC.teamDamageTaken));
					}
					else if (nPC.playerId == 3)
					{
						auto yellowPlayer = DOG::UI::Get()->GetUI<UILabel>(lyellowScoreID);
						yellowPlayer->SetText(std::wstring(L"Yellow player kill score: ") + std::to_wstring(inputC.killScore) + std::wstring(L"\nDamage done: ") + std::to_wstring((int)inputC.damageDoneToEnemies)
							+ std::wstring(L"\nTeam Damage taken: ") + std::to_wstring((int)inputC.teamDamageTaken));
					}
				});
			
			m_nrOfFramesToWait--;
			if (m_nrOfFramesToWait <= 0)
				m_gameState = GameState::ExitingToMainMenu;
			break;
		}
		case GameState::Exiting:
			CloseMainScene();
			break;
		case GameState::ExitingToMainMenu:
		{
			m_nrOfFramesToWait = 300;
			NetCode::Get().Reset();
			m_gameState = GameState::MainMenu;
			UI::Get()->ChangeUIscene(menuID);
			CloseMainScene();
			//Reset ui labels
			auto redPlayer = DOG::UI::Get()->GetUI<UILabel>(lredScoreID);
			redPlayer->SetText(L" ");
			auto bluePlayer = DOG::UI::Get()->GetUI<UILabel>(lblueScoreID);
			bluePlayer->SetText(L" ");
			auto greenPlayer = DOG::UI::Get()->GetUI<UILabel>(lgreenScoreID);
			greenPlayer->SetText(L" ");
			auto yellowPlayer = DOG::UI::Get()->GetUI<UILabel>(lyellowScoreID);
			yellowPlayer->SetText(L" ");
			auto redPlayerWin = DOG::UI::Get()->GetUI<UILabel>(lredScoreWinID);
			redPlayerWin->SetText(L" ");
			auto bluePlayerWin = DOG::UI::Get()->GetUI<UILabel>(lblueScoreWinID);
			bluePlayerWin->SetText(L" ");
			auto greenPlayerWin = DOG::UI::Get()->GetUI<UILabel>(lgreenScoreWinID);
			greenPlayerWin->SetText(L" ");
			auto yellowPlayerWin = DOG::UI::Get()->GetUI<UILabel>(lyellowScoreWinID);
			yellowPlayerWin->SetText(L" ");
			break;
		}
		case GameState::Restart:
			CloseMainScene();
			m_gameState = GameState::Lobby;
			break;
		default:
			break;
		}

	
	LuaGlobal* global = LuaMain::GetGlobal();
	global->SetNumber("DeltaTime", Time::DeltaTime());
	global->SetNumber("ElapsedTime", Time::ElapsedTime());

	KeyBindingDisplayMenu();
}

void GameLayer::StartMainScene()
{
	assert(m_mainScene == nullptr);

	//Change the index to change level. 
	uint32_t levelIndex = 4;
	if (levelIndex >= pcgLevelNames::nrLevels)
	{
		levelIndex = 0;
	}
	if (s_networkStatus == NetworkStatus::Offline)
		m_nrOfPlayers = 1;
	switch (m_selectedScene)
	{
	case SceneComponent::Type::PCGLevelScene:
	{
		std::string levelName = pcgLevelNames::pcgLevels[levelIndex];
		m_mainScene = std::make_unique<PCGLevelScene>
			(
				m_nrOfPlayers,
				std::bind(&GameLayer::SpawnAgents, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5),
				"..\\Offline-Tools\\PCG\\" + levelName
				);
		m_mainScene->SetUpScene();
		break;
	}
	case SceneComponent::Type::OldDefaultScene:
		// old default scene 
		m_mainScene = std::make_unique<OldDefaultScene>(m_nrOfPlayers, std::bind(&GameLayer::SpawnAgents, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));
		m_mainScene->SetUpScene();
		break;
	case SceneComponent::Type::ParticleScene:
		m_mainScene = std::make_unique<ParticleScene>();
		m_mainScene->SetUpScene();
		break;
	default:
		break;
	}

	//Get exit block coords.
	EntityManager::Get().Collect<ExitBlockComponent>().Do([&](entity e, ExitBlockComponent&)
		{
			m_exitPosition = EntityManager::Get().GetComponent<TransformComponent>(e).GetPosition();
		});

	LuaMain::GetScriptManager()->StartScripts();
	if (s_networkStatus != NetworkStatus::Offline)
		NetCode::Get().OnStartup();
	m_gameState = GameState::Playing;

}

void GameLayer::CloseMainScene()
{
	ItemManager::Get().DestroyAllItems();
	m_mainScene.reset();
}


//Only host should do this
void GameLayer::EvaluateWinCondition()
{
	if (m_noWinLose) return;
	
	static f64 freeRoamTimeAfterWin = 0;
	//If the exit is below 0.
	if (m_exitPosition.x < 0.0f)
	{
		bool agentsAlive = false;
		EntityManager::Get().Collect<AgentIdComponent>().Do([&agentsAlive](AgentIdComponent&) { agentsAlive = true; });

		if (agentsAlive)
			freeRoamTimeAfterWin = 0;
		else
			freeRoamTimeAfterWin += Time::DeltaTime();

		if (freeRoamTimeAfterWin > 8.0)
			m_gameState = GameState::Won;
	}
	else
	{
		uint32_t playersAtExit = 0u;
		uint32_t playersAlive = 0u;
		EntityManager::Get().Collect<PlayerAliveComponent>().Do([&](entity e, PlayerAliveComponent&)
		{
			++playersAlive;
			Vector3 pos = EntityManager::Get().GetComponent<TransformComponent>(e).GetPosition();
			if (pos.x > m_exitPosition.x - 2.5f && pos.y > m_exitPosition.y - 2.5f && pos.z > m_exitPosition.z - 2.5f &&
				pos.x < m_exitPosition.x + 2.5f && pos.y < m_exitPosition.y + 2.5f && pos.z < m_exitPosition.z + 2.5f)
			{
				++playersAtExit;
			}
		});
		if (playersAtExit == playersAlive)
		{
			m_gameState = GameState::Won;
		}

		auto playersAtExitText = DOG::UI::Get()->GetUI<UILabel>(l5ID);
		if (playersAtExit > 0 && m_gameState != GameState::Won)
		{
			//Show UI that tells how many are at exit.
			playersAtExitText->SetText(std::wstring(L"Players at exit: ") + std::to_wstring(playersAtExit) + std::wstring(L"/") + std::to_wstring(playersAlive));
		}
		else
		{
			playersAtExitText->SetText(std::wstring(L""));
		}
	}

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

	if (!playersAlive)
	{
		m_gameState = GameState::Lost;
	}
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

	auto& bc = m_entityManager.AddComponent<BarrelComponent>(e);
	bc.type = BarrelComponent::Type::Bullet;
	bc.currentAmmoCount = 30;
	bc.maximumAmmoCapacityForType = 999'999; // Representing infinity...?? (Emil F)

	m_entityManager.AddComponent<PlayerAliveComponent>(e);

	LuaMain::GetScriptManager()->AddScript(e, "Gun.lua");

	LuaMain::GetScriptManager()->AddScript(e, "PassiveItemSystem.lua");

	LuaMain::GetScriptManager()->AddScript(e, "ActiveItemSystem.lua");

	if (m_entityManager.HasComponent<ThisPlayer>(e))
	{
		auto& stats = m_entityManager.GetComponent<PlayerStatsComponent>(e);
		stats.health = stats.maxHealth;
	}

	m_entityManager.GetComponent<AnimationComponent>(e).SimpleAdd(static_cast<i8>(MixamoAnimations::Idle), AnimationFlag::Looping | AnimationFlag::ResetPrio);

	auto& controller = m_entityManager.GetComponent<PlayerControllerComponent>(e);
	m_entityManager.DeferredEntityDestruction(controller.debugCamera);
	controller.debugCamera = DOG::NULL_ENTITY;

	m_entityManager.GetComponent<RigidbodyComponent>(e).ConstrainPosition(false, false, false);
}

void GameLayer::KillPlayer(DOG::entity e)
{
	m_entityManager.RemoveComponent<PlayerAliveComponent>(e);
	if (m_entityManager.HasComponent<AnimationComponent>(e))
	{
		auto& ac = m_entityManager.GetComponent<AnimationComponent>(e);
		ac.SimpleAdd(static_cast<i8>(MixamoAnimations::DeathAnimation), AnimationFlag::Persist, 1u);
	}
	if (m_entityManager.HasComponent<ThisPlayer>(e))
	{
		entity localPlayer = e;

		LuaMain::GetScriptManager()->RemoveScript(localPlayer, "Gun.lua");
		LuaMain::GetScriptManager()->RemoveScript(localPlayer, "PassiveItemSystem.lua");
		LuaMain::GetScriptManager()->RemoveScript(localPlayer, "ActiveItemSystem.lua");
		std::string luaEventName = std::string("ItemPickup") + std::to_string(localPlayer);
		m_entityManager.RemoveComponent<ScriptComponent>(localPlayer);
		m_entityManager.RemoveComponent<BarrelComponent>(localPlayer);
		m_entityManager.RemoveComponent<MiscComponent>(localPlayer);

		RigidbodyComponent& rb = m_entityManager.GetComponent<RigidbodyComponent>(e);
		rb.ConstrainPosition(true, true, true);
		rb.ClearPhysics();

		DOG::entity playerToSpectate = DOG::NULL_ENTITY;
		const char* playerName{ nullptr };
		std::vector<DOG::entity> spectatables;
		
		//Get hold of another living player entity:
		m_entityManager.Collect<NetworkPlayerComponent, PlayerAliveComponent>().Do([&](DOG::entity otherPlayer, NetworkPlayerComponent, PlayerAliveComponent&)
			{
				spectatables.push_back(otherPlayer);
			});

		if (!spectatables.empty())
		{
			playerToSpectate = spectatables[0];
			playerName = m_entityManager.GetComponent<NetworkPlayerComponent>(spectatables[0]).playerName;
		}

		if (playerToSpectate != NULL_ENTITY) //So, if not all players are dead
		{
			auto& pcc = m_entityManager.GetComponent<PlayerControllerComponent>(localPlayer);
			auto& otherPcc = m_entityManager.GetComponent<PlayerControllerComponent>(playerToSpectate);
			pcc.spectatorCamera = m_mainScene->CreateEntity();

			m_entityManager.AddComponent<TransformComponent>(pcc.spectatorCamera)
				.worldMatrix = m_entityManager.GetComponent<TransformComponent>(otherPcc.cameraEntity).worldMatrix;
			
			m_entityManager.AddComponent<CameraComponent>(pcc.spectatorCamera).isMainCamera = true;
			
			m_entityManager.Collect<ChildComponent>().Do([&](DOG::entity playerModel, ChildComponent& cc)
				{
					if (cc.parent == localPlayer)
					{
						//This means that playerModel is the mesh model (suit), and it should be drawing for the main player again:
						m_entityManager.RemoveComponent<DontDraw>(playerModel);
					}
					else if (cc.parent == playerToSpectate)
					{
						//This means that playerModel is the spectated players' armor/suit, and it should not be eligible for drawing anymore:
						m_entityManager.AddComponent<DontDraw>(playerModel);
					}
				});

			auto& sc = m_entityManager.AddComponent<SpectatorComponent>(localPlayer);
			sc.playerBeingSpectated = playerToSpectate;
			sc.playerName = playerName;
			sc.playerSpectatorQueue = spectatables;

			auto& timer = m_entityManager.AddComponent<DeathUITimerComponent>(localPlayer);
			timer.duration = 4.0f;
			timer.timeLeft = timer.duration;
		}
		else // Of course, if all players are dead, this else will fire, but then the game would restart, so probably unnecessary.
		{
			auto& controller = m_entityManager.GetComponent<PlayerControllerComponent>(localPlayer);
			controller.debugCamera = m_mainScene->CreateEntity();

			m_entityManager.AddComponent<TransformComponent>(controller.debugCamera)
				.worldMatrix = m_entityManager.GetComponent<TransformComponent>(controller.cameraEntity);

			m_entityManager.AddComponent<CameraComponent>(controller.debugCamera).isMainCamera = true;
		}
	}
}

void GameLayer::UpdateGame()
{
	LuaMain::GetScriptManager()->UpdateScripts();
	LuaMain::GetScriptManager()->ReloadScripts();

	if (s_networkStatus != NetworkStatus::Offline)
		NetCode::Get().OnUpdate();

	HandleCheats();
	HpBarMVP();
	CheckIfPlayersIAreDead();

	EvaluateWinCondition();
	EvaluateLoseCondition();

	// Render this player or not
	EntityManager::Get().Collect<DontDraw>().Do([&](DontDraw& dd)
		{
			dd.dontDraw = !m_imguiRenderPlayer;
		});

	if (m_syncFrame)
	{
		// Reset animations
		EntityManager::Get().Collect<InputController, AnimationComponent>().Do([&](InputController&, AnimationComponent& ac)
			{
				ac.SimpleAdd(static_cast<i8>(MixamoAnimations::BindPose),  AnimationFlag::Interrupt | AnimationFlag::ResetPrio);
				ac.SimpleAdd(static_cast<i8>(MixamoAnimations::Idle), AnimationFlag::Looping);
			});
		m_syncFrame = false;
	}

	EntityManager::Get().Collect<TransformComponent, RigidbodyComponent>().Do([](TransformComponent& transform, RigidbodyComponent&)
		{
			if (Vector3 pos = transform.GetPosition(); pos.y < -20.0f)
			{
				pos.y = 10;
				transform.SetPosition(pos);
			}
		});

	//Check if assets should create lights
	EntityManager::Get().Collect<CheckForLightsComponent>().Do([](entity e, CheckForLightsComponent&)
		{
			ModelAsset* asset = AssetManager::Get().GetAsset<ModelAsset>(EntityManager::Get().GetComponent<ModelComponent>(e).id);
			if (asset)
			{
				Vector3 assetPosition = EntityManager::Get().GetComponent<TransformComponent>(e).GetPosition();
				for (uint32_t i{ 0u }; i < asset->lights.size(); ++i)
				{
					ImportedLight& currentLight = asset->lights[i];

					entity newLightEntity = EntityManager::Get().CreateEntity();
					Vector3 globalPosition;
					if (EntityManager::Get().HasComponent<TransformComponent>(e))
					{
						globalPosition = Vector3::Transform(Vector3(currentLight.translation), EntityManager::Get().GetComponent<TransformComponent>(e).worldMatrix);
					}
					EntityManager::Get().AddComponent<TransformComponent>(newLightEntity).SetPosition(globalPosition);

					PointLightDesc desc;
					desc.color = Vector3(currentLight.color[0], currentLight.color[1], currentLight.color[2]);
					desc.position = globalPosition;
					desc.radius = currentLight.radius;
					desc.strength = 1.0f;

					LightHandle handle = LightManager::Get().AddPointLight(desc, LightUpdateFrequency::Never);

					PointLightComponent& pointLightComp = EntityManager::Get().AddComponent<PointLightComponent>(newLightEntity);
					pointLightComp.handle = handle;
					pointLightComp.dirty = false;

					if (EntityManager::Get().HasComponent<SceneComponent>(e))
					{
						EntityManager::Get().AddComponent<SceneComponent>(newLightEntity, EntityManager::Get().GetComponent<SceneComponent>(e).scene);
					}
				}

				EntityManager::Get().RemoveComponent<CheckForLightsComponent>(e);
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
		if (EVENT(KeyPressedEvent).key == DOG::Key::Esc)
		{
			if (m_gameState == GameState::Playing)
			{
				if (InGameMenu::IsOpen())
				{
					Window::SetCursorMode(CursorMode::Confined);
					InGameMenu::Close();
				}
				else
				{
					InGameMenu::Open();
					
					Window::SetCursorMode(CursorMode::Visible);
				}
				EntityManager::Get().Collect<InputController, ThisPlayer>().Do([](InputController& inputC, ThisPlayer&) { inputC.toggleMoveView = true; });
				event.StopPropagation();
			}
		}
		if (InGameMenu::IsOpen()) break;

		if (EVENT(KeyPressedEvent).key == DOG::Key::E)
		{
			Interact();
		}
		
		Input(EVENT(KeyPressedEvent).key);
		break;
	}
	case EventType::KeyReleasedEvent:
	{
		Release(EVENT(KeyReleasedEvent).key);
	}
	}
}

//Lobby
void HostButtonFunc(void)
{	
	bool succes = true;
	int roomId = 0;
	
	std::string ip = NetCode::Get().GetIpAdress();

	if (ip == ROOM_1_IP) // Sam
	{
	 	roomId = 1;
		NetCode::Get().SetMulticastAdress("239.255.255.1");
	 	
		if (NetCode::Get().Host())
		{
			GameLayer::ChangeNetworkState(NetworkStatus::Hosting);
		}
		else
		{
			succes = false;
		}
	}
	else if (ip == ROOM_2_IP) //Filip
	{
	 	roomId = 2;
		NetCode::Get().SetMulticastAdress("239.255.255.2");
		if (NetCode::Get().Host())
		{
			GameLayer::ChangeNetworkState(NetworkStatus::Hosting);
		}
		else
		{
			succes = false;
		}
	}
	else if (ip == ROOM_3_IP) //Nad
	{
	 	roomId = 3;
		NetCode::Get().SetMulticastAdress("239.255.255.3");
		if (NetCode::Get().Host())
		{
			GameLayer::ChangeNetworkState(NetworkStatus::Hosting);
		}
		else
		{
			succes = false;
		}
	}
	else if (ip == ROOM_4_IP) //Axel
	{
	 	roomId = 4;
		NetCode::Get().SetMulticastAdress("239.255.255.4");
		if (NetCode::Get().Host())
		{
			GameLayer::ChangeNetworkState(NetworkStatus::Hosting);
		}
		else
		{
			succes = false;
		}
	}
	else if (ip == ROOM_5_IP) //Ove
	{
	 	roomId = 5;
		NetCode::Get().SetMulticastAdress("239.255.255.5");
		if (NetCode::Get().Host())
		{
			GameLayer::ChangeNetworkState(NetworkStatus::Hosting);
		}
		else
		{
			succes = false;
		}
	}
	else if (ip == ROOM_6_IP) //Gunnar
	{
	 	roomId = 6;
		NetCode::Get().SetMulticastAdress("239.255.255.6");
		if (NetCode::Get().Host())
		{
			GameLayer::ChangeNetworkState(NetworkStatus::Hosting);
		}
		else
		{
			succes = false;
		}
	}
	else if (ip == ROOM_7_IP) //Emil F
	{
	 	roomId = 7;
		NetCode::Get().SetMulticastAdress("239.255.255.7");
		if (NetCode::Get().Host())
		{
			GameLayer::ChangeNetworkState(NetworkStatus::Hosting);
		}
		else
		{
			succes = false;
		}
	}
	else if (ip == ROOM_8_IP) //Jonatan
	{
	 	roomId = 8;
		NetCode::Get().SetMulticastAdress("239.255.255.8");
		if (NetCode::Get().Host())
		{
			GameLayer::ChangeNetworkState(NetworkStatus::Hosting);
		}
		else
		{
			succes = false;
		}
	}
	else if (ip == ROOM_9_IP) //Emil H
	{
	 	roomId = 9;
		NetCode::Get().SetMulticastAdress("239.255.255.9");
		if (NetCode::Get().Host())
		{
			GameLayer::ChangeNetworkState(NetworkStatus::Hosting);
		}
		else
		{
			succes = false;
		}
	}
	else
	{
	 	roomId = 10;
		NetCode::Get().SetMulticastAdress("239.255.255.0");
		if (NetCode::Get().Host())
		{
			GameLayer::ChangeNetworkState(NetworkStatus::Hosting);
		}
		else
		{
			succes = false;
		}
	}

	if (succes)
	{
		GameLayer::ChangeGameState(GameState::Lobby);
		DOG::UI::Get()->ChangeUIscene(lobbyID);
		auto text2 = DOG::UI::Get()->GetUI<UILabel>(l1ID);
		text2->SetText(std::wstring(L"Room ") + std::to_wstring(roomId));
		auto text3 = DOG::UI::Get()->GetUI<UILabel>(l2ID);
		text3->SetText(std::wstring(L"Ip: ") + std::wstring(ip.begin(), ip.end()));
	}
}


void JoinButton(void)
{
	DOG::UI::Get()->ChangeUIscene(joinID);
}

void BackFromHost(void)
{
	DOG::UI::Get()->ChangeUIscene(multiID);
	NetCode::Get().Reset();
	GameLayer::ChangeNetworkState(NetworkStatus::Offline);
}

void HostLaunch(void)
{
	NetCode::Get().Play();
	
	GameLayer::ChangeGameState(GameState::StartPlaying);
	DOG::UI::Get()->ChangeUIscene(gameID);
}

void PlayButtonFunc(void)
{
	if(GameLayer::GetGameStatus() != GameState::Playing)
		GameLayer::ChangeGameState(GameState::StartPlaying);
	GameLayer::ChangeNetworkState(NetworkStatus::Offline);
	DOG::UI::Get()->ChangeUIscene(gameID);

}

void Room1Button(void)
{
	NetCode::Get().SetMulticastAdress("239.255.255.1");
	static char input[8]{};
	input[0] = 'a';
	if (NetCode::Get().Join(input))
	{
		auto text2 = DOG::UI::Get()->GetUI<UILabel>(l4ID);
		text2->SetText(std::wstring(L"Room 1"));
		GameLayer::ChangeNetworkState(NetworkStatus::Joining);
		GameLayer::ChangeGameState(GameState::Lobby);
		DOG::UI::Get()->ChangeUIscene(WaitingForHostID);
	}
	else
	{
		DOG::UI::Get()->ChangeUIscene(multiID);
	}
}

void Room2Button(void)
{
	NetCode::Get().SetMulticastAdress("239.255.255.2");
	static char input[8]{};
	input[0] = 'b';
	if (NetCode::Get().Join(input))
	{
		auto text2 = DOG::UI::Get()->GetUI<UILabel>(l4ID);
		text2->SetText(std::wstring(L"Room 2"));
		GameLayer::ChangeNetworkState(NetworkStatus::Joining);
		GameLayer::ChangeGameState(GameState::Lobby);
		DOG::UI::Get()->ChangeUIscene(WaitingForHostID);
	}
	else
	{
		DOG::UI::Get()->ChangeUIscene(multiID);
	}
}

void Room3Button(void)
{
	NetCode::Get().SetMulticastAdress("239.255.255.3");
	static char input[8]{};
	input[0] = 'c';
	if (NetCode::Get().Join(input))
	{
		auto text2 = DOG::UI::Get()->GetUI<UILabel>(l4ID);
		text2->SetText(std::wstring(L"Room 3"));
		GameLayer::ChangeNetworkState(NetworkStatus::Joining);
		GameLayer::ChangeGameState(GameState::Lobby);
		DOG::UI::Get()->ChangeUIscene(WaitingForHostID);
	}
	else
	{
		DOG::UI::Get()->ChangeUIscene(multiID);
	}
}
void Room4Button(void)
{
	NetCode::Get().SetMulticastAdress("239.255.255.4");
	static char input[8]{};
	input[0] = 'd';
	if (NetCode::Get().Join(input))
	{
		auto text2 = DOG::UI::Get()->GetUI<UILabel>(l4ID);
		text2->SetText(std::wstring(L"Room 4"));
		GameLayer::ChangeNetworkState(NetworkStatus::Joining);
		GameLayer::ChangeGameState(GameState::Lobby);
		DOG::UI::Get()->ChangeUIscene(WaitingForHostID);
	}
	else
	{
		DOG::UI::Get()->ChangeUIscene(multiID);
	}
}
void Room5Button(void)
{
	NetCode::Get().SetMulticastAdress("239.255.255.5");
	static char input[8]{};
	input[0] = 'e';
	if (NetCode::Get().Join(input))
	{
		auto text2 = DOG::UI::Get()->GetUI<UILabel>(l4ID);
		text2->SetText(std::wstring(L"Room 5"));
		GameLayer::ChangeNetworkState(NetworkStatus::Joining);
		GameLayer::ChangeGameState(GameState::Lobby);
		DOG::UI::Get()->ChangeUIscene(WaitingForHostID);
	}
	else
	{
		DOG::UI::Get()->ChangeUIscene(multiID);
	}
}
void Room6Button(void)
{
	NetCode::Get().SetMulticastAdress("239.255.255.6");
	static char input[8]{};
	input[0] = 'f';
	if (NetCode::Get().Join(input))
	{
		auto text2 = DOG::UI::Get()->GetUI<UILabel>(l4ID);
		text2->SetText(std::wstring(L"Room 6"));
		GameLayer::ChangeNetworkState(NetworkStatus::Joining);
		GameLayer::ChangeGameState(GameState::Lobby);
		DOG::UI::Get()->ChangeUIscene(WaitingForHostID);
	}
	else
	{
		DOG::UI::Get()->ChangeUIscene(multiID);
	}
}
void Room7Button(void)
{
	NetCode::Get().SetMulticastAdress("239.255.255.7");
	static char input[8]{};
	input[0] = 'g';
	if (NetCode::Get().Join(input))
	{
		auto text2 = DOG::UI::Get()->GetUI<UILabel>(l4ID);
		text2->SetText(std::wstring(L"Room 7"));
		GameLayer::ChangeNetworkState(NetworkStatus::Joining);
		GameLayer::ChangeGameState(GameState::Lobby);
		DOG::UI::Get()->ChangeUIscene(WaitingForHostID);
	}
	else
	{
		DOG::UI::Get()->ChangeUIscene(multiID);
	}
}
void Room8Button(void)
{
	NetCode::Get().SetMulticastAdress("239.255.255.8");
	static char input[8]{};
	input[0] = 'h';
	if (NetCode::Get().Join(input))
	{
		auto text2 = DOG::UI::Get()->GetUI<UILabel>(l4ID);
		text2->SetText(std::wstring(L"Room 8"));
		GameLayer::ChangeNetworkState(NetworkStatus::Joining);
		GameLayer::ChangeGameState(GameState::Lobby);
		DOG::UI::Get()->ChangeUIscene(WaitingForHostID);
	}
	else
	{
		DOG::UI::Get()->ChangeUIscene(multiID);
	}
}

void Room9Button(void)
{
	NetCode::Get().SetMulticastAdress("239.255.255.9");
	static char input[8]{};
	input[0] = 'i';
	if (NetCode::Get().Join(input))
	{
		auto text2 = DOG::UI::Get()->GetUI<UILabel>(l4ID);
		text2->SetText(std::wstring(L"Room 9"));
		GameLayer::ChangeNetworkState(NetworkStatus::Joining);
		GameLayer::ChangeGameState(GameState::Lobby);
		DOG::UI::Get()->ChangeUIscene(WaitingForHostID);
	}
	else
	{
		DOG::UI::Get()->ChangeUIscene(multiID);
	}
}

void Room10Button(void)
{
	NetCode::Get().SetMulticastAdress("239.255.255.0");
	static char input[8]{};
	input[0] = 'u';
	if (NetCode::Get().Join(input))
	{
		GameLayer::ChangeNetworkState(NetworkStatus::Joining);
		GameLayer::ChangeGameState(GameState::Lobby);
		auto text2 = DOG::UI::Get()->GetUI<UILabel>(l4ID);
		text2->SetText(std::wstring(L"Room 10"));
		DOG::UI::Get()->ChangeUIscene(WaitingForHostID);
	}
	else
	{
		DOG::UI::Get()->ChangeUIscene(multiID);
	}
}


void GameLayer::UpdateLobby()
{
	if (m_mainScene)
		CloseMainScene();
	if (s_networkStatus != NetworkStatus::Offline)
		NetCode::Get().OnUpdate();
	bool inLobby = m_gameState == GameState::Lobby;
	if (ImGui::Begin("Lobby", &inLobby))
	{
		switch (s_networkStatus)
		{
		case NetworkStatus::Offline:
		{
			ImGui::Text("Host to host, join to join, play to play offline");
			if (ImGui::Button("Host"))
			{
				s_networkStatus = NetworkStatus::HostLobby;

			}
			if (ImGui::Button("Join"))
			{
				s_networkStatus = NetworkStatus::JoinLobby;
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
				NetCode::Get().SetMulticastAdress("239.255.255.0");
				if (NetCode::Get().Host())
				{
					s_networkStatus = NetworkStatus::Hosting;
				}
			}
			if (ImGui::Button("Host Sam"))
			{
				NetCode::Get().SetMulticastAdress("239.255.255.1");
				if (NetCode::Get().Host())
				{
					s_networkStatus = NetworkStatus::Hosting;
				}
			}
			if (ImGui::Button("Host Filip"))
			{
				NetCode::Get().SetMulticastAdress("239.255.255.2");
				if (NetCode::Get().Host())
				{
					s_networkStatus = NetworkStatus::Hosting;
				}
			}
			if (ImGui::Button("Host Nad"))
			{
				NetCode::Get().SetMulticastAdress("239.255.255.3");
				if (NetCode::Get().Host())
				{
					s_networkStatus = NetworkStatus::Hosting;
				}
			}
			if (ImGui::Button("Host Axel"))
			{
				NetCode::Get().SetMulticastAdress("239.255.255.4");
				if (NetCode::Get().Host())
				{
					s_networkStatus = NetworkStatus::Hosting;
				}
			}
			if (ImGui::Button("Host Ove"))
			{
				NetCode::Get().SetMulticastAdress("239.255.255.5");
				if (NetCode::Get().Host())
				{
					s_networkStatus = NetworkStatus::Hosting;
				}
			}
			if (ImGui::Button("Host Gunnar"))
			{
				NetCode::Get().SetMulticastAdress("239.255.255.6");
				if (NetCode::Get().Host())
				{
					s_networkStatus = NetworkStatus::Hosting;
				}
			}
			if (ImGui::Button("Host Emil F"))
			{
				NetCode::Get().SetMulticastAdress("239.255.255.7");
				if (NetCode::Get().Host())
				{
					s_networkStatus = NetworkStatus::Hosting;
				}
			}
			if (ImGui::Button("Host Jonatan"))
			{
				NetCode::Get().SetMulticastAdress("239.255.255.8");
				if (NetCode::Get().Host())
				{
					s_networkStatus = NetworkStatus::Hosting;
				}
			}
			break;
		}
		case NetworkStatus::Hosting:
		{
			NetCode::Get().SetLobbyStatus(true);
			char ip[64];
			strcpy_s(ip, NetCode::Get().GetIpAdress().c_str());
			ImGui::Text("Nr of players connected: %d", NetCode::Get().GetNrOfPlayers());
			auto text3 = DOG::UI::Get()->GetUI<UILabel>(l3ID);
			text3->SetText(std::wstring(L"Nr of players connected ") + std::to_wstring(NetCode::Get().GetNrOfPlayers()));
			ImGui::Text("Youre ip adress: %s", ip);
			if (ImGui::Button("Play"))
			{
				m_nrOfPlayers = NetCode::Get().Play();
				inLobby = false;
			}
			break;
		}
		case NetworkStatus::JoinLobby:
		{
			break;
		}
		case NetworkStatus::Joining:
		{
			auto text2 = DOG::UI::Get()->GetUI<UILabel>(l6ID);
			text2->SetText(std::wstring(L"Nr of players connected: ") + std::to_wstring(NetCode::Get().GetNrOfPlayers()));
			m_nrOfPlayers = NetCode::Get().GetNrOfPlayers();
			ImGui::Text("Nr of players connected: %d", NetCode::Get().GetNrOfPlayers());
			ImGui::Text("Waiting for Host to press Play...");
			inLobby = NetCode::Get().IsLobbyAlive();
			break;
		}
		default:
			break;
		}
		if (!inLobby && s_networkStatus == NetworkStatus::Joining)
		{
			DOG::UI::Get()->ChangeUIscene(gameID);
			m_gameState = GameState::StartPlaying;
			m_nrOfPlayers = NetCode::Get().GetNrOfPlayers();
		}
		else if (!inLobby)
			m_gameState = GameState::StartPlaying;
		m_nrOfPlayers = NetCode::Get().GetNrOfPlayers();
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
	luaInterface.AddFunction<EntityInterface, &EntityInterface::GetMiscType>("GetMiscType");
	luaInterface.AddFunction<EntityInterface, &EntityInterface::GetAmmoCapacityForBarrelType>("GetAmmoCapacityForBarrelType");
	luaInterface.AddFunction<EntityInterface, &EntityInterface::GetAmmoCountPerPickup>("GetAmmoCountPerPickup");
	luaInterface.AddFunction<EntityInterface, &EntityInterface::GetOutlineColor>("GetOutlineColor");
	luaInterface.AddFunction<EntityInterface, &EntityInterface::UpdateMagazine>("UpdateMagazine");
	luaInterface.AddFunction<EntityInterface, &EntityInterface::IsBulletLocal>("IsBulletLocal");
	luaInterface.AddFunction<EntityInterface, &EntityInterface::Exists>("Exists");
	luaInterface.AddFunction<EntityInterface, &EntityInterface::GetEntityTypeAsString>("GetEntityTypeAsString");
	
	//luaInterface.AddFunction<EntityInterface, &EntityInterface::AgentHit>("AgentHit");
	luaInterface.AddFunction<EntityInterface, &EntityInterface::ModifyAnimationComponent>("ModifyAnimationComponent");
	luaInterface.AddFunction<EntityInterface, &EntityInterface::SpawnActiveItem>("SpawnActiveItem");
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
	luaInterface.AddFunction<PhysicsInterface, &PhysicsInterface::RayCast>("RayCast");

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
	luaInterface.AddFunction<GameInterface, &GameInterface::AddDamageToEntity>("AddDamageToEntity");
	luaInterface.AddFunction<GameInterface, &GameInterface::AddMagazineEffectsFromBullet>("AddMagazineEffectsFromBullet");
	luaInterface.AddFunction<GameInterface, &GameInterface::SpawnPickupMiscComponent>("SpawnPickupMiscComponent");
	luaInterface.AddFunction<GameInterface, &GameInterface::GetPlayerName>("GetPlayerName");

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
			if (key == DOG::Key::E)
				inputC.revive = true;
			if (key == DOG::Key::One)
				inputC.throwGlowStick = true;
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
			if (key == DOG::Key::E)
				inputC.revive = false;
			if (key == DOG::Key::One)
				inputC.throwGlowStick = false;

		});
}

std::vector<entity> GameLayer::SpawnAgents(const EntityTypes type, SceneComponent::Type scene, const Vector3& pos, u8 agentCount, f32 spread)
{
	ASSERT(EntityTypes::AgentsBegin <= type && type < EntityTypes::Agents, "type must be of in range EntityTypes::AgentBegin - EntityTypes::Agents");
	ASSERT(agentCount < AgentManager::GROUP_SIZE, "number of agents in group may not exceed AgentManager::GROUP_SIZE");

	u32 groupID = AgentManager::Get().GroupID();

	std::vector<entity> agents;
	for (auto i = 0; i < agentCount; ++i)
	{
		Vector3 offset = {
			spread * (i % 2) - (spread / 2.f),
			0,
			spread * (i % 2) - (spread / 2.f),
		};
		agents.emplace_back(AgentManager::Get().CreateAgent(type, groupID, pos - offset, scene));
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
			hbar->SetBarValue(stats.health, stats.maxHealth);
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

			bool checkboxLightScene = m_lightScene != nullptr;
			if (ImGui::Checkbox("LightScene", &checkboxLightScene))
			{
				if (checkboxLightScene)
				{
					m_lightScene = std::make_unique<LightScene>();
					m_lightScene->SetUpScene();
				}
				else
				{
					m_lightScene.reset();
					m_lightScene = nullptr;
				}
			}
			
			if (ImGui::RadioButton("PCGLevel", (int*)&m_selectedScene, (int)SceneComponent::Type::PCGLevelScene)) m_gameState = GameState::Restart;

			if (ImGui::RadioButton("OldBox", (int*)&m_selectedScene, (int)SceneComponent::Type::OldDefaultScene)) m_gameState = GameState::Restart;
			if (ImGui::RadioButton("Particle", (int*)&m_selectedScene, (int)SceneComponent::Type::ParticleScene)) m_gameState = GameState::Restart;
			
			ImGui::Checkbox("RenderPlayer", &m_imguiRenderPlayer);

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
	if (ImGui::Button("SpawnItems"))
	{
		constexpr float offset = 1.5f;
		Vector3 itemBaseSpawnPos = EntityManager::Get().GetComponent<TransformComponent>(GetPlayer()).GetPosition();
		itemBaseSpawnPos.y -= 0.4f;
		itemBaseSpawnPos.x -= 5;
		itemBaseSpawnPos.z -= 5.0f * offset / 2.0f;
		Vector3 pos = itemBaseSpawnPos;

		for (int i = (int)EntityTypes::PickupsBegin; i < (int)EntityTypes::Pickups; i++)
		{
			ItemManager::Get().CreateItem((EntityTypes)i, Vector3(pos));
			pos.x += offset;
			ItemManager::Get().CreateItem((EntityTypes)i, Vector3(pos));
			pos.x += offset;
		}

		itemBaseSpawnPos.z += offset;
		pos = itemBaseSpawnPos;
		for (int i = (int)EntityTypes::PassiveItemsBegin + 1; i < (int)EntityTypes::PassiveItems; i++)
		{
			ItemManager::Get().CreateItem((EntityTypes)i, Vector3(pos));
			pos.x += offset;
			ItemManager::Get().CreateItem((EntityTypes)i, Vector3(pos));
			pos.x += offset;
		}

		itemBaseSpawnPos.z += offset;
		pos = itemBaseSpawnPos;
		for (int i = (int)EntityTypes::ActiveItemsBegin + 1; i < (int)EntityTypes::ActiveItems; i++)
		{
			ItemManager::Get().CreateItem((EntityTypes)i, Vector3(pos));
			pos.x += offset;
			ItemManager::Get().CreateItem((EntityTypes)i, Vector3(pos));
			pos.x += offset;
		}

		itemBaseSpawnPos.z += offset;
		pos = itemBaseSpawnPos;
		for (int i = (int)EntityTypes::BarrelItemsBegin + 1; i < (int)EntityTypes::Barrels; i++)
		{
			ItemManager::Get().CreateItem((EntityTypes)i, Vector3(pos));
			pos.x += offset;
			ItemManager::Get().CreateItem((EntityTypes)i, Vector3(pos));
			pos.x += offset;
		}

		itemBaseSpawnPos.z += offset;
		pos = itemBaseSpawnPos;
		for (int i = (int)EntityTypes::MagazineModificationItemsBegin + 1; i < (int)EntityTypes::Magazines; i++)
		{
			ItemManager::Get().CreateItem((EntityTypes)i, Vector3(pos));
			pos.x += offset;
			ItemManager::Get().CreateItem((EntityTypes)i, Vector3(pos));
			pos.x += offset;
		}

		itemBaseSpawnPos.z += offset;
		pos = itemBaseSpawnPos;
		for (int i = (int)EntityTypes::MiscItemsBegin; i < (int)EntityTypes::Miscs; i++)
		{
			ItemManager::Get().CreateItem((EntityTypes)i, Vector3(pos));
			pos.x += offset;
			ItemManager::Get().CreateItem((EntityTypes)i, Vector3(pos));
			pos.x += offset;
		}
	}
	if (ImGui::Button("Respawn"))
	{
		RespawnDeadPlayer(GetPlayer());
	}
	static bool isLegacy = false;
	if (!isLegacy)
	{
		if (ImGui::Button("Legacy weapon system"))
		{
			auto player = GetPlayer();
			LuaMain::GetScriptManager()->RemoveScript(player, "Gun.lua");
			LuaMain::GetScriptManager()->AddScript(player, "GunLegacy.lua");
			isLegacy = !isLegacy;
		}
	}
	else
	{
		if (ImGui::Button("Normal weapon system"))
		{
			auto player = GetPlayer();
			LuaMain::GetScriptManager()->RemoveScript(player, "GunLegacy.lua");
			LuaMain::GetScriptManager()->AddScript(player, "Gun.lua");
			isLegacy = !isLegacy;
		}
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

void GameLayer::Interact()
{
	auto player = GetPlayer();
	if (!m_entityManager.HasComponent<InteractionQueryComponent>(player))
		m_entityManager.AddComponent<InteractionQueryComponent>(player);
}


void GameLayer::ChangeGameState(GameState state)
{
	m_gameState = state;
}

void GameLayer::ChangeNetworkState(NetworkStatus state)
{
	s_networkStatus = state;
}