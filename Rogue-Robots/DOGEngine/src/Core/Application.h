#pragma once
#include "../EventSystem/LayerStack.h"
#include "CoreUtils.h"
#include "../Graphics/RHI/Types/HardwareTypes.h"
namespace DOG
{
	namespace gfx { class Renderer; class FrontRenderer; }

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
		void SetGraphicsSettings(const GraphicsSettings& settings) noexcept;
		GraphicsSettings GetGraphicsSettings() const noexcept;
		gfx::Monitor GetMonitor() const noexcept;
	protected:
		virtual void OnStartUp() noexcept;
		virtual void OnShutDown() noexcept;
		void PushLayer(Layer* layer) noexcept;
		void PushOverlay(Layer* layer) noexcept;
		void PopLayer(Layer* layer) noexcept;
		void PopOverlay(Layer* layer) noexcept;
		const ApplicationSpecification& GetApplicationSpecification() const noexcept;
		Vector2u GetAspectRatio() const noexcept;
		void ApplyGraphicsSettings() noexcept;
	private:
		DELETE_COPY_MOVE_CONSTRUCTOR(Application);
		ApplicationSpecification m_specification;
		WindowMode m_fullscreenStateOnFocusLoss;
		CursorMode m_cursorModeOnFocusLoss;
		LayerStack& m_layerStack;
		bool m_isRunning;

		std::unique_ptr<gfx::Renderer> m_renderer;
		std::unique_ptr<gfx::FrontRenderer> m_frontRenderer;
	};
}

[[nodiscard]] std::unique_ptr<DOG::Application> CreateApplication() noexcept;
