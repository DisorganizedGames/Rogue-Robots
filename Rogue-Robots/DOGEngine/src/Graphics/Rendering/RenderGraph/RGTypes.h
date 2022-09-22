#pragma once
#include <d3d12.h>

namespace DOG::gfx
{
	struct RGResource { friend class TypedHandlePool; u64 handle{ 0 }; };
	struct RGResourceView { friend class TypedHandlePool; u64 handle{ 0 }; };

	// Only 2D texture support for now
	enum class RGTextureType
	{
		Texture2D
	};

	// Internal use
	enum class RGResourceType
	{
		Buffer, Texture
	};

	struct RGBufferDesc
	{
		u32 size{ 0 };
		D3D12_RESOURCE_FLAGS flags{ D3D12_RESOURCE_FLAG_NONE };
		D3D12_RESOURCE_STATES initState{ D3D12_RESOURCE_STATE_COMMON };
	};

	struct RGTextureDesc
	{

		static RGTextureDesc RenderTarget2D(DXGI_FORMAT format, u32 width, u32 height, u32 mipLevels = 1)
		{
			RGTextureDesc d{};
			d.format = format;
			d.width = width;
			d.height = height;
			d.mipLevels = mipLevels;
			d.initState = D3D12_RESOURCE_STATE_RENDER_TARGET;
			d.flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
			return d;
		}

		u32 width{ 1 };
		u32 height{ 1 };
		u32 depth{ 1 };
		u32 mipLevels{ 1 };
		DXGI_FORMAT format{ DXGI_FORMAT_UNKNOWN };
		RGTextureType type{ RGTextureType::Texture2D };
		D3D12_RESOURCE_FLAGS flags{ D3D12_RESOURCE_FLAG_NONE };

		std::array<float, 4> clearColor{ 0.f, 0.f, 0.f, 1.f };
		u8 stencilClear{ 0 };

		// Reverse Z by default
		float depthClear{ 0.f };

		D3D12_RESOURCE_STATES initState{ D3D12_RESOURCE_STATE_COMMON };
	};
}