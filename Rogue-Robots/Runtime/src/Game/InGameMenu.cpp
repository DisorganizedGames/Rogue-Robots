#include "InGameMenu.h"
#include <DOGEngine.h>
using namespace DOG;

void InGameMenu::Initialize(std::function<void(void)> exitToMainMenuCallback, std::function<void(void)> exitToDesktopCallback)
{
	InGameMenu::s_sceneID = UI::Get()->AddScene();
	auto&& inGameMenu = [exitToMainMenuCallback, exitToDesktopCallback](u32 clientWidth, u32 clientHeight) {

		auto instance = UI::Get();
		auto inGameMenuBack = instance->Create<DOG::UIBackground, float, float, std::wstring>(InGameMenu::s_backgroundID, (FLOAT)clientWidth / 2.0f, (FLOAT)clientHeight / 2.0f, std::wstring(L"InGameMenu"));
		instance->AddUIElementToScene(InGameMenu::s_sceneID, std::move(inGameMenuBack));

		auto resumeButton = instance->Create<DOG::UIButton, float, float, float, float, float, float, float, float, std::wstring>(
			InGameMenu::s_resumeButtonID, (FLOAT)clientWidth / 2.f - 75.f + 100.f, (FLOAT)clientHeight / 2.f + 40.f, 150.f, 60.f, 20.f, 1.0f, 1.0f, 1.0f,
			std::wstring(L"Resume"), []() { UI::Get()->ChangeUIscene(gameID); });
		instance->AddUIElementToScene(InGameMenu::s_sceneID, std::move(resumeButton));


		auto exitToMainMenuButton = instance->Create<DOG::UIButton, float, float, float, float, float, float, float, float, std::wstring>(
			InGameMenu::s_exitToMainMenuButtonID, (FLOAT)clientWidth / 2.f - 75.f + 100.f, (FLOAT)clientHeight / 2.f + 110.f, 150.f, 60.f, 20.f, 1.0f, 1.0f, 1.0f,
			std::wstring(L"Exit to main menu"), [&callback = exitToMainMenuCallback]() { callback(); });
		instance->AddUIElementToScene(InGameMenu::s_sceneID, std::move(exitToMainMenuButton));

		auto exitToDesktopButton = instance->Create<DOG::UIButton, float, float, float, float, float, float, float, float, std::wstring>(
			InGameMenu::s_exitToDesktopButtonID, (FLOAT)clientWidth / 2.f - 75.f + 100.f, (FLOAT)clientHeight / 2.f + 180.f, 150.f, 60.f, 20.f, 1.0f, 1.0f, 1.0f,
			std::wstring(L"Exit to desktop"), [&callback = exitToDesktopCallback]() { callback(); });
		instance->AddUIElementToScene(InGameMenu::s_sceneID, std::move(exitToDesktopButton));

	};
	UI::Get()->AddExternalUI(inGameMenu);
}

void InGameMenu::Open()
{
	UI::Get()->ChangeUIscene(InGameMenu::s_sceneID);
}
