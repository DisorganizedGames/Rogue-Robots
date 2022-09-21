#pragma once
#include <DOGEngine.h>
#include "MainPlayer.h"

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
	u64 m_redCube{ 0 };
	u64 m_greenCube{ 0 };
	u64 m_blueCube{ 0 };
	u64 m_magentaCube{ 0 };
	DOG::EntityManager& m_entityManager;
	MainPlayer m_player;
};