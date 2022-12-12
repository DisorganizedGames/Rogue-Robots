#include "SettingsMenu.h"

using namespace DOG;

DOG::GraphicsSettings SettingsMenu::s_graphicsSettings;

std::function<void(const GraphicsSettings&)> SettingsMenu::s_setGraphicsSettings;
std::function<GraphicsSettings(void)> SettingsMenu::s_getGraphicsSettings;

void SettingsMenu::Initialize(
	std::function<void(const GraphicsSettings&)> setGraphicsSettings,
	std::function<GraphicsSettings(void)> getGraphicsSettings
)
{
	s_setGraphicsSettings = setGraphicsSettings;
	s_getGraphicsSettings = getGraphicsSettings;
	s_graphicsSettings = s_getGraphicsSettings();



	auto&& settingsMenu = [](u32 clientWidth, u32 clientHeight) {

		auto instance = UI::Get();

		Vector3f color{ 1,1,1 };
		float backWidth = static_cast<float>(clientWidth);
		float backHeight = static_cast<float>(clientHeight);

		float leftStart = backWidth / 3;
		float topStart = backHeight / 4;

		float textSize = 0.016f * 1920;
		float checkBoxSize = 0.016f * 1920;
		float buttonWidth = 0.12f * 1920;
		float buttonHeight = 0.08f * 1080;
		float sliderWidth = 0.08f * 1920;
		float sliderHeight = 0.02f * 1080;

		float paddingY = 0.01f * 1080;
		float x = leftStart;
		float y = topStart;

		auto inGameMenuBack = instance->Create<DOG::UIBackground>(SettingsMenu::s_backgroundID, backWidth, backHeight, std::wstring(L""));
		instance->AddUIElementToScene(optionsID, std::move(inGameMenuBack));


		auto displaySettingsLabel = instance->Create<DOG::UILabel>(s_displaySettingsLabelID, std::wstring(L"display settings"), x, y, 16 * textSize, textSize, 30.f, DWRITE_TEXT_ALIGNMENT_LEADING);
		instance->AddUIElementToScene(optionsID, std::move(displaySettingsLabel));
		y += checkBoxSize + paddingY;

		auto fullscreenCheckBoxLabel = instance->Create<DOG::UILabel>(s_fullscreenLabelID, std::wstring(L"fullscreen"), x + 2 * checkBoxSize, y, 10 * textSize, textSize, 20.f, DWRITE_TEXT_ALIGNMENT_LEADING);
		auto fullscreenCheckBox = instance->Create<DOG::UICheckBox>(s_fullscreenCheckBoxID,
			x, y, checkBoxSize, checkBoxSize, color.x, color.y, color.z,
			[](bool checked) { s_graphicsSettings.windowMode = checked ? WindowMode::FullScreen : WindowMode::Windowed; s_setGraphicsSettings(s_graphicsSettings); });
		instance->AddUIElementToScene(optionsID, std::move(fullscreenCheckBoxLabel));
		instance->AddUIElementToScene(optionsID, std::move(fullscreenCheckBox));

		y += checkBoxSize + paddingY;

		auto vsyncCheckBoxLabel = instance->Create<DOG::UILabel>(s_vsyncLabelID, std::wstring(L"v-sync"), x + 2 * checkBoxSize, y, 6 * textSize, textSize, 20.f, DWRITE_TEXT_ALIGNMENT_LEADING);
		auto vsyncCheckBox = instance->Create<DOG::UICheckBox>(s_vsyncCheckBoxID,
			x, y, checkBoxSize, checkBoxSize, color.x, color.y, color.z,
			[](bool checked) { s_graphicsSettings.vSync = checked; s_setGraphicsSettings(s_graphicsSettings); });
		instance->AddUIElementToScene(optionsID, std::move(vsyncCheckBoxLabel));
		instance->AddUIElementToScene(optionsID, std::move(vsyncCheckBox));

		y += 1.5f * checkBoxSize + paddingY;

		auto graphicsSettingsLabel = instance->Create<DOG::UILabel>(s_graphicsSettingsLabelID, std::wstring(L"graphics settings"), x, y, 16 * textSize, textSize, 30.f, DWRITE_TEXT_ALIGNMENT_LEADING);
		instance->AddUIElementToScene(optionsID, std::move(graphicsSettingsLabel));
		y += checkBoxSize + paddingY;

		auto shadowMappingCheckBoxLabel = instance->Create<DOG::UILabel>(s_shadowMappingLabelID, std::wstring(L"shadow mapping"), x + 2 * checkBoxSize, y, 13 * textSize, textSize, 20.f, DWRITE_TEXT_ALIGNMENT_LEADING);
		auto shadowMappingCheckBox = instance->Create<DOG::UICheckBox>(s_shadowMappingCheckBoxID,
			x, y, checkBoxSize, checkBoxSize, color.x, color.y, color.z,
			[](bool checked) { s_graphicsSettings.shadowMapping = checked; s_setGraphicsSettings(s_graphicsSettings); });
		instance->AddUIElementToScene(optionsID, std::move(shadowMappingCheckBoxLabel));
		instance->AddUIElementToScene(optionsID, std::move(shadowMappingCheckBox));

		y += checkBoxSize + paddingY;

		auto ssaoCheckBoxLabel = instance->Create<DOG::UILabel>(s_ssaoLabelID, std::wstring(L"ssao"), x + 2 * checkBoxSize, y, 4 * textSize, textSize, 20.f, DWRITE_TEXT_ALIGNMENT_LEADING);
		auto ssaoCheckBox = instance->Create<DOG::UICheckBox>(s_ssaoCheckBoxID,
			x, y, checkBoxSize, checkBoxSize, color.x, color.y, color.z,
			[](bool checked) { s_graphicsSettings.ssao = checked; s_setGraphicsSettings(s_graphicsSettings); });
		instance->AddUIElementToScene(optionsID, std::move(ssaoCheckBoxLabel));
		instance->AddUIElementToScene(optionsID, std::move(ssaoCheckBox));

		y += checkBoxSize + paddingY;

		auto bloomCheckBoxLabel = instance->Create<DOG::UILabel>(s_bloomLabelID, std::wstring(L"bloom"), x + 2 * checkBoxSize, y, 4 * textSize, textSize, 20.f, DWRITE_TEXT_ALIGNMENT_LEADING);
		auto bloomCheckBox = instance->Create<DOG::UICheckBox>(s_bloomCheckBoxID,
			x, y, checkBoxSize, checkBoxSize, color.x, color.y, color.z,
			[](bool checked) { s_graphicsSettings.bloom = checked; s_setGraphicsSettings(s_graphicsSettings); });
		instance->AddUIElementToScene(optionsID, std::move(bloomCheckBoxLabel));
		instance->AddUIElementToScene(optionsID, std::move(bloomCheckBox));

		y += checkBoxSize + paddingY;

		auto bloomSliderLabel = instance->Create<DOG::UILabel>(s_bloomSliderLabelID, std::wstring(L"bloom strength"), x + sliderWidth, y, 13 * textSize, textSize, 20.f, DWRITE_TEXT_ALIGNMENT_LEADING);
		auto bloomSlider = instance->Create<DOG::UISlider>(s_bloomSliderID,
			x, y, sliderWidth, sliderHeight,
			[](float value) { s_graphicsSettings.bloomStrength = value; s_setGraphicsSettings(s_graphicsSettings); });
		instance->AddUIElementToScene(optionsID, std::move(bloomSliderLabel));
		instance->AddUIElementToScene(optionsID, std::move(bloomSlider));

		y += sliderHeight + 2 * paddingY;

		auto backButton = instance->Create<DOG::UIButton>(SettingsMenu::s_backButtonID,
			x, y, buttonWidth, buttonHeight, 20.f, color.x, color.y, color.z,
			std::wstring(L"Back"), []() { UI::Get()->ChangeUIscene(menuID); });
		y += buttonHeight + paddingY;
		

		instance->AddUIElementToScene(optionsID, std::move(backButton));
	};
	UI::Get()->AddExternalUI(settingsMenu);

	SettGraphicsSettings(s_graphicsSettings);
}

void SettingsMenu::SettGraphicsSettings(const DOG::GraphicsSettings& settings)
{
	s_graphicsSettings = settings;
	UI::Get()->GetUI<UICheckBox>(s_fullscreenCheckBoxID)->SetValue(s_graphicsSettings.windowMode == WindowMode::FullScreen);
	UI::Get()->GetUI<UICheckBox>(s_vsyncCheckBoxID)->SetValue(s_graphicsSettings.vSync);
	UI::Get()->GetUI<UICheckBox>(s_ssaoCheckBoxID)->SetValue(s_graphicsSettings.ssao);
	UI::Get()->GetUI<UICheckBox>(s_bloomCheckBoxID)->SetValue(s_graphicsSettings.bloom);
	UI::Get()->GetUI<UICheckBox>(s_shadowMappingCheckBoxID)->SetValue(s_graphicsSettings.shadowMapping);
}
