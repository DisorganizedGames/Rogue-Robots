#pragma once

namespace DOG
{
	enum class WindowMode : uint8_t { Windowed = 0, FullScreen };
	struct ApplicationSpecification;
	class Window
	{
	public:
		void Initialize(const ApplicationSpecification& spec) noexcept;
	private:
		STATIC_CLASS(Window);

	};
}