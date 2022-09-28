#include "RuntimeApplication.h"
#include <EntryPoint.h>

void SaveRuntimeSettings() noexcept;
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
	//PushLayer(&m_EmilFDebugLayer);
}

void RuntimeApplication::OnShutDown() noexcept
{
	SaveRuntimeSettings();
}

void RuntimeApplication::OnRestart() noexcept
{
	//...
}

void RuntimeApplication::OnEvent(IEvent& event) noexcept
{
	if (event.GetEventCategory() == EventCategory::KeyboardEventCategory)
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
		}

	}
	Application::OnEvent(event);
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

void SaveRuntimeSettings() noexcept
{
	std::string fullFilePath = RUNTIME_DIR + std::string("Runtime.ini");
	std::vector<std::string> lines;
	std::ifstream inFile(fullFilePath);
	if (!inFile)
	{
		// Temporary solution until this works as intended
		constexpr const char def[] = "[WINDOW][DIMENSIONS]\n1280\n720\n\n[WINDOW][MODE]\n0";
		std::ofstream initFile(fullFilePath);
		assert(initFile && "Couldn't initialize Runtime.ini");
		initFile.write(def, sizeof(def));
		inFile.open(fullFilePath);
		assert(inFile && "Couldn't open Runtime.ini for saving");
	}
	std::string aLine;
	while (std::getline(inFile, aLine))
	{
		lines.push_back(aLine);
	}
	inFile.close();

	std::ofstream outFile(fullFilePath);
	assert(outFile.is_open() && "File does not exist.");
	for (u32 i = 0u; i < lines.size(); i++)
	{
		if (lines[i] == "[WINDOW][DIMENSIONS]")
		{
			lines[i + 1] = std::to_string(DOG::Window::GetWidth());
			lines[i + 2] = std::to_string(DOG::Window::GetHeight());
		}
		else if (lines[i] == "[WINDOW][MODE]")
		{
			lines[i + 1] = std::to_string((int)DOG::Window::GetMode());
		}
		outFile << lines[i] << std::endl;
	}
	outFile.close();
}

[[nodiscard]] DOG::ApplicationSpecification LoadRuntimeSettings() noexcept
{
	std::string fullFilePath = RUNTIME_DIR + std::string("Runtime.ini");
	std::ifstream inFile(fullFilePath);

	DOG::ApplicationSpecification spec;
	spec.name = "Rogue Robots";
	spec.workingDir = GetWorkingDirectory();

	if (inFile.is_open())
	{
		std::string readData;
		while (inFile >> readData)
		{
			if (readData == "[WINDOW][DIMENSIONS]")
			{
				inFile >> spec.windowDimensions.x;
				inFile >> spec.windowDimensions.y;
			}
			else if (readData == "[WINDOW][MODE]")
			{
				int mode = -1;
				inFile >> mode;
				spec.initialWindowMode = static_cast<DOG::WindowMode>(mode);
			}
		}
		inFile.close();
		return spec;
	}

	// This is potentially never run @fix
	spec.windowDimensions.x = 1280u;
	spec.windowDimensions.y = 720u;
	spec.initialWindowMode = DOG::WindowMode::Windowed;
	return spec;
}

std::unique_ptr<DOG::Application> CreateApplication() noexcept
{
	return std::make_unique<RuntimeApplication>(LoadRuntimeSettings());
}
