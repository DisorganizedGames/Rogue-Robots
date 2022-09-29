#pragma once

#include "../EventSystem/Layer.h"
#include "../EventSystem/IEvent.h"

namespace DOG
{

	class ImGuiMenuLayer : public Layer
	{
	public:
		ImGuiMenuLayer() noexcept;
		virtual ~ImGuiMenuLayer() override final = default;
		virtual void OnAttach() override final;
		virtual void OnDetach() override final;
		virtual void OnUpdate() override final;
		virtual void OnRender() override final;
		virtual void OnImGuiRender() override final;
		virtual void OnEvent(IEvent& event) override final;

	private:
		bool m_showEmptyWindow = false;
		bool m_showDemoWindow = false;
		bool m_showModelSpawnerWindow = false;
	};
}
