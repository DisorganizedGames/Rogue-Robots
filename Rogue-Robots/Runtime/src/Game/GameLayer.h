#pragma once
#include "MainPlayer.h"
#include "LuaInterfaces.h"
#include "NetCode.h"
#include "Pathfinder/Pathfinder.h"
#include "Agent.h"
#include "GameComponent.h"
#include "GameSystems.h"

class GameLayer : public DOG::Layer
{
public:
	GameLayer() noexcept;
	virtual ~GameLayer() override final = default;
	virtual void OnAttach() override final;
	virtual void OnDetach() override final;
	virtual void OnUpdate() override final;
	virtual void OnRender() override final;
	virtual void OnImGuiRender() override final;
	virtual void OnEvent(DOG::IEvent& event) override final;

private:
	void RegisterLuaInterfaces();
	std::vector<DOG::entity> LoadLevel(); //Loads a PCG generated level.
	void Input(DOG::Key key);
	void Release(DOG::Key key);
	void CameraUpdate();
	std::vector<DOG::entity> SpawnPlayers(const DirectX::SimpleMath::Vector3& pos, u8 playerCount, f32 spread = 10.f);
private:
	std::unique_ptr<DOG::Scene> m_testScene;
	std::unique_ptr<DOG::Scene> m_mainScene;
	std::vector<u32> m_shapes;
	DOG::EntityManager& m_entityManager;
	std::vector<std::shared_ptr<LuaInterface>> m_luaInterfaces;
	std::shared_ptr<MainPlayer> m_player;
	std::array<u32, 4> m_playerModels;
	NetCode m_netCode;
	//Pathfinder m_pathfinder;		// uncomment to activate pathfinder
	std::shared_ptr <Agent> m_Agent;
};