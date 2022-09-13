#pragma once
#include "../EventSystem/LayerStack.h"
#include "CoreUtils.h"
namespace DOG
{
	class ApplicationManager
	{
	public:
		static [[nodiscard]] const bool ShouldRestartApplication() noexcept;
	private:
		static bool s_shouldRestart;
		static bool s_initialStartupDone;
	};

	class Application
	{
	public:
		explicit Application(const ApplicationSpecification& spec) noexcept;
		virtual ~Application() noexcept;
		void Run() noexcept;
		virtual void OnRestart() noexcept;
		virtual void OnEvent(IEvent& event) noexcept;
	protected:
		virtual void OnStartUp() noexcept;
		virtual void OnShutDown() noexcept;
		void PushLayer(Layer* layer) noexcept;
		void PushOverlay(Layer* layer) noexcept;
		void PopLayer(Layer* layer) noexcept;
		void PopOverlay(Layer* layer) noexcept;
	private:
		DELETE_COPY_MOVE_CONSTRUCTOR(Application);
		ApplicationSpecification m_specification;
		LayerStack& m_layerStack;
		bool m_isRunning;
	};
}

[[nodiscard]] std::unique_ptr<DOG::Application> CreateApplication() noexcept;
