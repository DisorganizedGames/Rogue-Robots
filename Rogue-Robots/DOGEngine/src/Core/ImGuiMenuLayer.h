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

		static void RegisterDebugWindow(const std::string& name, std::function<void(bool&)> func, bool startOpen = false);
		static void UnRegisterDebugWindow(const std::string& name);

	private:
		static void ModelSpawner(bool& open);
		static void DemoWindow(bool& open);
#if defined _DEBUG
		static void ECSPanel(bool& open);
#endif
	private:
		static std::map<std::string, std::pair<std::function<void(bool&)>, bool>> s_debugWindows;

		bool m_showEmptyWindow = false;
		bool m_showDemoWindow = false;
		bool m_showModelSpawnerWindow = false;
	};
}
