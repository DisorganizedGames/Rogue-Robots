#include "RuntimeApplication.h"
#include <EntryPoint.h>

void SaveRuntimeSettings() noexcept;

RuntimeApplication::RuntimeApplication(const DOG::ApplicationSpecification& spec) noexcept
	: DOG::Application{ spec }
{

}

RuntimeApplication::~RuntimeApplication()
{
	SaveRuntimeSettings();
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
	spec.Name = "Rogue Robots";
	if (inFile.is_open())
	{
		std::string readData;
		while (inFile >> readData)
		{
			if (readData == "[WINDOW][DIMENSIONS]")
			{
				inFile >> spec.WindowDimensions.x;
				inFile >> spec.WindowDimensions.y;
			}
			else if (readData == "[WINDOW][MODE]")
			{
				int mode = -1;
				inFile >> mode;
				spec.InitialWindowMode = static_cast<DOG::WindowMode>(mode);
			}
		}
		inFile.close();
		return spec;
	}

	spec.WindowDimensions.x = 1280u;
	spec.WindowDimensions.y = 720u;
	spec.InitialWindowMode = DOG::WindowMode::Windowed;
	return spec;
}

std::unique_ptr<DOG::Application> CreateApplication() noexcept
{
	return std::make_unique<RuntimeApplication>(LoadRuntimeSettings());
}
