#pragma once
namespace DOG
{
	enum class WindowMode : uint8_t { Windowed = 0, FullScreen };

	struct GraphicsSettings
	{
		WindowMode windowMode = WindowMode::Windowed;
		Vector2u renderResolution{ 1920, 1080 };
		std::optional<DXGI_MODE_DESC> displayMode = std::nullopt;
		bool vSync = false;
		bool bloom = true;
		float bloomThreshold = 0.5f;
	};


	struct ApplicationSpecification
	{
		std::string name;
		Vector2u windowDimensions{ 1280, 720 };
		WindowMode initialWindowMode;
		std::string workingDir;
		GraphicsSettings graphicsSettings;
	};

	enum class CursorMode
	{
		Visible = 1,
		Confined = 2
	};


	inline CursorMode operator ~(CursorMode m)
	{
		return (CursorMode)~(int)m;
	}
	inline CursorMode operator &(CursorMode l, CursorMode r)
	{
		return (CursorMode)((int)l & (int)r);
	}
	inline CursorMode operator |(CursorMode l, CursorMode r)
	{
		return (CursorMode)((int)l | (int)r);
	}
}