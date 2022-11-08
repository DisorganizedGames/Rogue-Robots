#include "RuntimeApplication.h"
#include <EntryPoint.h>
using namespace DOG;
void SaveRuntimeSettings(const ApplicationSpecification& spec, const std::string& path) noexcept;
std::string GetWorkingDirectory();


RuntimeApplication::RuntimeApplication(const DOG::ApplicationSpecification& spec) noexcept
	: DOG::Application{ spec }
{
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

	#if defined _DEBUG
	IssueDebugFunctionality();
	#endif
}

void RuntimeApplication::OnShutDown() noexcept
{
	//SaveRuntimeSettings();
	SaveRuntimeSettings(GetApplicationSpecification(), "Runtime/Settings.lua");
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
				break;

			}
		case EventType::RightMouseButtonPressedEvent:
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
				break;
			}
		}

	}
	Application::OnEvent(event);
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

void SaveRuntimeSettings(const ApplicationSpecification& spec, const std::string& path) noexcept
{
	std::ofstream outFile(path);
	outFile << "Settings =\n{";

	outFile << "\n\t" << "fullscreen = " << static_cast<int>(spec.graphicsSettings.windowMode);
	outFile << ",\n\t" << "clientWidth = " << spec.windowDimensions.x;
	outFile << ",\n\t" << "clientHeight = " << spec.windowDimensions.y;
	outFile << ",\n\t" << "renderResolutionWidth = " << spec.graphicsSettings.renderResolution.x;
	outFile << ",\n\t" << "renderResolutionHeight = " << spec.graphicsSettings.renderResolution.y;
	outFile << ",\n\t" << "vsync = " << (spec.graphicsSettings.vSync ? "true" : "false");
	outFile << ",\n\t" << "bloom = " << (spec.graphicsSettings.bloom ? "true" : "false");
	outFile << ",\n\t" << "bloomTreshold = " << spec.graphicsSettings.bloomThreshold;

	if (spec.graphicsSettings.displayMode)
	{
		const auto& mode = *spec.graphicsSettings.displayMode;
		outFile << ",\n\n\t--DXGI_MODE_DESC";
		outFile << ",\n\t" << "displayWidth = " << mode.Width;
		outFile << ",\n\t" << "displayHeight = " << mode.Height;
		outFile << ",\n\t" << "refreshRateNumerator = " << mode.RefreshRate.Numerator;
		outFile << ",\n\t" << "refreshRateDenominator = " << mode.RefreshRate.Denominator;
		outFile << ",\n\t" << "format = " << mode.Format;
		outFile << ",\n\t" << "scanLine = " << mode.ScanlineOrdering;
		outFile << ",\n\t" << "scaling = " << mode.Scaling;
	}

	outFile << "\n}\n";
}

[[nodiscard]] ApplicationSpecification LoadRuntimeSettings(const std::string& path) noexcept
{
	ApplicationSpecification appSpec;
	if (!std::filesystem::exists(path))
	{
		SaveRuntimeSettings(appSpec, path);
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
		err |= !tryGetSpec("clientWidth", appSpec.windowDimensions.x);
		err |= !tryGetSpec("clientHeight", appSpec.windowDimensions.y);
		err |= !tryGetSpec("renderResolutionWidth", appSpec.graphicsSettings.renderResolution.x);
		err |= !tryGetSpec("renderResolutionHeight", appSpec.graphicsSettings.renderResolution.y);
		err |= !tryGetSpec("vsync", appSpec.graphicsSettings.vSync);
		err |= !tryGetSpec("fullscreen", (int&)appSpec.graphicsSettings.windowMode);
		err |= !tryGetSpec("bloom", appSpec.graphicsSettings.bloom);
		err |= !tryGetSpec("bloomTreshold", appSpec.graphicsSettings.bloomThreshold);

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
			SaveRuntimeSettings(appSpec, path);
		}
	}
	else
	{
		SaveRuntimeSettings(appSpec, path);
	}
	return appSpec;
}

std::unique_ptr<DOG::Application> CreateApplication() noexcept
{
	ApplicationSpecification spec = LoadRuntimeSettings(RUNTIME_DIR + std::string("Settings.lua"));
	spec.name = "Rogue Robots";
	spec.workingDir = GetWorkingDirectory();
	return std::make_unique<RuntimeApplication>(spec);
}