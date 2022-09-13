#pragma once
#include "../EventSystem/EventPublisher.h"
#include "CoreUtils.h"
namespace DOG
{
	struct ApplicationSpecification;
	class Window : public EventPublisher
	{
	public:
		static void Initialize(const ApplicationSpecification& spec) noexcept;
		static void OnUpdate() noexcept;
		static const u32 GetWidth() noexcept;
		static const u32 GetHeight() noexcept;
		static const std::pair<u32, u32> GetDimensions() noexcept;
		static const WindowMode GetMode() noexcept;
		static const HWND GetHandle() noexcept;
	private:
		static LRESULT WindowProcedure(HWND windowHandle, UINT message, WPARAM wParam, LPARAM lParam);
		STATIC_CLASS(Window);
	};


}