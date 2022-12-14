#pragma once
#include <DOGEngine.h>
#include "../Game/GameLayer.h"
#include "../UI/SettingsMenu.h"
#include "../Game/EmilFDebugLayer.h"
#include "../Pathfinder/PathfinderDebugLayer.h"
#include "../../../DOGEngine/src/Graphics/Rendering/UI.h"
class RuntimeApplication : public DOG::Application
{
public:
	explicit RuntimeApplication(const DOG::ApplicationSpecification& spec, const GameSettings& gameSettings) noexcept;
	virtual ~RuntimeApplication() noexcept override final;
	virtual void OnStartUp() noexcept override final;
	virtual void OnShutDown() noexcept override final;
	virtual void OnRestart() noexcept override final;
	virtual void OnEvent(DOG::IEvent& event) noexcept final;
	const GameSettings& GetGameSettings() const noexcept;
private:
	void IssueDebugFunctionality() noexcept;
private:
	void SettingDebugMenu(bool& open);
	bool m_showImGuiMenu = false;
	GameLayer m_gameLayer;
	DOG::ImGuiMenuLayer m_imGuiMenuLayer;
	//EmilFDebugLayer m_EmilFDebugLayer;
	//PathfinderDebugLayer m_PathfinderDebugLayer;
};
