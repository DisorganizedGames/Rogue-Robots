#pragma once
#include <DOGEngine.h>
#include "../Game/GameLayer.h"
class RuntimeApplication : public DOG::Application
{
public:
	explicit RuntimeApplication(const DOG::ApplicationSpecification& spec) noexcept;
	virtual ~RuntimeApplication() noexcept override final;
	virtual void OnStartUp() noexcept override final;
	virtual void OnShutDown() noexcept override final;
	virtual void OnRestart() noexcept override final;
private:
	GameLayer m_gameLayer;
};
