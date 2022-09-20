#include <d3d12.h>

namespace DOG::gfx
{
	struct RGTexture { friend class TypedHandlePool; u64 handle{ 0 }; };
	struct RGBuffer { friend class TypedHandlePool; u64 handle{ 0 }; };
	struct RGTextureView { friend class TypedHandlePool; u64 handle{ 0 }; };
	struct RGBufferView { friend class TypedHandlePool; u64 handle{ 0 }; };

	// Only 2D texture support for now
	enum class RGTextureType
	{
		Texture2D
	};

	struct RGBufferDesc
	{
		u32 size{ 0 };
		D3D12_RESOURCE_STATES initState{ D3D12_RESOURCE_STATE_COMMON };
	};

	struct RGTextureDesc
	{
		u32 width{ 1 };
		u32 height{ 1 };
		u32 depth{ 1 };
		u32 mipLevels{ 1 };
		DXGI_FORMAT format{ DXGI_FORMAT_UNKNOWN };
		RGTextureType type{ RGTextureType::Texture2D };

		std::array<float, 4> clearColor{ 0.f, 0.f, 0.f, 1.f };
		u8 stencilClear{ 0 };

		// Reverse Z by default
		float depthClear{ 0.f };

		D3D12_RESOURCE_STATES initState{ D3D12_RESOURCE_STATE_COMMON };
	};
}