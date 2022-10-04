#pragma once
#include <DOGEngine.h>
class EmilFDebugLayer : public DOG::Layer
{
public:
	EmilFDebugLayer() noexcept;
	virtual ~EmilFDebugLayer() override final = default;
	virtual void OnAttach() override final;
	virtual void OnDetach() override final;
	virtual void OnUpdate() override final;
	virtual void OnRender() override final;
	virtual void OnImGuiRender() override final;
	virtual void OnEvent(DOG::IEvent& event) override final;

private:
	DOG::EntityManager& m_entityManager;
	std::vector<std::unique_ptr<DOG::ISystem>> m_systems;
};