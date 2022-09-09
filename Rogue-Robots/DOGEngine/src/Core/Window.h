#pragma once

namespace DOG
{
	enum class WindowMode : uint8_t { Windowed = 0, FullScreen };
	struct ApplicationSpecification;
	class Window
	{
	public:
		static void Initialize(const ApplicationSpecification& spec) noexcept;
		static bool OnUpdate() noexcept;
		static const u32 GetWidth() noexcept;
		static const u32 GetHeight() noexcept;
		static const std::pair<u32, u32> GetDimensions() noexcept;
		static const WindowMode GetMode() noexcept;
		static const HWND GetHandle() noexcept;
	private:
		STATIC_CLASS(Window);
	};
}