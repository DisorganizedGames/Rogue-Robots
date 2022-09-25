#pragma once
#include "../../RHI/RenderResourceHandles.h"


namespace DOG::gfx
{
#define RG_RESOURCE(name) RGResourceID(#name)

	struct RGResourceID
	{
		std::string name = "";

		RGResourceID() = default;
		RGResourceID(const std::string& name) : name(name) {}

		operator std::string() const
		{
			return name;
		}

		bool operator==(const RGResourceID& other) const
		{
			return name == other.name;
		}

	};
	
	enum class RGTextureType { Texture1D, Texture2D, Texture3D };
	enum class RGResourceType { Buffer, Texture };
	enum class RGResourceVariant { Declared, Imported, Aliased };

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

	struct RGBufferDesc
	{
		u32 size{ 0 };
		D3D12_RESOURCE_FLAGS flags{ D3D12_RESOURCE_FLAG_NONE };
		D3D12_RESOURCE_STATES initState{ D3D12_RESOURCE_STATE_COMMON };
	};
}

/*
	We can switch this hashing function later to use some compile time hashed string
*/
namespace std
{
	template <>
	struct hash<DOG::gfx::RGResourceID>
	{
		std::size_t operator()(const DOG::gfx::RGResourceID& k) const
		{
			return hash<string>()(k.name);
			// return k.hashedString
		}
	};
}