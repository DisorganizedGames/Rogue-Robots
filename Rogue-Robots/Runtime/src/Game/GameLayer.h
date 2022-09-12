#pragma once
#include <DOGEngine.h>

#include "../DOGEngine/src/Core/DataPiper.h"
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
	virtual void OnEvent(DOG::IEvent& event) override final;
private:
	DOG::piper::PipedData m_pipedData{};

private:
	DebugCamera m_debugCam;
	
};