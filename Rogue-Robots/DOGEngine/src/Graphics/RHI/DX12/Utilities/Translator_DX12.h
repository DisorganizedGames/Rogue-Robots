#pragma once
#include "../../RHITypes.h"

namespace DOG::gfx
{
	inline UINT D3D12CalcSubresource(UINT mipSlice, UINT arraySlice, UINT planeSlice, UINT mipLevels, UINT arraySize)
	{
		return mipSlice + (arraySlice * mipLevels) + (planeSlice * mipLevels * arraySize);
	}

	inline D3D12_HEAP_TYPE to_internal(MemoryType memType)
	{
		switch (memType)
		{
		case MemoryType::Default:
			return D3D12_HEAP_TYPE_DEFAULT;
		case MemoryType::Upload:
			return D3D12_HEAP_TYPE_UPLOAD;
		case MemoryType::Readback:
			return D3D12_HEAP_TYPE_READBACK;
		default:
			assert(false);
		}

		return D3D12_HEAP_TYPE_DEFAULT;
	}

	inline D3D12_RESOURCE_DIMENSION to_internal(TextureType type)
	{
		switch (type)
		{
		case TextureType::Texture1D:
			return D3D12_RESOURCE_DIMENSION_TEXTURE1D;
		case TextureType::Texture2D:
			return D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		case TextureType::Texture3D:
			return D3D12_RESOURCE_DIMENSION_TEXTURE3D;
		default:
			assert(false);
		}
		return D3D12_RESOURCE_DIMENSION_UNKNOWN;
	}

	inline D3D12_SHADER_BYTECODE to_internal(const CompiledShader* compiled_shader)
	{
		D3D12_SHADER_BYTECODE bc{};
		bc.pShaderBytecode = compiled_shader->blob.data();
		bc.BytecodeLength = compiled_shader->blob.size();
		return bc;
	}




	inline D3D_PRIMITIVE_TOPOLOGY to_internal_topology(PrimitiveTopology topology, u8 numControlPatches)
	{
		switch (topology)
		{
		case PrimitiveTopology::None:
			return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
		case PrimitiveTopology::PointList:
			return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
		case PrimitiveTopology::LineList:
			return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
		case PrimitiveTopology::LineList_Adj:
			return D3D_PRIMITIVE_TOPOLOGY_LINELIST_ADJ;
		case PrimitiveTopology::LineStrip:
			return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
		case PrimitiveTopology::LineStrip_Adj:
			return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP_ADJ;
		case PrimitiveTopology::TriangleList:
			return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		case PrimitiveTopology::TriangleList_Adj:
			return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ;
		case PrimitiveTopology::TriangleStrip:
			return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
		case PrimitiveTopology::TriangleStrip_Adj:
			return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP_ADJ;
		case PrimitiveTopology::PatchList:
		{
			assert(numControlPatches > 1 && numControlPatches <= 32);
			return D3D_PRIMITIVE_TOPOLOGY(D3D_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST + (numControlPatches - 1));
		}
		default:
			return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
		}

	}

	inline D3D12_PRIMITIVE_TOPOLOGY_TYPE to_internal(PrimitiveTopology topology)
	{
		switch (topology)
		{
		case PrimitiveTopology::None:
			return D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
		case PrimitiveTopology::PointList:
			return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
		case PrimitiveTopology::LineList:
		case PrimitiveTopology::LineList_Adj:
		case PrimitiveTopology::LineStrip:
		case PrimitiveTopology::LineStrip_Adj:
			return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
		case PrimitiveTopology::TriangleList:
		case PrimitiveTopology::TriangleList_Adj:
		case PrimitiveTopology::TriangleStrip:
		case PrimitiveTopology::TriangleStrip_Adj:
			return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		case PrimitiveTopology::PatchList:
			return D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
		default:
			return D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
		}

	}

	inline D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE to_internal(RenderPassBeginAccessType access)
	{
		switch (access)
		{
		case RenderPassBeginAccessType::Clear:
			return D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR;
		case RenderPassBeginAccessType::Discard:
			return D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_DISCARD;
		case RenderPassBeginAccessType::Preserve:
			return D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE;
		default:
			return D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR;
		}
	}

	inline D3D12_RENDER_PASS_ENDING_ACCESS_TYPE to_internal(RenderPassEndingAccessType access)
	{
		switch (access)
		{
		case RenderPassEndingAccessType::Discard:
			return D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_DISCARD;
		case RenderPassEndingAccessType::Preserve:
			return D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
		case RenderPassEndingAccessType::Resolve:
			return D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_RESOLVE;
		default:
			return D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_DISCARD;
		}
	}

