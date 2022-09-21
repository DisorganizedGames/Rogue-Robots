#pragma once
#include <DOGEngine.h>
class EmilDebugLayer : public DOG::Layer
{
public:
	EmilDebugLayer() noexcept;
	virtual ~EmilDebugLayer() override final = default;
	virtual void OnAttach() override final;
	virtual void OnDetach() override final;
	virtual void OnUpdate() override final;
	virtual void OnRender() override final;
	virtual void OnImGuiRender() override final;
	virtual void OnEvent(DOG::IEvent& event) override final;

private:
	DOG::EntityManager& m_entityManager;
};