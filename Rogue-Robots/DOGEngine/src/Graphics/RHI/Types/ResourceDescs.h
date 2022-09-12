#pragma once
#include <d3d12.h>

namespace DOG::gfx
{

	enum class MemoryType
	{
		Default,
		Upload,
		Readback
	};

	enum class ViewType
	{
		None,
		RenderTarget,			// RTV
		DepthStencil,			// DSV
		ShaderResource,			// SRV
		RaytracingAS,			// SRV (Raytracing Acceleration Structure)
		UnorderedAccess,		// UAV
		Constant,				// CBV
		Index					// IBV
	};

	enum class TextureViewDimension
	{
		None,
		Texture1D,
		Texture1D_Array,

		Texture2D,
		Texture2D_MS,
		Texture2D_Array,
		Texture2D_MS_Array,

		Texture3D,
		TextureCube,
		TextureCube_Array,
	};

	enum class TextureType
	{
		Texture1D,
		Texture2D,
		Texture3D
	};

	struct BufferDesc
	{
		MemoryType memType{ MemoryType::Default };
		D3D12_RESOURCE_FLAGS flags{ D3D12_RESOURCE_FLAG_NONE };
		D3D12_RESOURCE_STATES initState{ D3D12_RESOURCE_STATE_COMMON };

		u32 size{ 0 };
		u32 alignment{ 0 };

		BufferDesc() = default;
		BufferDesc(MemoryType memType, u32 size, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATES initState = D3D12_RESOURCE_STATE_COMMON) :
			memType(memType), flags(flags), initState(initState), size(size) {}
	};

	struct TextureDesc
	{
		MemoryType memType{ MemoryType::Default };
		D3D12_RESOURCE_FLAGS flags{ D3D12_RESOURCE_FLAG_NONE };
		D3D12_RESOURCE_STATES initState{ D3D12_RESOURCE_STATE_COMMON };
		TextureType type{ TextureType::Texture2D };

		DXGI_FORMAT format{ DXGI_FORMAT_UNKNOWN };
		u32 alignment{ 0 };

		u32 width{ 1 };
		u32 height{ 1 };
		u32 depth{ 1 };

		u32 mipLevels{ 0 };

		u32 sampleCount{ 1 };
		u32 sampleQuality{ 0 };

		std::array<float, 4> clearColor{ 0.f, 0.f, 0.f, 1.f };
		u8 stencilClear{ 0 };

#ifdef USE_REVERSE_Z
		float depthClear{ 0.f };
#else
		float depthClear{ 1.f };
#endif

		TextureDesc() = default;
		TextureDesc(MemoryType memType, DXGI_FORMAT format, u32 width, u32 height, u32 depth, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATES initState = D3D12_RESOURCE_STATE_COMMON) :
			memType(memType), flags(flags), initState(initState), format(format), width(width), height(height), depth(depth) {}

		TextureDesc& SetMipLevels(u32 value) { mipLevels = value; return *this; }
		TextureDesc& SetSampling(u32 count, u32 quality) { sampleCount = count; sampleQuality = quality; return *this; }
		TextureDesc& SetStencilClear(u8 value) { stencilClear = value; return *this; }

		// In normalized range [0, 1]
		TextureDesc& SetClearColor(u8 r, u8 g, u8 b) { clearColor[0] = r; clearColor[1] = g; clearColor[2] = b; };

	};

	struct BufferViewDesc
	{
		ViewType viewType{ ViewType::None };
		u32 offset{ 0 };
		u32 stride{ 0 };
		u32 count{ 1 };

		bool raw{ false };

		BufferViewDesc() = delete;
		BufferViewDesc(ViewType viewType, u32 offset, u32 stride, u32 count = 1) : viewType(viewType), offset(offset), stride(stride), count(count) {};
	};

	struct TextureViewDesc
	{
		ViewType viewType{ ViewType::None };
		TextureViewDimension viewDimension{ TextureViewDimension::None };
		DXGI_FORMAT format{ DXGI_FORMAT_UNKNOWN };

		u32 mipLevelBase{ 0 };
		u32 mipLevels{ 1 };

		u32 arrayBase{ 0 };
		u32 arrayCount{ 1 };

		float minLodClamp{ 0.f };

		// Flags to specify DS
		bool depthReadOnly{ false };
		bool stencilReadOnly{ false };

		TextureViewDesc() = delete;
		TextureViewDesc(ViewType viewType, TextureViewDimension viewDimension, DXGI_FORMAT format) : viewType(viewType), viewDimension(viewDimension), format(format) {}

		TextureViewDesc& SetArrayRange(u32 base, u32 count) { arrayBase = base; arrayCount = count; return *this; }
		TextureViewDesc& SetMipRange(u32 base, u32 count) { mipLevelBase = base; mipLevels = count; return *this; }
		TextureViewDesc& SetMinLodClamp(float value) { minLodClamp = value; return *this; }
		TextureViewDesc& SetDepthReadOnly() { depthReadOnly = true; return *this; }
		TextureViewDesc& SetStencilReadOnly() { stencilReadOnly = true; return *this; }

	};

	struct ShaderArgs
	{
		std::array<u32, 10> constants{};
		u8 numConstants{ 0 };

		ShaderArgs& AppendConstant(u32 constant) { assert(numConstants < 10); constants[numConstants++] = constant; return *this; }
	};

	struct Viewports
	{
		std::array<D3D12_VIEWPORT, 8> vps;
		u8 numVps{ 0 };

#ifdef USE_REVERSE_Z
		Viewports& Append(f32 topLeftX, f32 topLeftY, f32 width, f32 height, f32 minDepth = 0.f, f32 maxDepth = D3D12_MAX_DEPTH)
#else
		Viewports& Append(f32 topLeftX, f32 topLeftY, f32 width, f32 height, f32 minDepth = D3D12_MAX_DEPTH, f32 maxDepth = 0.f)
#endif
		{
			assert(numVps < 8);
			D3D12_VIEWPORT vp{};
			vp.TopLeftX = topLeftX;
			vp.TopLeftY = topLeftY;
			vp.Width = width;
			vp.Height = height;
			vp.MinDepth = minDepth;
			vp.MaxDepth = maxDepth;

			vps[numVps++] = vp;
			return *this;
		}
	};

	struct ScissorRects
	{
		std::array<D3D12_RECT, 8> scissors;
		u8 numScissors{ 0 };

		ScissorRects& Append(u32 left, u32 top, u32 right, u32 bottom)
		{
			D3D12_RECT rect{};
			rect.left = left;
			rect.top = top;
			rect.right = right;
			rect.bottom = bottom;
			scissors[numScissors++] = rect;

			return *this;
		}

	};

}