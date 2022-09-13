#pragma once
#include <DOGEngine.h>
class GameLayer : public DOG::Layer
{
public:
	GameLayer() noexcept;
	virtual ~GameLayer() override final = default;
	virtual void OnAttach() override final;
	virtual void OnDetach() override final;
	virtual void OnUpdate() override final;
	virtual void OnRender() override final;
	virtual void OnEvent(DOG::IEvent& event) override final;
private:

};