	inline D3D12_RENDER_PASS_FLAGS to_internal(RenderPassFlag flags)
	{
		switch (flags)
		{
		case RenderPassFlag::AllowUnorderedAccessWrites:
			return D3D12_RENDER_PASS_FLAG_ALLOW_UAV_WRITES;
		default:
			return D3D12_RENDER_PASS_FLAG_NONE;
		}
	}

	// calling api still needs to finish the begin/end access resources (clear/resources)
	D3D12_RENDER_PASS_RENDER_TARGET_DESC to_internal(const RenderPassTargetDesc& desc)
	{
		D3D12_RENDER_PASS_RENDER_TARGET_DESC api_desc{};
		api_desc.BeginningAccess.Type = to_internal(desc.beginAccess);
		api_desc.EndingAccess.Type = to_internal(desc.endAccess);
		return api_desc;
	}

	// calling api still needs to finish the begin/end access resources (clear/resources)
	D3D12_RENDER_PASS_DEPTH_STENCIL_DESC to_internal(const RenderPassDepthStencilTargetDesc& desc)
	{
		D3D12_RENDER_PASS_DEPTH_STENCIL_DESC api_desc{};
		api_desc.DepthBeginningAccess.Type = to_internal(desc.depthBeginAccess);
		api_desc.DepthEndingAccess.Type = to_internal(desc.depthEndAccess);
		api_desc.StencilBeginningAccess.Type = to_internal(desc.stencilBeginAccess);
		api_desc.StencilEndingAccess.Type = to_internal(desc.stencilEndAccess);
		return api_desc;
	}

	D3D12_RTV_DIMENSION to_internal_rtv(TextureViewDimension type)
	{
		switch (type)
		{
		case TextureViewDimension::Texture1D:
			return D3D12_RTV_DIMENSION_TEXTURE1D;
		case TextureViewDimension::Texture1D_Array:
			return D3D12_RTV_DIMENSION_TEXTURE1DARRAY;
		case TextureViewDimension::Texture2D:
			return D3D12_RTV_DIMENSION_TEXTURE2D;
		case TextureViewDimension::Texture2D_MS:
			return D3D12_RTV_DIMENSION_TEXTURE2DMS;
		case TextureViewDimension::Texture2D_MS_Array:
			return D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY;
		case TextureViewDimension::Texture2D_Array:
			return D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
		case TextureViewDimension::Texture3D:
			return D3D12_RTV_DIMENSION_TEXTURE3D;
		default:
			return D3D12_RTV_DIMENSION_UNKNOWN;
		}
	}


	D3D12_DSV_DIMENSION to_internal_dsv(TextureViewDimension type)
	{
		switch (type)
		{
		case TextureViewDimension::Texture1D:
			return D3D12_DSV_DIMENSION_TEXTURE1D;
		case TextureViewDimension::Texture1D_Array:
			return D3D12_DSV_DIMENSION_TEXTURE1DARRAY;
		case TextureViewDimension::Texture2D:
			return D3D12_DSV_DIMENSION_TEXTURE2D;
		case TextureViewDimension::Texture2D_Array:
			return D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
		case TextureViewDimension::Texture2D_MS:
			return D3D12_DSV_DIMENSION_TEXTURE2DMS;
		case TextureViewDimension::Texture2D_MS_Array:
			return D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY;
		default:
			return D3D12_DSV_DIMENSION_UNKNOWN;
		}
	}

	D3D12_SRV_DIMENSION to_internal_srv(TextureViewDimension type)
	{
		switch (type)
		{
		case TextureViewDimension::Texture1D:
			return D3D12_SRV_DIMENSION_TEXTURE1D;
		case TextureViewDimension::Texture1D_Array:
			return D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
		case TextureViewDimension::Texture2D:
			return D3D12_SRV_DIMENSION_TEXTURE2D;
		case TextureViewDimension::Texture2D_Array:
			return D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
		case TextureViewDimension::Texture3D:
			return D3D12_SRV_DIMENSION_TEXTURE3D;
		case TextureViewDimension::TextureCube:
			return D3D12_SRV_DIMENSION_TEXTURECUBE;
		case TextureViewDimension::TextureCube_Array:
			return D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
		default:
			return D3D12_SRV_DIMENSION_UNKNOWN;
		}
	}

