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
		float bloomStrength = 0.420f;
		float bloomThreshold = 1.0f;
		float gamma = 2.22f;
		bool lightCulling = true;
		bool visualizeLightCulling = false;
		bool ssao{ true };
		bool lit{ true };
		bool shadowMapping{ true };
		u32 shadowMapCapacity{ 4 };



		// Rendering limits, restart is required
		u32 maxConstantsPerFrame = 150'000;

		// Limits for uploadHeaps
		u32 maxHeapUploadSizeDefault = 4'000'000;
		u32 maxHeapUploadSizeTextures = 5'000'000;

		// Limits for MeshTable	
		u32 maxBytesPerAttribute = 4'000'000;
		u32 maxNumberOfIndices = 1'000'000;
		u32 maxTotalSubmeshes = 500;

		// Limits for MaterialTable
		u32 maxMaterialArgs = 1000;

		// Limits for LightTable
		u32 maxStaticPointLights = 512;
		u32 maxDynamicPointLights = 1024;
		u32 maxSometimesPointLights = 12;
		u32 maxStaticSpotLights = 12;
		u32 maxDynamicSpotLights = 12;
		u32 maxSometimesSpotLights = 12;

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