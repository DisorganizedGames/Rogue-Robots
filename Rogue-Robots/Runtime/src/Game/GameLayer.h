#pragma once
#include <DOGEngine.h>
#include "DebugCamera.h"

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
	DebugCamera m_debugCam;
	DOG::entity m_camera{ 0 };
	u64 m_redCube{ 0 };
	u64 m_greenCube{ 0 };
	u64 m_blueCube{ 0 };
	u64 m_magentaCube{ 0 };
	DOG::EntityManager& m_entityManager;
};