#pragma once
#include "LuaInterfaces.h"
#include "NetCode.h"
#include "AgentManager/AgentManager.h"
#include "GameComponent.h"
#include "GameSystems.h"
#include "Scene.h"


enum class GameState
{
	None = 0,
	MainMenu,
	Initializing,
	Lobby,
	StartPlaying,
	Playing,
	Won,
	Lost,
	Exiting,
	ExitingToMainMenu,
	Restart,
	WaitingForHost
};

enum class NetworkStatus
{
	Offline = 0,
	HostLobby,
	Hosting,
	JoinLobby,
	Joining,
};

class GameLayer : public DOG::Layer
{
public:
	GameLayer() noexcept;
	virtual ~GameLayer() override final;
	virtual void OnAttach() override final;
	virtual void OnDetach() override final;
	virtual void OnUpdate() override final;
	virtual void OnRender() override final;
	virtual void OnImGuiRender() override final;
	virtual void OnEvent(DOG::IEvent& event) override final;
	
	static void ChangeGameState(GameState state);
	static void ChangeNetworkState(NetworkStatus);
	static void ResetConnectedPlayers();

	static GameState GetGameStatus() { return m_gameState; };
	static NetworkStatus GetNetworkStatus() { return s_networkStatus; }
	static uint32_t s_levelIndex;
private:
	void UpdateLobby();
	void UpdateGame();
	void StartMainScene();
	void CloseMainScene();

	void EvaluateWinCondition();
	void EvaluateLoseCondition();
	void CheckIfPlayersIAreDead();
	void RespawnDeadPlayer(DOG::entity e);
	void KillPlayer(DOG::entity e);

	void RegisterLuaInterfaces();
	void Input(DOG::Key key);
	void Release(DOG::Key key);
	std::vector<DOG::entity> SpawnAgents(const EntityTypes type, SceneComponent::Type scene, const DirectX::SimpleMath::Vector3& pos, u8 agentCount, f32 spread = 10.f);
	void ToggleFlashlight();

	void HandleCheats();

	void HpBarMVP();
	void GameLayerDebugMenu(bool& open);
	void CheatSettingsImGuiMenu();
	void CheatDebugMenu(bool& open);
	void Interact();
private:
	static GameState m_gameState;
	static NetworkStatus s_networkStatus;
	SceneComponent::Type m_selectedScene = SceneComponent::Type::PCGLevelScene;
	std::unique_ptr<Scene> m_testScene;
	std::unique_ptr<Scene> m_mainScene;
	std::unique_ptr<Scene> m_lightScene;
	std::vector<u32> m_shapes;
	DOG::EntityManager& m_entityManager;
	std::vector<std::shared_ptr<LuaInterface>> m_luaInterfaces;
	std::array<u32, 4> m_playerModels;
	INT8 m_nrOfPlayers;
	ImFont* m_imguiFont = nullptr;

	// Cheats
	bool m_isCheating = false;
	bool m_godModeCheat = false;
	bool m_unlimitedAmmoCheat = false;
	bool m_noClipCheat = false;
	bool m_noWinLose = false;

	//Win condition
	DirectX::SimpleMath::Vector3 m_exitPosition = DirectX::SimpleMath::Vector3(-1.0f, -1.0f, -1.0f);

	bool m_imguiRenderPlayer = false;
	bool m_syncFrame = false;
	int m_nrOfFramesToWait = 300;

	static bool s_connectedPlayersLobby[MAX_PLAYER_COUNT];
};
