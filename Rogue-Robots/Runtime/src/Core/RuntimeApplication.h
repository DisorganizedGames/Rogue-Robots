#pragma once
#include <DOGEngine.h>
class RuntimeApplication : public DOG::Application
{
public:
	explicit RuntimeApplication(const DOG::ApplicationSpecification& spec) noexcept;
	virtual ~RuntimeApplication() override final = default;
};
