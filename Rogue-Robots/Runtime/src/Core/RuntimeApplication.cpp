#include "RuntimeApplication.h"
#include <EntryPoint.h>
using namespace DOG;
void SaveRuntimeSettings(const ApplicationSpecification& spec, const GameSettings& gameSettings, const std::string& path) noexcept;
std::string GetWorkingDirectory();


RuntimeApplication::RuntimeApplication(const DOG::ApplicationSpecification& spec, const GameSettings& gameSettings) noexcept
	: DOG::Application{ spec }
{
	m_gameLayer.SetGameSettings(gameSettings);
	OnStartUp();
}

RuntimeApplication::~RuntimeApplication()
{
	OnShutDown();
}

void RuntimeApplication::OnStartUp() noexcept
{
	PushLayer(&m_gameLayer);
	PushOverlay(DOG::UI::Get());
	//PushLayer(&m_EmilFDebugLayer);
	//PushLayer(&m_PathfinderDebugLayer);
	ImGuiMenuLayer::RegisterDebugWindow("GraphicsSetting", [this](bool& open) { SettingDebugMenu(open); }, false, std::make_pair(Key::LCtrl, Key::V));


	SettingsMenu::Initialize(
		[this](auto settings) {
			SetGraphicsSettings(settings);
		},
		[this]() {
			return GetGraphicsSettings();
		},
		[this]() {
			return GetAspectRatio();
		},
		[this](auto settings) {
			SetAudioSettings(settings);
		},
		[this]() {
			return GetAudioSettings();
		},
		[this](auto settings) {
			m_gameLayer.SetGameSettings(settings);
		},
		[this]() {
			return m_gameLayer.GetGameSettings();
		}
	);


	#if defined _DEBUG
	IssueDebugFunctionality();
	#endif
}

void RuntimeApplication::OnShutDown() noexcept
{
	//SaveRuntimeSettings();
	ImGuiMenuLayer::UnRegisterDebugWindow("GraphicsSetting");
	SaveRuntimeSettings(GetApplicationSpecification(), m_gameLayer.GetGameSettings(), "Settings.lua");
}

void RuntimeApplication::OnRestart() noexcept
{
	//...
}

void RuntimeApplication::OnEvent(IEvent& event) noexcept
{
	if (event.GetEventCategory() == EventCategory::KeyboardEventCategory
		|| event.GetEventCategory() == EventCategory::MouseEventCategory)
	{
		switch (event.GetEventType())
		{
		case EventType::KeyPressedEvent:
			{
				if (EVENT(KeyPressedEvent).key == Key::F1)
				{
					m_showImGuiMenu = !m_showImGuiMenu;
					if (m_showImGuiMenu)
					{
						PushOverlay(&m_imGuiMenuLayer);

					}
					else
					{
						PopOverlay(&m_imGuiMenuLayer);
					}
				}
				if (EVENT(KeyPressedEvent).key == Key::F2)
				{
					static bool lockMouse = false;
					if (lockMouse)
					{
						Window::SetCursorMode(CursorMode::Confined);
						if (m_showImGuiMenu) m_imGuiMenuLayer.RemoveFocus();
					}
					else
					{
						Window::SetCursorMode(CursorMode::Visible);
					}

					lockMouse = !lockMouse;
					event.StopPropagation();
				}
				if (EVENT(KeyPressedEvent).key == Key::Esc)
				{
					if (SettingsMenu::IsOpen())
					{
						SettingsMenu::Close();
						event.StopPropagation();
					}
				}
				break;
			}
		}
	}

	if (event.GetEventType() == EventType::WindowPostResizedEvent)
	{
		SettingsMenu::SetGraphicsSettings(GetGraphicsSettings());
		SettingsMenu::SetAudioSettings(GetAudioSettings());
		SettingsMenu::SetGameSettings(m_gameLayer.GetGameSettings());
	}
	Application::OnEvent(event);
}

const GameSettings& RuntimeApplication::GetGameSettings() const noexcept
{
	return m_gameLayer.GetGameSettings();
}

