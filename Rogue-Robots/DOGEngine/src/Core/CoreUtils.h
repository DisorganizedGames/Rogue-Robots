#pragma once
namespace DOG
{
	enum class WindowMode : uint8_t { Windowed = 0, FullScreen };

	#define DEBUG_SETTING_LIT 1
	#define DEBUG_SETTING_LIGHT_CULLING 2
	#define DEBUG_SETTING_LIGHT_CULLING_VISUALIZATION 4

	struct GraphicsSettings
	{
		WindowMode windowMode = WindowMode::Windowed;
		Vector2u renderResolution{ 1920, 1080 };
		std::optional<DXGI_MODE_DESC> displayMode = std::nullopt;
		bool vSync = false;
		bool bloom = true;
		float bloomThreshold = 1.0f;
		float gamma = 2.22f;
		bool lightCulling = true;
		bool visualizeLightCulling = false;
		bool ssao{ true };
#if defined _DEBUG
		bool lit{ false };
		bool shadowMapping{ false };
#else
		bool lit{ true };
		bool shadowMapping{ true };
#endif
		u32 shadowMapCapacity{ 4 };
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

	static [[nodiscard]] float Lerp(float a, float b, float t) noexcept
	{
		return (1.0f - t) * a + b * t;
	}

	static [[nodiscard]] float InverseLerp(float a, float b, float v) noexcept
	{
		return (v - a) / (b - a);
	}

	static [[nodiscard]] float Remap(float iMin, float iMax, float oMin, float oMax, float v) noexcept
	{
		float t = InverseLerp(iMin, iMax, v);
		return Lerp(oMin, oMax, t);
	}

}