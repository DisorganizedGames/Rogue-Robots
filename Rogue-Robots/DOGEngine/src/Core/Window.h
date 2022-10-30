#pragma once
#include "../EventSystem/EventPublisher.h"
#include "CoreUtils.h"
struct ImFont;
namespace DOG
{
	class Application;
	struct ApplicationSpecification;
	class Window : public EventPublisher
	{
		friend Application;
	public:
		static void Initialize(const ApplicationSpecification& spec) noexcept;
		static bool OnUpdate() noexcept;
		static const u32 GetWidth() noexcept;
		static const u32 GetHeight() noexcept;
		static const std::pair<u32, u32> GetDimensions() noexcept;
		static const WindowMode GetMode() noexcept;
		static const RECT GetWindowRect() noexcept;
		static const HWND GetHandle() noexcept;
		static void SetCursorMode(CursorMode mode) noexcept;
		static CursorMode GetCursorMode() noexcept;
		static ImFont* GetFont() noexcept;
		static void SetFont(ImFont* pFont) noexcept;

		// Reserved for renderer until further notice (no functionality for WM callbacks is implemented for Window otherwise)
		static void SetWMHook(const std::function<LRESULT(HWND, UINT, WPARAM, LPARAM)> func);

	private:
		static LRESULT WindowProcedure(HWND windowHandle, UINT message, WPARAM wParam, LPARAM lParam);
		STATIC_CLASS(Window);
	};


}