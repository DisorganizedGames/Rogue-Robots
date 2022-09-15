#pragma once
namespace DOG
{
	enum class WindowMode : uint8_t { Windowed = 0, FullScreen };

	struct ApplicationSpecification
	{
		std::string name;
		Vector2u windowDimensions;
		WindowMode initialWindowMode;
		std::string workingDir;
	};
}