	D3D12_UAV_DIMENSION to_internal_uav(TextureViewDimension type)
	{
		switch (type)
		{
		case TextureViewDimension::Texture1D:
			return D3D12_UAV_DIMENSION_TEXTURE1D;
		case TextureViewDimension::Texture1D_Array:
			return D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
		case TextureViewDimension::Texture2D:
			return D3D12_UAV_DIMENSION_TEXTURE2D;
		case TextureViewDimension::Texture2D_Array:
			return D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
		case TextureViewDimension::Texture3D:
			return D3D12_UAV_DIMENSION_TEXTURE3D;
		default:
			return D3D12_UAV_DIMENSION_UNKNOWN;
		}
	}



	D3D12_SHADER_RESOURCE_VIEW_DESC to_srv(const TextureViewDesc& range, u32 mipLevels, u32 arraySize, std::set<u32>* subresources)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC desc{};
		desc.Format = range.format;
		desc.ViewDimension = to_internal_srv(range.viewDimension);
		desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		switch (range.viewDimension)
		{
		case TextureViewDimension::Texture1D:
		{
			desc.Texture1D.MostDetailedMip = range.mipLevelBase;
			desc.Texture1D.MipLevels = range.mipLevels;
			desc.Texture1D.ResourceMinLODClamp = range.minLodClamp;

			for (u32 i = 0; i < range.mipLevels; ++i)
				subresources->insert(D3D12CalcSubresource(range.mipLevelBase + i, 0, 0, mipLevels, arraySize));

			break;
		}
		case TextureViewDimension::Texture1D_Array:
		{
			desc.Texture1DArray.MostDetailedMip = range.mipLevelBase;
			desc.Texture1DArray.MipLevels = range.mipLevels;
			desc.Texture1DArray.ResourceMinLODClamp = range.minLodClamp;

			desc.Texture1DArray.ArraySize = range.arrayCount;
			desc.Texture1DArray.FirstArraySlice = range.arrayBase;

			for (u32 i = 0; i < range.mipLevels; ++i)
			{
				for (u32 u = 0; u < range.arrayCount; ++u)
				{
					subresources->insert(D3D12CalcSubresource(
						range.mipLevelBase + i,
						range.arrayBase + u,
						0, mipLevels, arraySize));
				}
			}

			break;
		}
		case TextureViewDimension::Texture2D:
		{
			desc.Texture2D.MostDetailedMip = range.mipLevelBase;
			desc.Texture2D.MipLevels = range.mipLevels;
			desc.Texture2D.ResourceMinLODClamp = range.minLodClamp;

			desc.Texture2D.PlaneSlice = range.arrayBase;
			assert(range.arrayCount == 1);


			for (u32 i = 0; i < range.mipLevels; ++i)
			{
				subresources->insert(D3D12CalcSubresource(
					range.mipLevelBase + i,
					0,							// arraySlice = 0? @todo
					range.arrayBase, 
					mipLevels, arraySize));
			}

			break;
		}
		case TextureViewDimension::Texture2D_Array:
		{
			desc.Texture2DArray.MostDetailedMip = range.mipLevelBase;
			desc.Texture2DArray.MipLevels = range.mipLevels;
			desc.Texture2DArray.ResourceMinLODClamp = range.minLodClamp;

			desc.Texture2DArray.ArraySize = range.arrayCount;
			desc.Texture2DArray.FirstArraySlice = range.arrayBase;
			desc.Texture2DArray.PlaneSlice = 0;

			// continue calculating subresources..

			break;
		}
		case TextureViewDimension::Texture2D_MS:
		{
			desc.Texture2DMS.UnusedField_NothingToDefine = 0;
			break;
		}
		case TextureViewDimension::Texture2D_MS_Array:
		{
			desc.Texture2DMSArray.FirstArraySlice = range.arrayBase;
			desc.Texture2DMSArray.ArraySize = range.arrayCount;

			break;
		}
		case TextureViewDimension::Texture3D:
		{
			desc.Texture3D.MostDetailedMip = range.mipLevelBase;
			desc.Texture3D.MipLevels = range.mipLevels;
			desc.Texture3D.ResourceMinLODClamp = range.minLodClamp;

			break;
		}
		case TextureViewDimension::TextureCube:
		{
			desc.TextureCube.MostDetailedMip = range.mipLevelBase;
			desc.TextureCube.MipLevels = range.mipLevels;
			desc.TextureCube.ResourceMinLODClamp = range.minLodClamp;
			break;
		}
		case TextureViewDimension::TextureCube_Array:
		{
			desc.TextureCubeArray.MostDetailedMip = range.mipLevelBase;
			desc.TextureCubeArray.MipLevels = range.mipLevels;
			desc.TextureCubeArray.ResourceMinLODClamp = range.minLodClamp;

			// https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkImageSubresourceRange.html
			// Above is the only doc I can find for what "First2DArrayFace" means in D12 ...
			desc.TextureCubeArray.NumCubes = range.arrayCount;
			desc.TextureCubeArray.First2DArrayFace = range.arrayBase;		// Represent base WITHIN a texture cube! (array_base + i): where i % 6

			break;
		}
		default:
			assert(false);
		}

