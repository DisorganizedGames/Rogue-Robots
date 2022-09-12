#pragma once
#include "ShaderTypes.h"
#include <d3d12.h>

namespace DOG::gfx
{
	enum class PrimitiveTopology
	{
		None,

		PointList,

		TriangleList,
		TriangleList_Adj,

		TriangleStrip,
		TriangleStrip_Adj,

		LineList,
		LineList_Adj,

		LineStrip,
		LineStrip_Adj,

		PatchList			// To select the patch points --> Pass in PipelineStateDesc!
	};


	struct GraphicsPipelineDesc
	{
		const CompiledShader* vs{ nullptr };
		const CompiledShader* gs{ nullptr };
		const CompiledShader* ds{ nullptr };
		const CompiledShader* hs{ nullptr };
		const CompiledShader* ps{ nullptr };

		// Blend
		D3D12_BLEND_DESC blendState{};
		u32 sampleMask{ UINT_MAX };

		D3D12_RASTERIZER_DESC rasterizer{};
		D3D12_DEPTH_STENCIL_DESC depthStencil{};
		PrimitiveTopology topology{ PrimitiveTopology::TriangleList };

		u8 numRenderTargets{ 0 };
		std::array<DXGI_FORMAT, 8> rtvFormats{ DXGI_FORMAT_UNKNOWN };		// Max 8 render targets
		DXGI_FORMAT dsvFormat{ DXGI_FORMAT_UNKNOWN };

		// Multisampling
		u8 sampleCount{ 1 };
		u8 sampleQuality{ 0 };

		u32 numControlPatches{ 3 };

	};
}