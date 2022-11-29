#include "InGameMenu.h"
#include <DOGEngine.h>
using namespace DOG;

void InGameMenu::Initialize(std::function<void(void)> exitToMainMenuCallback, std::function<void(void)> exitToDesktopCallback)
{
	InGameMenu::s_sceneID = UI::Get()->AddScene();
	auto&& inGameMenu = [exitToMainMenuCallback, exitToDesktopCallback](u32 clientWidth, u32 clientHeight) {
		auto instance = UI::Get();

		float backWidth = 0.3f * clientWidth;
		float backHeight = 0.6f * clientHeight;
		float backLeft = (clientWidth - backWidth) / 2.0f;
		float backTop = (clientHeight - backHeight) / 2.0f;

		float buttonWidth = 0.7f * backWidth;
		float buttonHeight = 0.2f * backHeight;
		float buttonMarginY = 0.1f * buttonWidth;
		float buttonX = (clientWidth - buttonWidth) / 2.0f;
		float buttonY = backTop + 2 * buttonMarginY;

		auto resumeButton = instance->Create<DOG::UIButton>(InGameMenu::s_resumeButtonID,
			buttonX, buttonY, buttonWidth, buttonHeight, 20.f, 1.0f, 1.0f, 1.0f,
			std::wstring(L"Resume"), []() { UI::Get()->ChangeUIscene(gameID); });

		buttonY += buttonHeight + buttonMarginY;
		auto exitToMainMenuButton = instance->Create<DOG::UIButton>(InGameMenu::s_exitToMainMenuButtonID,
			buttonX, buttonY, buttonWidth, buttonHeight, 20.f, 1.0f, 1.0f, 1.0f,
			std::wstring(L"Exit to main menu"), [&callback = exitToMainMenuCallback]() { callback(); });

		buttonY += buttonHeight + buttonMarginY;
		auto exitToDesktopButton = instance->Create<DOG::UIButton>(InGameMenu::s_exitToDesktopButtonID,
			buttonX, buttonY, buttonWidth, buttonHeight, 20.f, 1.0f, 1.0f, 1.0f,
			std::wstring(L"Exit to desktop"), [&callback = exitToDesktopCallback]() { callback(); });


		backHeight = buttonY;
		auto inGameMenuBack = instance->Create<DOG::UIBackground>(InGameMenu::s_backgroundID, backWidth, backHeight, std::wstring(L""), backLeft, backTop);

		instance->AddUIElementToScene(InGameMenu::s_sceneID, std::move(inGameMenuBack));
		instance->AddUIElementToScene(InGameMenu::s_sceneID, std::move(resumeButton));
		instance->AddUIElementToScene(InGameMenu::s_sceneID, std::move(exitToMainMenuButton));
		instance->AddUIElementToScene(InGameMenu::s_sceneID, std::move(exitToDesktopButton));
	};
	UI::Get()->AddExternalUI(inGameMenu);
}

void InGameMenu::Open()
{
	UI::Get()->ChangeUIscene(InGameMenu::s_sceneID);
}