void RuntimeApplication::IssueDebugFunctionality() noexcept
{
	//Have no spotlight casts shadows by default in debug:
	EntityManager::Get().Collect<DOG::SpotLightComponent, DOG::ShadowCasterComponent>().Do([](entity spotlight, DOG::SpotLightComponent&, DOG::ShadowCasterComponent&) 
		{
			EntityManager::Get().RemoveComponent<DOG::ShadowCasterComponent>(spotlight);
		});
}

std::string GetWorkingDirectory()
{
	std::string currentWorkSpace = std::filesystem::current_path().string();
	std::string workSpaceInBuildTime = PROJECT_WORKSPACE;
	std::string binBuildTime = PROJECT_BIN;

	std::filesystem::path binRelativeToWorkSpace = binBuildTime.substr(workSpaceInBuildTime.length());
	binRelativeToWorkSpace = binRelativeToWorkSpace.make_preferred(); // On Windows, Converts a/b -> a\\b

	if (currentWorkSpace.find(binRelativeToWorkSpace.string()) != std::string::npos)
	{
		// Remove binRelativeToWorkSpace from currentWorkSpace
		return currentWorkSpace.substr(0, currentWorkSpace.length() - binRelativeToWorkSpace.string().length());
	}
	else
	{
		std::cout << binRelativeToWorkSpace.string() << " is not part of the path to the exe";
		return currentWorkSpace;
	}
}


static std::string oldWorkDir = "";
void SaveRuntimeSettings(const ApplicationSpecification& spec, const GameSettings& gameSettings, const std::string& path) noexcept
{
	// This funtion will need revert our working directory.
	if(oldWorkDir != "") std::filesystem::current_path(oldWorkDir);

	std::ofstream outFile(path);
	outFile << "--Delete file to reset all options to default (don't run with an empty file, it will not work)";
	outFile << "\n--Delete rows to reset them to default";
	outFile << "\nSettings =\n{";

	outFile << "\n\t" << "masterVolume = " << spec.audioSettings.masterVolume;
	outFile << ",\n\t" << "mouseSensitivity = " << gameSettings.mouseSensitivity;
	outFile << ",\n\t" << "fullscreen = " << static_cast<int>(spec.graphicsSettings.windowMode);
	outFile << ",\n\t" << "clientWidth = " << spec.windowDimensions.x;
	outFile << ",\n\t" << "clientHeight = " << spec.windowDimensions.y;
	outFile << ",\n\t" << "renderResolutionWidth = " << spec.graphicsSettings.renderResolution.x;
	outFile << ",\n\t" << "renderResolutionHeight = " << spec.graphicsSettings.renderResolution.y;
	outFile << ",\n\t" << "vsync = " << (spec.graphicsSettings.vSync ? "true" : "false");
	outFile << ",\n\t" << "ssao = " << (spec.graphicsSettings.ssao ? "true" : "false");
	outFile << ",\n\t" << "shadowMapping = " << (spec.graphicsSettings.shadowMapping ? "true" : "false");
	outFile << ",\n\t" << "shadowMapCapacity = " << spec.graphicsSettings.shadowMapCapacity;
	outFile << ",\n\t" << "bloom = " << (spec.graphicsSettings.bloom ? "true" : "false");
	outFile << ",\n\t" << "bloomTreshold = " << spec.graphicsSettings.bloomThreshold;
	outFile << ",\n\t" << "bloomStrength = " << spec.graphicsSettings.bloomStrength;
	outFile << ",\n\t" << "lit = " << (spec.graphicsSettings.lit ? "true" : "false");
	outFile << ",\n\t" << "gamma = " << spec.graphicsSettings.gamma;

	if (spec.graphicsSettings.displayMode)
	{
		const auto& mode = *spec.graphicsSettings.displayMode;
		outFile << ",\n\n\t--DXGI_MODE_DESC";
		outFile << "\n\t" << "displayWidth = " << mode.Width;
		outFile << ",\n\t" << "displayHeight = " << mode.Height;
		outFile << ",\n\t" << "refreshRateNumerator = " << mode.RefreshRate.Numerator;
		outFile << ",\n\t" << "refreshRateDenominator = " << mode.RefreshRate.Denominator;
		outFile << ",\n\t" << "format = " << mode.Format;
		outFile << ",\n\t" << "scanLine = " << mode.ScanlineOrdering;
		outFile << ",\n\t" << "scaling = " << mode.Scaling;
	}

	outFile << ",\n\n\t--Rendering limits";
	outFile << "\n\t" << "maxStaticPointLights = " << spec.graphicsSettings.maxStaticPointLights;
	outFile << ",\n\t" << "maxDynamicPointLights = " << spec.graphicsSettings.maxDynamicPointLights;
	outFile << ",\n\t" << "maxSometimesPointLights = " << spec.graphicsSettings.maxSometimesPointLights;
	outFile << ",\n\t" << "maxStaticSpotLights = " << spec.graphicsSettings.maxStaticSpotLights;
	outFile << ",\n\t" << "maxDynamicSpotLights = " << spec.graphicsSettings.maxDynamicSpotLights;
	outFile << ",\n\t" << "maxSometimesSpotLights = " << spec.graphicsSettings.maxSometimesSpotLights;
	outFile << ",\n\t" << "maxMaterialArgs = " << spec.graphicsSettings.maxMaterialArgs;
	outFile << ",\n\t" << "maxTotalSubmeshes = " << spec.graphicsSettings.maxTotalSubmeshes;
	outFile << ",\n\t" << "maxNumberOfIndices = " << spec.graphicsSettings.maxNumberOfIndices;
	outFile << ",\n\t" << "maxBytesPerAttribute = " << spec.graphicsSettings.maxBytesPerAttribute;
	outFile << ",\n\t" << "maxHeapUploadSizeDefault = " << spec.graphicsSettings.maxHeapUploadSizeDefault;
	outFile << ",\n\t" << "maxHeapUploadSizeTextures = " << spec.graphicsSettings.maxHeapUploadSizeTextures;
	outFile << ",\n\t" << "maxConstantsPerFrame = " << spec.graphicsSettings.maxConstantsPerFrame;
	
	outFile << "\n}\n";
}

