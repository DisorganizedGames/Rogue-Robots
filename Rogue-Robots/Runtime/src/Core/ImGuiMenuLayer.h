#pragma once

#include <DOGEngine.h>

class ImGuiMenuLayer : public DOG::Layer
{
public:
	ImGuiMenuLayer() noexcept;
	virtual ~ImGuiMenuLayer() override final = default;
	virtual void OnAttach() override final;
	virtual void OnDetach() override final;
	virtual void OnUpdate() override final;
	virtual void OnRender() override final;
	virtual void OnImGuiRender() override final;
	virtual void OnEvent(DOG::IEvent& event) override final;

private:
	bool m_showEmptyWindow = false;
	bool m_showDemoWindow = false;
	bool m_showModelSpawnerWindow = false;
};