		return desc;
	}

	D3D12_RENDER_TARGET_VIEW_DESC to_rtv(const TextureViewDesc& range, u32 mipLevels, u32 arraySize, std::set<u32>* subresources = nullptr)
	{
		UNREFERENCED_PARAMETER(mipLevels);
		UNREFERENCED_PARAMETER(arraySize);
		UNREFERENCED_PARAMETER(subresources);

		D3D12_RENDER_TARGET_VIEW_DESC desc{};
		desc.Format = range.format;
		desc.ViewDimension = to_internal_rtv(range.viewDimension);
		switch (range.viewDimension)
		{
		case TextureViewDimension::Texture1D:
		{
			desc.Texture1D.MipSlice = range.mipLevelBase;

			assert(range.arrayCount == 1);
			break;
		}
		case TextureViewDimension::Texture1D_Array:
		{
			desc.Texture1DArray.FirstArraySlice = range.arrayBase;
			desc.Texture1DArray.ArraySize = range.arrayCount;
			desc.Texture1DArray.MipSlice = range.mipLevelBase;
			assert(range.mipLevels == 1);

			break;
		}
		case TextureViewDimension::Texture2D:
		{
			desc.Texture2D.MipSlice = range.mipLevelBase;
			desc.Texture2D.PlaneSlice = 0;

			assert(range.arrayCount == 1);
			break;
		}
		case TextureViewDimension::Texture2D_Array:
		{
			desc.Texture2DArray.FirstArraySlice = range.arrayBase;
			desc.Texture2DArray.ArraySize = range.arrayCount;
			desc.Texture2DArray.MipSlice = range.mipLevelBase;

			desc.Texture2DArray.PlaneSlice = 0;

			assert(range.mipLevels == 1);

			break;
		}
		case TextureViewDimension::Texture2D_MS:
		{
			desc.Texture2DMS.UnusedField_NothingToDefine = 0;
			break;
		}
		case TextureViewDimension::Texture2D_MS_Array:
		{
			desc.Texture2DMSArray.FirstArraySlice = range.arrayBase;
			desc.Texture2DMSArray.ArraySize = range.arrayCount;
			break;
		}
		case TextureViewDimension::Texture3D:
		{
			desc.Texture3D.MipSlice = range.mipLevelBase;

			desc.Texture3D.FirstWSlice = range.arrayBase;
			desc.Texture3D.WSize = range.arrayCount;

			assert(range.mipLevels == 1);

			break;
		}
		default:
			assert(false);
		}

		return desc;
	}

	D3D12_DEPTH_STENCIL_VIEW_DESC to_dsv(const TextureViewDesc& range, u32 mipLevels, u32 arraySize, std::set<u32>* subresources = nullptr)
	{
		UNREFERENCED_PARAMETER(mipLevels);
		UNREFERENCED_PARAMETER(arraySize);
		UNREFERENCED_PARAMETER(subresources);

		D3D12_DEPTH_STENCIL_VIEW_DESC desc{};
		desc.Format = range.format;
		desc.ViewDimension = to_internal_dsv(range.viewDimension);
		desc.Flags = D3D12_DSV_FLAG_NONE;
		if (range.depthReadOnly)
			desc.Flags |= D3D12_DSV_FLAG_READ_ONLY_DEPTH;
		if (range.stencilReadOnly)
			desc.Flags |= D3D12_DSV_FLAG_READ_ONLY_STENCIL;

		switch (range.viewDimension)
		{
		case TextureViewDimension::Texture1D:
		{
			desc.Texture1D.MipSlice = range.mipLevelBase;

			assert(range.mipLevels == 1);

			break;
		}
		case TextureViewDimension::Texture1D_Array:
		{
			desc.Texture1DArray.MipSlice = range.mipLevelBase;
			desc.Texture1DArray.FirstArraySlice = range.arrayBase;
			desc.Texture1DArray.ArraySize = range.arrayCount;

			assert(range.mipLevels == 1);

			break;
		}
		case TextureViewDimension::Texture2D:
		{
			desc.Texture2D.MipSlice = range.mipLevelBase;

			assert(range.mipLevels == 1);

			break;
		}
		case TextureViewDimension::Texture2D_Array:
		{
			desc.Texture2DArray.MipSlice = range.mipLevelBase;
			desc.Texture2DArray.FirstArraySlice = range.arrayBase;
			desc.Texture2DArray.ArraySize = range.arrayCount;

			assert(range.mipLevels == 1);

			break;
		}
		case TextureViewDimension::Texture2D_MS:
		{
			desc.Texture2DMS.UnusedField_NothingToDefine = 0;

			break;
		}
		case TextureViewDimension::Texture2D_MS_Array:
		{
			desc.Texture2DMSArray.FirstArraySlice = range.arrayBase;
			desc.Texture2DMSArray.ArraySize = range.arrayCount;
			break;
		}

		default:
			assert(false);
		}

		return desc;
	}

	D3D12_UNORDERED_ACCESS_VIEW_DESC to_uav(const TextureViewDesc& range, u32 mipLevels, u32 arraySize, std::set<u32>* subresources = nullptr)
	{
		UNREFERENCED_PARAMETER(mipLevels);
		UNREFERENCED_PARAMETER(arraySize);
		UNREFERENCED_PARAMETER(subresources);

		D3D12_UNORDERED_ACCESS_VIEW_DESC desc{};
		desc.Format = range.format;
		desc.ViewDimension = to_internal_uav(range.viewDimension);

		assert(range.mipLevels == 1);

		switch (range.viewDimension)
		{
		case TextureViewDimension::Texture1D:
		{
			desc.Texture1D.MipSlice = range.mipLevelBase;

			break;
		}
		case TextureViewDimension::Texture1D_Array:
		{
			desc.Texture1DArray.MipSlice = range.mipLevelBase;
			desc.Texture1DArray.FirstArraySlice = range.arrayBase;
			desc.Texture1DArray.ArraySize = range.arrayCount;


			break;
		}
		case TextureViewDimension::Texture2D:
		{
			desc.Texture2D.MipSlice = range.mipLevelBase;
			desc.Texture2D.PlaneSlice = 0;

			break;
		}
		case TextureViewDimension::Texture2D_Array:
		{
			desc.Texture2DArray.FirstArraySlice = range.arrayBase;
			desc.Texture2DArray.ArraySize = range.arrayCount;

			desc.Texture2DArray.MipSlice = range.mipLevelBase;
			desc.Texture2DArray.PlaneSlice = 0;


			break;
		}
		case TextureViewDimension::Texture3D:
		{
			desc.Texture3D.MipSlice = range.mipLevelBase;
			desc.Texture3D.FirstWSlice = range.arrayBase;
			desc.Texture3D.WSize = range.arrayCount;

			break;
		}
		default:
			assert(false);
		}

		return desc;
	}

	inline D3D12_GRAPHICS_PIPELINE_STATE_DESC to_internal(const GraphicsPipelineDesc& desc, ID3D12RootSignature* rsig)
	{
		// Limited support, for now
		D3D12_GRAPHICS_PIPELINE_STATE_DESC apiDesc{};
		apiDesc.pRootSignature = rsig;
		apiDesc.VS = to_internal(desc.vs);
		apiDesc.PS = to_internal(desc.ps);
		if (desc.gs)
			apiDesc.GS = to_internal(desc.gs);
		if (desc.ds)
			apiDesc.DS = to_internal(desc.ds);
		if (desc.hs)
			apiDesc.HS = to_internal(desc.hs);
		apiDesc.RasterizerState = desc.rasterizer;
		apiDesc.DepthStencilState = desc.depthStencil;

		apiDesc.SampleMask = desc.sampleMask;
		apiDesc.BlendState = desc.blendState;

		apiDesc.SampleDesc.Count = desc.sampleCount;
		apiDesc.SampleDesc.Quality = desc.sampleQuality;

		apiDesc.PrimitiveTopologyType = to_internal(desc.topology);

		apiDesc.DSVFormat = desc.dsvFormat;
		apiDesc.NumRenderTargets = desc.numRenderTargets;
		for (u32 i = 0; i < desc.numRenderTargets; ++i)
			apiDesc.RTVFormats[i] = desc.rtvFormats[i];

		apiDesc.StreamOutput = {};
		apiDesc.InputLayout = {};
		apiDesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
		apiDesc.NodeMask = 0;
		apiDesc.CachedPSO = {};
		apiDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
		return apiDesc;
	}

	inline D3D12_COMPUTE_PIPELINE_STATE_DESC to_internal_compute(const ComputePipelineDesc& desc, ID3D12RootSignature* rsig)
	{
		D3D12_COMPUTE_PIPELINE_STATE_DESC apiDesc{};

		assert(desc.cs->shaderType == ShaderType::Compute);
		apiDesc.CS = to_internal(desc.cs);
		apiDesc.pRootSignature = rsig;

		return apiDesc;
	}
}
