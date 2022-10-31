#pragma once
#include "MainPlayer.h"
#include "LuaInterfaces.h"
#include "NetCode.h"
#include "Pathfinder/Pathfinder.h"
#include "AgentManager/AgentManager.h"
#include "GameComponent.h"
#include "GameSystems.h"


enum class GameState
{
	None = 0,
	Initializing,
	Lobby,
	StartPlaying,
	Playing,
	Won,
	Lost,
	Exiting
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
	std::vector<DOG::entity> LoadLevel(); //Loads a PCG generated level.
	void Input(DOG::Key key);
	void Release(DOG::Key key);
	void CameraUpdate();
	std::vector<DOG::entity> SpawnPlayers(const DirectX::SimpleMath::Vector3& pos, u8 playerCount, f32 spread = 10.f);
	std::vector<DOG::entity> AddFlashlightsToPlayers(const std::vector<DOG::entity>& players);
	std::vector<DOG::entity> SpawnAgents(const EntityTypes type, const DirectX::SimpleMath::Vector3& pos, u8 agentCount, f32 spread = 10.f);

	void HandleCheats();

	void KeyBindingDisplayMenu();
	void GameLayerDebugMenu(bool& open);
	void CheatSettingsImGuiMenu();
	void CheatDebugMenu(bool& open);
private:
	GameState m_gameState;
	std::unique_ptr<DOG::Scene> m_testScene;
	std::unique_ptr<DOG::Scene> m_mainScene;
	std::vector<u32> m_shapes;
	DOG::EntityManager& m_entityManager;
	std::vector<std::shared_ptr<LuaInterface>> m_luaInterfaces;
	std::shared_ptr<MainPlayer> m_player;
	std::array<u32, 4> m_playerModels;
	NetCode m_netCode;
	//Pathfinder m_pathfinder;		// uncomment to activate pathfinder
	INT8 m_nrOfPlayers;
	AgentManager* m_agentManager;
	u8 m_networkStatus;
	ImFont* m_imguiFont = nullptr;

	// Temp container for keybindings, just strings to visualize them in the menu
	std::vector<std::pair<std::string, std::string>> m_keyBindingDescriptions;
	bool m_displayKeyBindings = true;


	// Cheats
	bool m_isCheating = false;
	bool m_godModeCheat = false;
	bool m_unlimitedAmmoCheat = false;
	bool m_noClipCheat = false;

};