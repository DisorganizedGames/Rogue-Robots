#pragma once
#include <DOGEngine.h>
#include "../Game/GameLayer.h"
#include "../Game/EmilFDebugLayer.h"
class RuntimeApplication : public DOG::Application
{
public:
	explicit RuntimeApplication(const DOG::ApplicationSpecification& spec) noexcept;
	virtual ~RuntimeApplication() noexcept override final;
	virtual void OnStartUp() noexcept override final;
	virtual void OnShutDown() noexcept override final;
	virtual void OnRestart() noexcept override final;
	virtual void OnEvent(IEvent& event) noexcept final;

private:
	bool m_showImGuiMenu = false;
	GameLayer m_gameLayer;
	ImGuiMenuLayer m_imGuiMenuLayer;
	EmilFDebugLayer m_EmilFDebugLayer;
};
