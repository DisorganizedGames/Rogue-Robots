#include "RuntimeApplication.h"
#include <EntryPoint.h>

RuntimeApplication::RuntimeApplication(const DOG::ApplicationSpecification& spec) noexcept
	: DOG::Application{ spec }
{

}

const std::unique_ptr<DOG::Application> CreateApplication() noexcept
{
	DOG::ApplicationSpecification specification;

	return std::make_unique<RuntimeApplication>(specification);
}
