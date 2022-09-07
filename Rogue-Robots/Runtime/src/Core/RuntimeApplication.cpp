#include "RuntimeApplication.h"
#include <EntryPoint.h>

RuntimeApplication::RuntimeApplication(const DOG::ApplicationSpecification& spec) noexcept
	: DOG::Application{ spec }
{

}

std::unique_ptr<DOG::Application> CreateApplication() noexcept
{
	DOG::ApplicationSpecification specification = {};
	specification.Name = "Rogue Robots";
	specification.WindowDimensions = {1280.0f, 720.0f};
	specification.InitialWindowMode = DOG::WindowMode::Windowed;

	return std::make_unique<RuntimeApplication>(specification);
}
