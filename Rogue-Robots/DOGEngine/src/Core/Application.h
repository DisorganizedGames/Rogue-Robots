#pragma once
#include "Window.h"
#include "../EventSystem/LayerStack.h"
namespace DOG
{
	class Layer;
	struct ApplicationSpecification
	{
		std::string name;
		Vector2u windowDimensions;
		WindowMode initialWindowMode;
	};

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
	protected:
		virtual void OnStartUp() noexcept;
		virtual void OnShutDown() noexcept;
		void PushLayer(Layer* layer) noexcept;
		void PushOverlay(Layer* layer) noexcept;
		void PopLayer(Layer* layer) noexcept;
		void PopOverlay(Layer* layer) noexcept;
	private:
		ApplicationSpecification m_specification;
		LayerStack m_layerStack;
		bool m_isRunning;
	};
}

[[nodiscard]] std::unique_ptr<DOG::Application> CreateApplication() noexcept;
