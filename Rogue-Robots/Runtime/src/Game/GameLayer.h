#pragma once
#include "MainPlayer.h"
#include "LuaInterfaces.h"
#include "NetCode.h"
#include "Pathfinder/Pathfinder.h"
#include "Agent.h"
#include "GameComponent.h"
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
	void LoadLevel(); //Loads a PCG generated level.
	void Input(DOG::Key key);
	void Release(DOG::Key key);
	void CameraUpdate();
private:
	u32 m_mixamo{ 0 };
	u32 m_redCube{ 0 };
	u32 m_greenCube{ 0 };
	u32 m_blueCube{ 0 };
	u32 m_magentaCube{ 0 };
	std::vector<u32> m_shapes;
	DOG::EntityManager& m_entityManager;
	std::vector<std::shared_ptr<LuaInterface>> m_luaInterfaces;
	std::shared_ptr<MainPlayer> m_player;
	NetCode m_netCode;
	//Pathfinder m_pathfinder;		// uncomment to activate pathfinder
	std::shared_ptr <Agent> m_Agent;

	entity m_movingPointLight;
	f64 m_elapsedTime{ 0.f };
};