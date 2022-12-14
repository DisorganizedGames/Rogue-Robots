#pragma once
#include <DOGEngine.h>

class LuaLayer : public DOG::Layer
{
public:
	LuaLayer() noexcept;
	virtual ~LuaLayer() override final;
	virtual void OnAttach() override final;
	virtual void OnDetach() override final;
	virtual void OnUpdate() override final;
	virtual void OnRender() override final;
	virtual void OnImGuiRender() override final;
	virtual void OnEvent(DOG::IEvent& event) override final;
};