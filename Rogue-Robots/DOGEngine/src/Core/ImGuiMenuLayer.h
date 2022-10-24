#pragma once

#include "../EventSystem/Layer.h"
#include "../EventSystem/IEvent.h"
#include "../Input/Keyboard.h"

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

		static void RegisterDebugWindow(const std::string& name, std::function<void(bool&)> func, bool startOpen = false, std::optional<std::pair<Key, Key>> shortCut = std::nullopt);
		static void UnRegisterDebugWindow(const std::string& name);
		static void RemoveFocus();

	private:
		static void ModelSpawner(bool& open);
		static void DemoWindow(bool& open);
#if defined _DEBUG
		static void ECSPanel(bool& open);
#endif
	private:
		struct DebugMenu
		{
			std::function<void(bool&)> implementation;
			bool open;
			std::optional<std::pair<Key, Key>> shortCut;
		};
		static std::map<std::string, DebugMenu> s_debugWindows;
		static bool s_forceFocusLoss;

		bool m_showEmptyWindow = false;
		bool m_showDemoWindow = false;
		bool m_showModelSpawnerWindow = false;
	};
}
