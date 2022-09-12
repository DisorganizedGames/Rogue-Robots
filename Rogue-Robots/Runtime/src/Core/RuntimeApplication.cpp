#include "RuntimeApplication.h"
#include <EntryPoint.h>

void SaveRuntimeSettings() noexcept;

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
}

void RuntimeApplication::OnShutDown() noexcept
{
	SaveRuntimeSettings();
}

void RuntimeApplication::OnRestart() noexcept
{
	//...
}

void SaveRuntimeSettings() noexcept
{
	std::string fullFilePath = RUNTIME_DIR + std::string("Runtime.ini");
	std::vector<std::string> lines;
	std::ifstream inFile(fullFilePath);
	assert(inFile.is_open() && "File does not exist.");
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

	spec.windowDimensions.x = 1280u;
	spec.windowDimensions.y = 720u;
	spec.initialWindowMode = DOG::WindowMode::Windowed;
	return spec;
}

std::unique_ptr<DOG::Application> CreateApplication() noexcept
{
	return std::make_unique<RuntimeApplication>(LoadRuntimeSettings());
}