[[nodiscard]] std::pair<ApplicationSpecification, GameSettings> LoadRuntimeSettings(const std::string& path) noexcept
{
	ApplicationSpecification appSpec;
	GameSettings gameSettings;
	if (!std::filesystem::exists(path))
	{
		SaveRuntimeSettings(appSpec, gameSettings, path);
	}

	LuaTable table;
	if (!table.TryCreateEnvironment(path))
	{
		LuaTable spec = table.GetTableFromTable("Settings");

		auto&& tryGetSpec = [&](const auto& key, auto& value)
		{
			bool succeeded = spec.TryGetValueFromTable(key, value);
			if (!succeeded) std::cout << key << " is missing value" << std::endl;
			return succeeded;
		};

		bool err = false;
		err |= !tryGetSpec("masterVolume", appSpec.audioSettings.masterVolume);
		appSpec.audioSettings.masterVolume = std::clamp(appSpec.audioSettings.masterVolume, 0.0f, 1.0f);
		err |= !tryGetSpec("mouseSensitivity", gameSettings.mouseSensitivity);
		err |= !tryGetSpec("clientWidth", appSpec.windowDimensions.x);
		err |= !tryGetSpec("clientHeight", appSpec.windowDimensions.y);
		err |= !tryGetSpec("renderResolutionWidth", appSpec.graphicsSettings.renderResolution.x);
		err |= !tryGetSpec("renderResolutionHeight", appSpec.graphicsSettings.renderResolution.y);
		err |= !tryGetSpec("vsync", appSpec.graphicsSettings.vSync);
		err |= !tryGetSpec("ssao", appSpec.graphicsSettings.ssao);
		err |= !tryGetSpec("shadowMapping", appSpec.graphicsSettings.shadowMapping);
		err |= !tryGetSpec("shadowMapCapacity", appSpec.graphicsSettings.shadowMapCapacity);
		err |= !tryGetSpec("fullscreen", (int&)appSpec.graphicsSettings.windowMode);
		err |= !tryGetSpec("bloom", appSpec.graphicsSettings.bloom);
		err |= !tryGetSpec("bloomTreshold", appSpec.graphicsSettings.bloomThreshold);
		err |= !tryGetSpec("bloomStrength", appSpec.graphicsSettings.bloomStrength);
		err |= !tryGetSpec("lit", appSpec.graphicsSettings.lit);

		// Rendering limits
		err |= !tryGetSpec("maxStaticPointLights", appSpec.graphicsSettings.maxStaticPointLights);
		err |= !tryGetSpec("maxDynamicPointLights", appSpec.graphicsSettings.maxDynamicPointLights);
		err |= !tryGetSpec("maxSometimesPointLights", appSpec.graphicsSettings.maxSometimesPointLights);
		err |= !tryGetSpec("maxStaticSpotLights", appSpec.graphicsSettings.maxStaticSpotLights);
		err |= !tryGetSpec("maxDynamicSpotLights", appSpec.graphicsSettings.maxDynamicSpotLights);
		err |= !tryGetSpec("maxSometimesSpotLights", appSpec.graphicsSettings.maxSometimesSpotLights);
		err |= !tryGetSpec("maxMaterialArgs", appSpec.graphicsSettings.maxMaterialArgs);
		err |= !tryGetSpec("maxTotalSubmeshes", appSpec.graphicsSettings.maxTotalSubmeshes);
		err |= !tryGetSpec("maxNumberOfIndices", appSpec.graphicsSettings.maxNumberOfIndices);
		err |= !tryGetSpec("maxBytesPerAttribute", appSpec.graphicsSettings.maxBytesPerAttribute);
		err |= !tryGetSpec("maxHeapUploadSizeDefault", appSpec.graphicsSettings.maxHeapUploadSizeDefault);
		err |= !tryGetSpec("maxHeapUploadSizeTextures", appSpec.graphicsSettings.maxHeapUploadSizeTextures);
		err |= !tryGetSpec("maxConstantsPerFrame", appSpec.graphicsSettings.maxConstantsPerFrame);

		bool modeErr = false;
		appSpec.graphicsSettings.displayMode = DXGI_MODE_DESC{};
		modeErr |= !tryGetSpec("scanLine", (int&)appSpec.graphicsSettings.displayMode->ScanlineOrdering);
		modeErr |= !tryGetSpec("format", (int&)appSpec.graphicsSettings.displayMode->Format);
		modeErr |= !tryGetSpec("scaling", (int&)appSpec.graphicsSettings.displayMode->Scaling);
		modeErr |= !tryGetSpec("refreshRateNumerator", appSpec.graphicsSettings.displayMode->RefreshRate.Numerator);
		modeErr |= !tryGetSpec("refreshRateDenominator", appSpec.graphicsSettings.displayMode->RefreshRate.Denominator);
		modeErr |= !tryGetSpec("displayWidth", appSpec.graphicsSettings.displayMode->Width);
		modeErr |= !tryGetSpec("displayHeight", appSpec.graphicsSettings.displayMode->Height);

		if (modeErr)
		{
			// If the saved mode is broken, set null and let the app query a new one from the renderer later on
			appSpec.graphicsSettings.displayMode = std::nullopt;
		}

		if (err || modeErr)
		{
			std::cout << path << " is missing some values, they will be replaced with defaults" << std::endl;
			SaveRuntimeSettings(appSpec, gameSettings, path);
		}
	}
	else
	{
		SaveRuntimeSettings(appSpec, gameSettings, path);
	}

	return { appSpec, gameSettings };
}

