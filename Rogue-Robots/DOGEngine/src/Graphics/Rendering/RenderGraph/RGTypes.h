#pragma once
#include "../../RHI/RenderResourceHandles.h"
#include "../../RHI/Types/DepthTypes.h"


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
	enum class RGResourceVariant { Declared, Imported, Aliased, Proxy };

	struct RGTextureDesc
	{
		static RGTextureDesc RenderTarget2D(DXGI_FORMAT format, u32 width, u32 height, u32 depth = 1, u32 mipLevels = 1)
		{
			RGTextureDesc d{};
			d.format = format;
			d.width = width;
			d.height = height;
			d.depth = depth;
			d.mipLevels = mipLevels;
			d.initState = D3D12_RESOURCE_STATE_RENDER_TARGET;
			d.flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
			return d;
		}

		static RGTextureDesc DepthWrite2D(DepthFormat format, u32 width, u32 height, u32 depth = 1, u32 mipLevels = 1)
		{
			RGTextureDesc d{};
			switch (format)
			{
			case DepthFormat::D32:
				d.format = DXGI_FORMAT_D32_FLOAT;               // Fully qualififies the format: DXGI_FORMAT_R32_TYPELESS
				break;
			case DepthFormat::D32_S8:
				d.format = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;    // Fully qualififies the format: DXGI_FORMAT_R32G8X24_TYPELESS
				break;
			case DepthFormat::D24_S8:
				d.format = DXGI_FORMAT_D24_UNORM_S8_UINT;       // Fully qualififies the format: DXGI_FORMAT_R24G8_TYPELESS
				break;
			case DepthFormat::D16:
				d.format = DXGI_FORMAT_D16_UNORM;               // Fully qualififies the format: DXGI_FORMAT_R16_TYPELESS
				break;
			default:
				assert(false);
			}
			d.width = width;
			d.height = height;
			d.depth = depth;
			d.mipLevels = mipLevels;
			d.initState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
			d.flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
		}

		static RGTextureDesc ReadWrite2D(DXGI_FORMAT format, u32 width, u32 height, u32 depth = 1, u32 mipLevels = 1)
		{
			RGTextureDesc d{};
			d.format = format;
			d.width = width;
			d.height = height;
			d.depth = depth;
			d.mipLevels = mipLevels;
			d.initState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
			d.flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
			return d;
		}

		RGTextureDesc& AppendExtraFlag(D3D12_RESOURCE_FLAGS flag)
		{
			flags |= flag;
			return *this;
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


	static bool IsReadState(D3D12_RESOURCE_STATES states)
	{
		if ((states & D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER) == D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER ||
			(states & D3D12_RESOURCE_STATE_INDEX_BUFFER) == D3D12_RESOURCE_STATE_INDEX_BUFFER ||
			(states & D3D12_RESOURCE_STATE_DEPTH_READ) == D3D12_RESOURCE_STATE_DEPTH_READ ||
			(states & D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE) == D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE ||
			(states & D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE) == D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE ||
			(states & D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT) == D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT ||
			(states & D3D12_RESOURCE_STATE_COPY_SOURCE) == D3D12_RESOURCE_STATE_COPY_SOURCE)
		{
			return true;
		}
		return false;
	}

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