void RuntimeApplication::SettingDebugMenu(bool& open)
{
	if (ImGui::BeginMenu("View"))
	{
		if (ImGui::MenuItem("Graphics settings", "Ctrl + V"))
		{
			open = true;
		}
		ImGui::EndMenu(); // "View"
	}

	if (open)
	{
		if (ImGui::Begin("Application settings", &open, ImGuiWindowFlags_NoFocusOnAppearing))
		{
			GraphicsSettings graphicsSettings = GetGraphicsSettings();

			gfx::Monitor monitor = GetMonitor();

			ImGui::Text("Display settings");
			ImGui::Text(std::filesystem::path(monitor.output.DeviceName).string().c_str());

			int left = monitor.output.DesktopCoordinates.left;
			int right = monitor.output.DesktopCoordinates.right;
			int top = monitor.output.DesktopCoordinates.top;
			int bottom = monitor.output.DesktopCoordinates.bottom;

			std::string rectX = "left: " + std::to_string(left) + ", right: " + std::to_string(right);
			std::string rectY = "top: " + std::to_string(top) + ", bottom : " + std::to_string(bottom);
			ImGui::Text("Rect");
			ImGui::Text(rectX.c_str());
			ImGui::Text(rectY.c_str());


			auto&& modeElementToString = [&monitor](i64 index) -> std::string
			{
				std::string str = "resolution: " + std::to_string(monitor.modes[index].Width) + "x" + std::to_string(monitor.modes[index].Height);
				str += ", hz: " + std::to_string(static_cast<float>(monitor.modes[index].RefreshRate.Numerator) / monitor.modes[index].RefreshRate.Denominator);
				str += ", Format: " + std::to_string(monitor.modes[index].Format);
				str += ", scanline: " + std::to_string(monitor.modes[index].ScanlineOrdering);
				str += ", scaling: " + std::to_string(monitor.modes[index].Scaling);

				UINT c = std::gcd(monitor.modes[index].Width, monitor.modes[index].Height);
				UINT w = monitor.modes[index].Width / c;
				UINT h = monitor.modes[index].Height / c;
				str += ", aspect ratio: " + std::to_string(w) + "/" + std::to_string(h);

				return str;
			};

			static i64 selectedModeIndex = std::ssize(monitor.modes) - 1;
			selectedModeIndex = std::min(selectedModeIndex, static_cast<i64>(std::ssize(monitor.modes) - 1));

			static bool firstTime = true;
			if (firstTime)
			{
				selectedModeIndex = [&]()->i64 {
					for (int i = 1; i < monitor.modes.size() - 1; i++)
					{
						auto& other = monitor.modes[i];
						auto& current = *graphicsSettings.displayMode;

						if (current.Format == other.Format && current.RefreshRate.Denominator == other.RefreshRate.Denominator
							&& current.RefreshRate.Numerator == other.RefreshRate.Numerator && current.Height == other.Height
							&& current.Width == other.Width && current.Scaling == other.Scaling && current.ScanlineOrdering == other.ScanlineOrdering)
						{
							return i;
						}
					}
					return selectedModeIndex;
				}();
			}

			bool gfxChanged{ false };
			if (ImGui::BeginCombo("modes", modeElementToString(selectedModeIndex).c_str()))
			{
				for (i64 i = std::ssize(monitor.modes) - 1; i >= 0; i--)
				{
					if (ImGui::Selectable(modeElementToString(i).c_str(), selectedModeIndex == i))
					{
						selectedModeIndex = i;
						graphicsSettings.displayMode = monitor.modes[selectedModeIndex];
						gfxChanged = true;
					}
				}
				ImGui::EndCombo();
			}

			static int selectedFullscreenStateIndex = 0;
			//selectedFullscreenStateIndex = static_cast<int>(GetFullscreenState());
			selectedFullscreenStateIndex = static_cast<int>(graphicsSettings.windowMode);
			std::array<const char*, 2> fullscreenCombo = { "Windowed", "Fullscreen" };
			if (ImGui::Combo("Fullscreen mode", &selectedFullscreenStateIndex, fullscreenCombo.data(), static_cast<int>(fullscreenCombo.size())))
			{
				graphicsSettings.windowMode = static_cast<WindowMode>(selectedFullscreenStateIndex);
				gfxChanged = true;
			}

			
			if (ImGui::Checkbox("Vsync", &graphicsSettings.vSync))
				gfxChanged = true;

			ImGui::Separator();

			ImGui::Text("Graphics settings");




			static std::vector<std::string> res =
			{
				"144",
				"360",
				"720",
				"1080",
				"1440",
				"2160",
			};
			static int resIndex = 3;

			if (firstTime)
			{
				resIndex = [&]()->int {
					for (int i = 1; i < res.size() - 1; i++)
						if (graphicsSettings.renderResolution.y == static_cast<u32>(std::stoi(res[i]))) return i;

					res.push_back(std::to_string(graphicsSettings.renderResolution.y));
					return static_cast<int>(res.size() - 1);
				}();

			}


			ImGui::Text("resolution");
			ImGui::SameLine();

			Vector2u resolutionRatio = GetAspectRatio();
			auto&& resToString = [&](int index) -> std::string
			{
				std::string resX = std::to_string(std::stoi(res[index]) * resolutionRatio.x / resolutionRatio.y);
				return resX + "x" + res[index];
			};

			if (ImGui::BeginCombo("res", resToString(resIndex).c_str()))
			{
				for (int i = 0; i < std::size(res); i++)
				{
					if (ImGui::Selectable(resToString(i).c_str(), resIndex == i))
					{
						resIndex = i;
						graphicsSettings.renderResolution.y = std::stoi(res[resIndex]);
						graphicsSettings.renderResolution.x = graphicsSettings.renderResolution.y * resolutionRatio.x / resolutionRatio.y;
						gfxChanged = true;
					}
				}
				ImGui::EndCombo();
			}

			if (ImGui::Checkbox("Bloom", &graphicsSettings.bloom))
				gfxChanged = true;

			if (ImGui::SliderFloat("BloomThreshold", &graphicsSettings.bloomThreshold, 0.1f, 3))
				gfxChanged = true;

			if (ImGui::SliderFloat("BloomStrength", &graphicsSettings.bloomStrength, 0.0f, 2))
				gfxChanged = true;

			if (ImGui::Checkbox("SSAO", &graphicsSettings.ssao))
				gfxChanged = true;

			if (ImGui::Checkbox("Shadow Mapping", &graphicsSettings.shadowMapping))
			{
				gfxChanged = true;
			}

			if (ImGui::SliderFloat("Gamma", &graphicsSettings.gamma, 1.0f, 5.0f))
				gfxChanged = true;

			ImGui::SameLine();
			if (ImGui::Button("Default"))
			{
				graphicsSettings.gamma = 2.22f;
				gfxChanged = true;
			}

			if (ImGui::Checkbox("Lit", &graphicsSettings.lit))
				gfxChanged = true;

			if (ImGui::Checkbox("Light culling", &graphicsSettings.lightCulling))
				gfxChanged = true;

			if (ImGui::Checkbox("Visualize light culling", &graphicsSettings.visualizeLightCulling))
				gfxChanged = true;

			if (gfxChanged)
			{
				SetGraphicsSettings(graphicsSettings);
				SettingsMenu::SetGraphicsSettings(GetGraphicsSettings());
			}

			firstTime = false;

			//--------------
		}
		ImGui::End(); // "Graphics settings"
	}
}

std::unique_ptr<DOG::Application> CreateApplication() noexcept
{
	ApplicationSpecification spec;
	GameSettings gameSettings;
	std::tie(spec, gameSettings) = LoadRuntimeSettings(std::string("Settings.lua"));
	spec.name = "Rogue Robots";
	spec.workingDir = GetWorkingDirectory();
	oldWorkDir = std::filesystem::absolute(std::filesystem::current_path()).string();
	return std::make_unique<RuntimeApplication>(spec, gameSettings);
}