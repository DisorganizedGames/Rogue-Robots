#include "RenderDevice_DX12.h"
#include "Swapchain_DX12.h"
#include "CommonIncludes_DX12.h"

#include "Utilities/D3D12MemAlloc.h"


#include "Utilities/Translator_DX12.h"
#include "Utilities/DX12DescriptorManager.h"
#include "Utilities/DX12Queue.h"
#include "Utilities/DX12Fence.h"

#include "Tracy/Tracy.hpp"

#include "D11Device.h"

namespace DOG::gfx
{
	GPUPoolMemoryInfo ToMemoryInfo(const D3D12MA::DetailedStatistics& stats)
	{
		GPUPoolMemoryInfo info{};
		info.blocksAllocated = stats.Stats.BlockCount;
		info.numAllocations = stats.Stats.AllocationCount;
		info.blockBytes = (u32)stats.Stats.BlockBytes;
		info.allocationBytes = (u32)stats.Stats.AllocationBytes;
		info.allocatedButUnusedBytes = info.blockBytes - info.allocationBytes;

		info.numUnusedRange = stats.UnusedRangeCount;
		info.smallestAllocation = stats.AllocationSizeMin;
		info.largestAllocation = stats.AllocationSizeMax;
		info.smallestUnusedRange = stats.UnusedRangeSizeMin;
		info.largestUnusedRange = stats.UnusedRangeSizeMax;
		return info;
	};


	RenderDevice_DX12::RenderDevice_DX12(ComPtr<ID3D12Device5> device, IDXGIAdapter* adapter, bool debug, UINT numBackBuffers) :
		m_device(device),
		m_debugOn(debug)
	{
		// 0 marked as un-used
		m_buffers.resize(1);
		m_textures.resize(1);
		m_pipelines.resize(1);
		m_renderPasses.resize(1);
		m_syncs.resize(1);
		m_cmdls.resize(1);
		m_bufferViews.resize(1);
		m_textureViews.resize(1);
		m_memoryPools.resize(1);

		CreateQueues();
		InitDMA(adapter);

		m_descriptorMgr = std::make_unique<DX12DescriptorManager>(m_device.Get());
		

		m_reservedDescriptor = m_descriptorMgr->allocate(1, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_d2dReservedDescriptor = m_descriptorMgr->allocate(numBackBuffers, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		InitRootsig();

		InitializeD11(m_device.Get(), GetQueue(QueueType::Graphics));
	}

	RenderDevice_DX12::~RenderDevice_DX12()
	{
		RenderDevice_DX12::Flush();

		for (auto& view : m_bufferViews)
		{
			if (view.has_value())
			{
				m_descriptorMgr->free(&(*view).view);
			}
		}

		for (auto& view : m_textureViews)
		{
			if (view.has_value())
			{
				m_descriptorMgr->free(&(*view).view);
				if (view->uavClear)
					m_descriptorMgr->free_cbv_srv_uav_cpu(&(*view->uavClear));
			}
		}


		if (m_reservedDescriptor)
			m_descriptorMgr->free(&(*m_reservedDescriptor));
		
		if (m_d2dReservedDescriptor)
			m_descriptorMgr->free(&(*m_d2dReservedDescriptor));

		DestroyD11();
	}

	Swapchain* RenderDevice_DX12::CreateSwapchain(void* hwnd, u8 numBuffers)
	{
		m_swapchain = std::make_unique<Swapchain_DX12>(this, (HWND)hwnd, numBuffers, m_debugOn);
		return m_swapchain.get();
	}

	Monitor RenderDevice_DX12::GetMonitor()
	{
		Monitor monitor;
		monitor.modes = m_swapchain->GetModeDescs(m_swapchain->GetBufferFormat());
		monitor.output = m_swapchain->GetOutputDesc();
		return monitor;
	}

	const GPUPoolMemoryInfo& RenderDevice_DX12::GetPoolMemoryInfo(MemoryPool pool)
	{
		auto& storage = HandleAllocator::TryGet(m_memoryPools, HandleAllocator::GetSlot(pool.handle));

		D3D12MA::DetailedStatistics stats{};
		storage.pool->CalculateStatistics(&stats);
		storage.info = ToMemoryInfo(stats);
	
		return storage.info;
	}

	const GPUTotalMemoryInfo& RenderDevice_DX12::GetTotalMemoryInfo()
	{	
		D3D12MA::TotalStatistics stats;
		m_dma->CalculateStatistics(&stats);
		
		for (u32 i = 0; i < 4; ++i)
			m_totalMemoryInfo.heap[i] = ToMemoryInfo(stats.HeapType[i]);
		m_totalMemoryInfo.total = ToMemoryInfo(stats.Total);

		return m_totalMemoryInfo;
	}

	u64 RenderDevice_DX12::GetTotalTextureSize(const TextureDesc& desc)
	{
		D3D12_RESOURCE_DESC rd{};
		rd.Dimension = to_internal(desc.type);
		rd.Alignment = desc.alignment;
		rd.Width = desc.width;
		rd.Height = desc.height;
		rd.DepthOrArraySize = (u16)desc.depth;
		rd.MipLevels = (u16)desc.mipLevels;
		rd.Format = desc.format;
		rd.SampleDesc.Count = desc.sampleCount;
		rd.SampleDesc.Quality = desc.sampleQuality;
		rd.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		rd.Flags = desc.flags;

		auto allocInfo = m_device->GetResourceAllocationInfo(0, 1, &rd);
		return allocInfo.SizeInBytes;
	}

	Buffer RenderDevice_DX12::CreateBuffer(const BufferDesc& desc, MemoryPool pool)
	{
		HRESULT hr{ S_OK };

		D3D12MA::ALLOCATION_DESC ad{};
		ad.HeapType = to_internal(desc.memType);
		if (pool.handle != 0)
		{
			auto poolStorage = HandleAllocator::TryGet(m_memoryPools, HandleAllocator::GetSlot(pool.handle));
			ad.CustomPool = poolStorage.pool.Get();
			ad.Flags = D3D12MA::ALLOCATION_FLAG_STRATEGY_BEST_FIT;
			//ad.Flags |= D3D12MA::ALLOCATION_FLAG_NEVER_ALLOCATE;		// If we want tighter constraints, we can enable this
			assert(ad.HeapType == poolStorage.desc.heapType);
		}

		D3D12_RESOURCE_DESC rd{};
		rd.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		rd.Alignment = desc.alignment;
		rd.Width = desc.size;
		rd.Height = rd.DepthOrArraySize = rd.MipLevels = 1;
		rd.Format = DXGI_FORMAT_UNKNOWN;
		rd.SampleDesc.Count = 1;
		rd.SampleDesc.Quality = 0;
		rd.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		rd.Flags = desc.flags;

		D3D12_RESOURCE_STATES initState{ desc.initState };
		if (ad.HeapType == D3D12_HEAP_TYPE_UPLOAD)
			initState = D3D12_RESOURCE_STATE_GENERIC_READ;
		else if (ad.HeapType == D3D12_HEAP_TYPE_READBACK)
			initState = D3D12_RESOURCE_STATE_COPY_DEST;

		Buffer_Storage storage{};
		storage.desc = desc;

		hr = m_dma->CreateResource(&ad, &rd, initState, nullptr, storage.alloc.GetAddressOf(), IID_PPV_ARGS(storage.resource.GetAddressOf()));
		HR_VFY(hr);

		auto handle = m_rhp.Allocate<Buffer>();
		HandleAllocator::TryInsertMove(m_buffers, std::move(storage), HandleAllocator::GetSlot(handle.handle));
		return handle;
	}

	Texture RenderDevice_DX12::CreateTexture(const TextureDesc& desc, MemoryPool pool)
	{
		HRESULT hr{ S_OK };

		D3D12MA::ALLOCATION_DESC ad{};
		ad.HeapType = to_internal(desc.memType);
		//ad.Flags = D3D12MA::ALLOCATION_FLAG_STRATEGY_BEST_FIT | D3D12MA::ALLOCATION_FLAG_WITHIN_BUDGET;
		//ad.CustomPool = m_pool.Get();
		if (pool.handle != 0)
		{
			auto poolStorage = HandleAllocator::TryGet(m_memoryPools, HandleAllocator::GetSlot(pool.handle));
			ad.CustomPool = poolStorage.pool.Get();
			ad.Flags = D3D12MA::ALLOCATION_FLAG_STRATEGY_BEST_FIT;
			//ad.Flags |= D3D12MA::ALLOCATION_FLAG_NEVER_ALLOCATE;		// If we want tighter constraints, we can enable this
			assert(ad.HeapType == poolStorage.desc.heapType);
		}

		D3D12_RESOURCE_DESC rd{};
		rd.Dimension = to_internal(desc.type);
		rd.Alignment = desc.alignment;
		rd.Width = desc.width;
		rd.Height = desc.height;
		rd.DepthOrArraySize = (u16)desc.depth;
		rd.MipLevels = (u16)desc.mipLevels;
		rd.Format = desc.format;
		rd.SampleDesc.Count = desc.sampleCount;
		rd.SampleDesc.Quality = desc.sampleQuality;
		rd.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		rd.Flags = desc.flags;

		// Nad bug fix
		std::optional<D3D12_CLEAR_VALUE> clearVal;
		if ((rd.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) == D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)
		{
			clearVal = D3D12_CLEAR_VALUE();
			clearVal->Format = desc.format;
			clearVal->DepthStencil.Depth = desc.depthClear;
			clearVal->DepthStencil.Stencil = desc.stencilClear;
		}
		else if ((rd.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) == D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET)
		{
			clearVal = D3D12_CLEAR_VALUE();
			clearVal->Format = desc.format;
			clearVal->Color[0] = desc.clearColor[0];
			clearVal->Color[1] = desc.clearColor[1];
			clearVal->Color[2] = desc.clearColor[2];
			clearVal->Color[3] = desc.clearColor[3];
		}

		D3D12_RESOURCE_STATES initState{ desc.initState };
		if (ad.HeapType == D3D12_HEAP_TYPE_UPLOAD)
			initState = D3D12_RESOURCE_STATE_GENERIC_READ;
		else if (ad.HeapType == D3D12_HEAP_TYPE_READBACK)
			initState = D3D12_RESOURCE_STATE_COPY_DEST;

		Texture_Storage storage{};
		storage.desc = desc;

		hr = m_dma->CreateResource(&ad, &rd, initState, clearVal ? &clearVal.value() : nullptr, storage.alloc.GetAddressOf(), IID_PPV_ARGS(storage.resource.GetAddressOf()));
		HR_VFY(hr);

#ifdef GPU_VALIDATION_ON
		// Hold the original resource place on a certain heap on a certain offset
		// intentionally leak to avoid crashing CreatePlacedResource
		//auto& heapMap = m_mapping[(u64)storage.alloc->GetHeap()];
		//heapMap[storage.alloc->GetOffset()].push_back(storage.resource);
		//storage.resource->AddRef();	
#endif


		auto handle = m_rhp.Allocate<Texture>();
		HandleAllocator::TryInsertMove(m_textures, std::move(storage), HandleAllocator::GetSlot(handle.handle));

		return handle;
	}

	Pipeline RenderDevice_DX12::CreateGraphicsPipeline(const GraphicsPipelineDesc& desc)
	{
		assert(!desc.vs->blob.empty() && !desc.ps->blob.empty());
		HRESULT hr{ S_OK };

		D3D12_GRAPHICS_PIPELINE_STATE_DESC api_desc = to_internal(desc, m_gfxRsig.Get());

		ComPtr<ID3D12PipelineState> pso;
		hr = m_device->CreateGraphicsPipelineState(&api_desc, IID_PPV_ARGS(pso.GetAddressOf()));
		HR_VFY(hr);

		Pipeline_Storage storage{};
		storage.desc = desc;
		storage.topology = to_internal_topology(desc.topology, (u8)desc.numControlPatches);
		storage.pipeline = pso;

		auto handle = m_rhp.Allocate<Pipeline>();
		HandleAllocator::TryInsertMove(m_pipelines, std::move(storage), HandleAllocator::GetSlot(handle.handle));
		return handle;
	}

	Pipeline RenderDevice_DX12::CreateComputePipeline(const ComputePipelineDesc& desc)
	{
		D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = to_internal_compute(desc, m_gfxRsig.Get());
		HRESULT hr{ S_OK };
		
		ComPtr<ID3D12PipelineState> pso;
		hr = m_device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(pso.GetAddressOf()));
		HR_VFY(hr);
		
		Pipeline_Storage storage{};
		storage.computeDesc = desc;
		storage.pipeline = pso;
		storage.isCompute = true;

		auto handle = m_rhp.Allocate<Pipeline>();
		HandleAllocator::TryInsertMove(m_pipelines, std::move(storage), HandleAllocator::GetSlot(handle.handle));
		return handle;
	}

	RenderPass RenderDevice_DX12::CreateRenderPass(const RenderPassDesc& desc)
	{
		RenderPass_Storage storage{};
		storage.desc = desc;
		storage.flags = to_internal(desc.flags);

		// Translate description to D3D12
		auto& descs = storage.renderTargets;
		for (const auto& rtd : desc.renderTargetDescs)
		{
			auto& res = HandleAllocator::TryGet(m_textureViews, HandleAllocator::GetSlot(rtd.view.handle));		// grab view md

			auto api = to_internal(rtd);
			assert(res.type == ViewType::RenderTarget);
			api.cpuDescriptor = res.view.cpu_handle(0);

			auto& tex = HandleAllocator::TryGet(m_textures, HandleAllocator::GetSlot(res.tex.handle));				// grab underlying texture md
			api.BeginningAccess.Clear.ClearValue.Format = tex.desc.format;
			api.BeginningAccess.Clear.ClearValue.Color[0] = tex.desc.clearColor[0];
			api.BeginningAccess.Clear.ClearValue.Color[1] = tex.desc.clearColor[1];
			api.BeginningAccess.Clear.ClearValue.Color[2] = tex.desc.clearColor[2];
			api.BeginningAccess.Clear.ClearValue.Color[3] = tex.desc.clearColor[3];

			// not supporting resolves for now
			descs.push_back(api);
		} 

		// Translate depth stencil 
		if (desc.depthStencilDesc.has_value())
		{
			auto& res = HandleAllocator::TryGet(m_textureViews, HandleAllocator::GetSlot(desc.depthStencilDesc->view.handle));
			auto depthApi = to_internal(*desc.depthStencilDesc);
			assert(res.type == ViewType::DepthStencil);
			depthApi.cpuDescriptor = res.view.cpu_handle(0);

			auto& tex_res = HandleAllocator::TryGet(m_textures, HandleAllocator::GetSlot(res.tex.handle));				// grab underlying texture md

			depthApi.DepthBeginningAccess.Clear.ClearValue.DepthStencil.Depth = tex_res.desc.depthClear;
			depthApi.DepthBeginningAccess.Clear.ClearValue.DepthStencil.Stencil = tex_res.desc.stencilClear;
			// resolve not supported for now

			storage.depthStencil = depthApi;
		}

		auto handle = m_rhp.Allocate<RenderPass>();
		HandleAllocator::TryInsertMove(m_renderPasses, std::move(storage), HandleAllocator::GetSlot(handle.handle));
		return handle;


	}

	BufferView RenderDevice_DX12::CreateView(Buffer buffer, const BufferViewDesc& desc)
	{
		assert(desc.viewType != ViewType::None);
		assert(desc.viewType != ViewType::DepthStencil);
		assert(desc.viewType != ViewType::RenderTarget);

		auto& buffer_storage = HandleAllocator::TryGet(m_buffers, HandleAllocator::GetSlot(buffer.handle));
		auto view_desc = m_descriptorMgr->allocate(1, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		assert((desc.offset + desc.stride * desc.count) <= buffer_storage.desc.size);

		std::optional<DX12DescriptorChunk> uavClear;
		if (desc.viewType == ViewType::Constant)
		{
			assert(desc.stride % 256 == 0);

			D3D12_CONSTANT_BUFFER_VIEW_DESC cbvd{};
			cbvd.BufferLocation = buffer_storage.resource.Get()->GetGPUVirtualAddress() + desc.offset;
			cbvd.SizeInBytes = desc.stride * desc.count;
			m_device->CreateConstantBufferView(&cbvd, view_desc.cpu_handle(0));
		}
		else if (desc.viewType == ViewType::ShaderResource)
		{
			assert(desc.offset % desc.stride == 0);

			D3D12_SHADER_RESOURCE_VIEW_DESC srvd{};
			srvd.Format = DXGI_FORMAT_UNKNOWN;
			srvd.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
			srvd.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvd.Buffer.FirstElement = desc.offset / desc.stride;
			srvd.Buffer.NumElements = desc.count;
			srvd.Buffer.StructureByteStride = desc.stride;
			srvd.Buffer.Flags = desc.raw ? D3D12_BUFFER_SRV_FLAG_RAW : D3D12_BUFFER_SRV_FLAG_NONE;

			m_device->CreateShaderResourceView(buffer_storage.resource.Get(), &srvd, view_desc.cpu_handle(0));

		}
		else if (desc.viewType == ViewType::UnorderedAccess)
		{
			assert(desc.offset % desc.stride == 0);

			D3D12_UNORDERED_ACCESS_VIEW_DESC uavd{};
			uavd.Format = DXGI_FORMAT_UNKNOWN;
			uavd.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
			uavd.Buffer.FirstElement = desc.offset / desc.stride;
			uavd.Buffer.NumElements = desc.count;
			uavd.Buffer.StructureByteStride = desc.stride;
			uavd.Buffer.CounterOffsetInBytes = 0;
			uavd.Buffer.Flags = desc.raw ? D3D12_BUFFER_UAV_FLAG_RAW : D3D12_BUFFER_UAV_FLAG_NONE;

			// User can choose to either manually use InterlockedAdd with separate buffer or actually pass in a Counter Buffer
			uavClear = m_descriptorMgr->allocate_cbv_srv_uav_cpu(1);
			if (desc.uavCounterResource.handle != 0)
			{
				const auto& counterStorage = HandleAllocator::TryGet(m_buffers, HandleAllocator::GetSlot(desc.uavCounterResource.handle));
				const auto& counterResource = counterStorage.resource;

				uavd.Buffer.CounterOffsetInBytes = desc.uavCounterOffset;
				assert(uavd.Buffer.CounterOffsetInBytes % 4 == 0);
				assert(uavd.Buffer.CounterOffsetInBytes < counterStorage.desc.size);
				assert(uavd.Buffer.Flags != D3D12_BUFFER_UAV_FLAG_RAW);

				m_device->CreateUnorderedAccessView(buffer_storage.resource.Get(), counterResource.Get(), &uavd, view_desc.cpu_handle(0));
				m_device->CreateUnorderedAccessView(buffer_storage.resource.Get(), counterResource.Get(), &uavd, uavClear->cpu_handle(0));

			}
			// PREFERRABLY: User should manually use InterlockedAdd --> Most flexibility
			else
			{
				m_device->CreateUnorderedAccessView(buffer_storage.resource.Get(), nullptr, &uavd, view_desc.cpu_handle(0));
				m_device->CreateUnorderedAccessView(buffer_storage.resource.Get(), nullptr, &uavd, uavClear->cpu_handle(0));
			}

		}
		else if (desc.viewType == ViewType::RaytracingAS)
		{
			D3D12_SHADER_RESOURCE_VIEW_DESC srvd{};
			srvd.Format = DXGI_FORMAT_UNKNOWN;
			srvd.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
			srvd.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvd.RaytracingAccelerationStructure.Location = desc.offset;

			m_device->CreateShaderResourceView(buffer_storage.resource.Get(), &srvd, view_desc.cpu_handle(0));
		}
		else
		{
			assert(false);
		}

		auto handle = m_rhp.Allocate<BufferView>();
		auto viewStorage = BufferView_Storage(buffer, desc.viewType, view_desc);
		viewStorage.uavClear = uavClear;
		HandleAllocator::TryInsertMove(m_bufferViews, std::move(viewStorage), HandleAllocator::GetSlot(handle.handle));
		return handle;

	}

	TextureView RenderDevice_DX12::CreateView(Texture texture, const TextureViewDesc& desc2)
	{
		auto desc = desc2;

		assert(desc.viewType != ViewType::None);
		assert(desc.viewType != ViewType::Constant);
		assert(desc.viewType != ViewType::RaytracingAS);

		auto& tex_storage = HandleAllocator::TryGet(m_textures, HandleAllocator::GetSlot(texture.handle));


		DX12DescriptorChunk view_desc;
		if (desc.viewType == ViewType::RenderTarget)
			view_desc = m_descriptorMgr->allocate(1, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		else if (desc.viewType == ViewType::DepthStencil)
			view_desc = m_descriptorMgr->allocate(1, D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
		else
			view_desc = m_descriptorMgr->allocate(1, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		if (tex_storage.desc.format == DXGI_FORMAT_BC7_UNORM_SRGB || tex_storage.desc.format == DXGI_FORMAT_BC7_UNORM)
			desc.format = tex_storage.desc.format;

		std::optional<DX12DescriptorChunk> uavClear;
		std::set<u32> subresources;
		if (desc.viewType == ViewType::DepthStencil)
		{
			auto dsv = to_dsv(desc, tex_storage.desc.mipLevels, tex_storage.desc.depth, &subresources);
			m_device->CreateDepthStencilView(tex_storage.resource.Get(), &dsv, view_desc.cpu_handle(0));
		}
		else if (desc.viewType == ViewType::RenderTarget)
		{
			auto rtv = to_rtv(desc, tex_storage.desc.mipLevels, tex_storage.desc.depth, &subresources);
			m_device->CreateRenderTargetView(tex_storage.resource.Get(), &rtv, view_desc.cpu_handle(0));
		}
		else if (desc.viewType == ViewType::ShaderResource)
		{
			auto srv = to_srv(desc, tex_storage.desc.mipLevels, tex_storage.desc.depth, &subresources);
			m_device->CreateShaderResourceView(tex_storage.resource.Get(), &srv, view_desc.cpu_handle(0));
		}
		else if (desc.viewType == ViewType::UnorderedAccess)
		{
			auto uav = to_uav(desc, tex_storage.desc.mipLevels, tex_storage.desc.depth, &subresources);

			uavClear = m_descriptorMgr->allocate_cbv_srv_uav_cpu(1);
			m_device->CreateUnorderedAccessView(tex_storage.resource.Get(), nullptr, &uav, view_desc.cpu_handle(0));
			m_device->CreateUnorderedAccessView(tex_storage.resource.Get(), nullptr, &uav, uavClear->cpu_handle(0));
		}
		else
		{
			assert(false);
		}

		auto handle = m_rhp.Allocate<TextureView>();
		auto viewStorage = TextureView_Storage(texture, desc.viewType, view_desc);
		viewStorage.uavClear = uavClear;
		HandleAllocator::TryInsertMove(m_textureViews, std::move(viewStorage), HandleAllocator::GetSlot(handle.handle));
		return handle;
	}

	MemoryPool RenderDevice_DX12::CreateMemoryPool(const MemoryPoolDesc& desc)
	{
		ComPtr<D3D12MA::Pool> pool;
		D3D12MA::POOL_DESC pd{};
		pd.HeapFlags = desc.heapFlags;
		pd.HeapProperties.Type = desc.heapType;
		pd.BlockSize = desc.size;
		HRESULT hr{ S_OK };
		hr = m_dma->CreatePool(&pd, pool.GetAddressOf());
		HR_VFY(hr);

		MemoryPool_Storage storage{};
		storage.desc = desc;
		storage.pool = pool;

		D3D12MA::DetailedStatistics stats{};
		pool->CalculateStatistics(&stats);
		storage.info.blocksAllocated = stats.Stats.BlockCount;
		storage.info.numAllocations = stats.Stats.AllocationCount;
		storage.info.blockBytes = (u32)stats.Stats.BlockBytes;
		storage.info.allocationBytes = (u32)stats.Stats.AllocationBytes;

		storage.info.numUnusedRange = stats.UnusedRangeCount;
		storage.info.smallestAllocation = stats.AllocationSizeMin;
		storage.info.largestAllocation = stats.AllocationSizeMax;
		storage.info.smallestUnusedRange = stats.UnusedRangeSizeMin;
		storage.info.largestUnusedRange = stats.UnusedRangeSizeMax;

		auto handle = m_rhp.Allocate<MemoryPool>();
		HandleAllocator::TryInsertMove(m_memoryPools, std::move(storage), HandleAllocator::GetSlot(handle.handle));

		return handle;
	}

	std::vector<Texture> RenderDevice_DX12::CreateAliasedTextures(const std::vector<TextureDesc>& descs, MemoryPool pool)
	{
		if (descs.size() == 1)
			return { CreateTexture(descs[0], pool) };

		assert(descs.size() > 0);

		std::vector<D3D12_RESOURCE_DESC> internalDescs;
		internalDescs.reserve(descs.size());

		D3D12_RESOURCE_ALLOCATION_INFO finalAllocInfo{};
		for (const auto& desc : descs)
		{
			assert(descs[0].memType == desc.memType);	// All resources passed must have the same memory type

			D3D12_RESOURCE_DESC rd{};
			rd.Dimension = to_internal(desc.type);
			rd.Alignment = desc.alignment;
			rd.Width = desc.width;
			rd.Height = desc.height;
			rd.DepthOrArraySize = (u16)desc.depth;
			rd.MipLevels = (u16)desc.mipLevels;
			rd.Format = desc.format;
			rd.SampleDesc.Count = desc.sampleCount;
			rd.SampleDesc.Quality = desc.sampleQuality;
			rd.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
			rd.Flags = desc.flags;
			internalDescs.push_back(rd);

			auto allocInfo = m_device->GetResourceAllocationInfo(0, 1, &rd);

			finalAllocInfo.Alignment = std::max(finalAllocInfo.Alignment, allocInfo.Alignment);
			finalAllocInfo.SizeInBytes = std::max(finalAllocInfo.SizeInBytes, allocInfo.SizeInBytes);
		}

		D3D12MA::ALLOCATION_DESC allocDesc{};
		allocDesc.HeapType = to_internal(descs[0].memType);
		if (pool.handle != 0)
		{
			auto poolStorage = HandleAllocator::TryGet(m_memoryPools, HandleAllocator::GetSlot(pool.handle));
			allocDesc.CustomPool = poolStorage.pool.Get();
			allocDesc.Flags = D3D12MA::ALLOCATION_FLAG_STRATEGY_BEST_FIT;
			//ad.Flags |= D3D12MA::ALLOCATION_FLAG_NEVER_ALLOCATE;		// If we want tighter constraints, we can enable this
			assert(allocDesc.HeapType == poolStorage.desc.heapType);
		}

		ComPtr<D3D12MA::Allocation> alloc;
		HRESULT hr{ S_OK };
		hr = m_dma->AllocateMemory(&allocDesc, &finalAllocInfo, &alloc);
		assert(SUCCEEDED(hr));
		assert(alloc != NULL && alloc->GetHeap() != NULL);

		/*
			Create N resources using this alloc
			Store the same alloc to all Texture_Storage
		*/
		std::vector<Texture> texturesToRet;
		texturesToRet.reserve(internalDescs.size());
		u32 i = 0;
		for (const auto& desc : internalDescs)
		{
			Texture_Storage storage{};
			storage.desc = descs[i];

			std::optional<D3D12_CLEAR_VALUE> clearVal;
			if ((desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) == D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)
			{
				clearVal = D3D12_CLEAR_VALUE();
				clearVal->Format = descs[i].format;
				clearVal->DepthStencil.Depth = descs[i].depthClear;
				clearVal->DepthStencil.Stencil = descs[i].stencilClear;
			}
			else if ((desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) == D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET)
			{
				clearVal = D3D12_CLEAR_VALUE();
				clearVal->Format = descs[i].format;
				clearVal->Color[0] = descs[i].clearColor[0];
				clearVal->Color[1] = descs[i].clearColor[1];
				clearVal->Color[2] = descs[i].clearColor[2];
				clearVal->Color[3] = descs[i].clearColor[3];
			}

			storage.alloc = alloc;
			hr = m_dma->CreateAliasingResource(alloc.Get(), 0, &desc, descs[i].initState, clearVal ? &clearVal.value() : nullptr, IID_PPV_ARGS(storage.resource.GetAddressOf()));
			HR_VFY(hr);

			auto handle = m_rhp.Allocate<Texture>();
			HandleAllocator::TryInsertMove(m_textures, std::move(storage), HandleAllocator::GetSlot(handle.handle));
			texturesToRet.push_back(handle);

			++i;
		}

		return texturesToRet;
	}

	void RenderDevice_DX12::FreeBuffer(Buffer handle)
	{
		HandleAllocator::FreeStorage(m_rhp, m_buffers, handle);
	}

	void RenderDevice_DX12::FreeTexture(Texture handle)
	{
		ZoneScopedN("RD Texture Free");
		HandleAllocator::FreeStorage(m_rhp, m_textures, handle);
	}

	void RenderDevice_DX12::FreePipeline(Pipeline handle)
	{
		HandleAllocator::FreeStorage(m_rhp, m_pipelines, handle);
	}

	void RenderDevice_DX12::FreeRenderPass(RenderPass handle)
	{
		HandleAllocator::FreeStorage(m_rhp, m_renderPasses, handle);
	}

	void RenderDevice_DX12::FreeView(BufferView handle)
	{
		auto& res = HandleAllocator::TryGet(m_bufferViews, HandleAllocator::GetSlot(handle.handle));
		m_descriptorMgr->free(&res.view);
		if (res.uavClear)
			m_descriptorMgr->free_cbv_srv_uav_cpu(&*res.uavClear);
		HandleAllocator::FreeStorage(m_rhp, m_bufferViews, handle);
	}

	void RenderDevice_DX12::FreeView(TextureView handle)
	{
		auto& res = HandleAllocator::TryGet(m_textureViews, HandleAllocator::GetSlot(handle.handle));
		m_descriptorMgr->free(&res.view);
		if (res.uavClear)
			m_descriptorMgr->free_cbv_srv_uav_cpu(&*res.uavClear);
		HandleAllocator::FreeStorage(m_rhp, m_textureViews, handle);
	}

	void RenderDevice_DX12::FreeMemoryPool(MemoryPool handle)
	{
		HandleAllocator::FreeStorage(m_rhp, m_memoryPools, handle);
		assert(false);		// cleanup to D3D12MA?
	}

	void RenderDevice_DX12::RecycleSync(SyncReceipt receipt)
	{
		auto sync = std::move(HandleAllocator::TryGet(m_syncs, HandleAllocator::GetSlot(receipt.handle)));
		m_recycledSyncs.push(sync);

		HandleAllocator::FreeStorage(m_rhp, m_syncs, receipt);
	}

	void RenderDevice_DX12::RecycleCommandList(CommandList handle)
	{
		auto& res = HandleAllocator::TryGet(m_cmdls, HandleAllocator::GetSlot(handle.handle));
		if (!res.closed)
			res.pair.list->Close();

		CommandAtorAndList storage{ res.pair };
		m_recycledAtorAndLists[res.pair.queueType].push(storage);

		HandleAllocator::FreeStorage(m_rhp, m_cmdls, handle);
	}

	CommandList RenderDevice_DX12::AllocateCommandList(QueueType queue)
	{
		CommandList_Storage storage{};

		auto resetState = [&, rootsig = m_gfxRsig.Get(), dheap = m_descriptorMgr->get_gpu_dh_resource()](ID3D12GraphicsCommandList4* list, QueueType queueType) mutable
		{
			if (queueType == QueueType::Graphics || queueType == QueueType::Compute)
			{
				ID3D12DescriptorHeap* dheaps[] = { dheap };
				list->SetDescriptorHeaps(_countof(dheaps), dheaps);

				// Set both..
				list->SetGraphicsRootSignature(rootsig);
				list->SetComputeRootSignature(rootsig);
			}
		};

		// Re-use if any
		/*
			Recycling is arbitrary, no size tracked
		*/
		auto& recycledPool = m_recycledAtorAndLists[queue];
		if (!recycledPool.empty())
		{
			auto ator_list = recycledPool.front();
			ator_list.Reset();		// Reset ator and list for re-use
			recycledPool.pop();

			resetState(ator_list.list.Get(), queue);

			storage.pair = ator_list;
		}
		else
		{
			// Allocate new ator + list
			HRESULT hr{ S_OK };
			ComPtr<ID3D12CommandAllocator> ator;
			ComPtr<ID3D12GraphicsCommandList4> cmdl;
			hr = m_device->CreateCommandAllocator(GetListType(queue), IID_PPV_ARGS(ator.GetAddressOf()));
			HR_VFY(hr);
			hr = m_device->CreateCommandList1(0, GetListType(queue), D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(cmdl.GetAddressOf()));
			HR_VFY(hr);

			// Reset 
			ator->Reset();
			cmdl->Reset(ator.Get(), nullptr);

			resetState(cmdl.Get(), queue);

			storage.pair.ator = ator;
			storage.pair.list = cmdl;
		}

		storage.pair.queueType = queue;

		auto handle = m_rhp.Allocate<CommandList>();
		HandleAllocator::TryInsertMove(m_cmdls, std::move(storage), HandleAllocator::GetSlot(handle.handle));
		return handle;
	}


	std::optional<SyncReceipt> RenderDevice_DX12::SubmitCommandLists(std::span<CommandList> lists, QueueType queue, std::optional<SyncReceipt> incoming_sync, bool generate_sync)
	{
		ID3D12CommandList* cmdls[16];
		for (u32 i = 0; i < lists.size(); ++i)
		{
			auto& storage = HandleAllocator::TryGet(m_cmdls, HandleAllocator::GetSlot(lists[i].handle));
			storage.closed = true;

			auto cmdl = storage.pair.list;
			cmdl->Close();
			cmdls[i] = cmdl.Get();

			assert(queue == storage.pair.queueType);
		}


		DX12Queue* curr_queue = GetQueue(queue);

		// Wait for incoming sync
		if (incoming_sync.has_value())
		{
			// Lookup sync, and perform GPU wait
			const auto& sync = HandleAllocator::TryGet(m_syncs, HandleAllocator::GetSlot(incoming_sync->handle));
			sync.fence.gpu_wait(*curr_queue);
		}

		curr_queue->execute_command_lists((u32)lists.size(), cmdls);

		// Generate outgoing sync
		std::optional<SyncReceipt> syncReceipt{ std::nullopt };
		if (generate_sync)
		{
			SyncPrimitive sync{};

			// Use recycled syncs
			if (!m_recycledSyncs.empty())
			{
				sync = std::move(m_recycledSyncs.front());
				m_recycledSyncs.pop();
			}
			// Create new sync
			else
			{
				sync.fence = DX12Fence(m_device.Get(), 0);
			}

			curr_queue->insert_signal(sync.fence);

			syncReceipt = m_rhp.Allocate<SyncReceipt>();
			HandleAllocator::TryInsertMove(m_syncs, std::move(sync), HandleAllocator::GetSlot(syncReceipt->handle));
		}
		return syncReceipt;

	}

	std::optional<SyncReceipt> RenderDevice_DX12::SubmitCommandList(CommandList list, QueueType queue, std::optional<SyncReceipt> incoming_sync, bool generate_sync)
	{
		// verify that the submitted lists are compiled
		ID3D12CommandList* cmdls[1];

		auto& storage = HandleAllocator::TryGet(m_cmdls, HandleAllocator::GetSlot(list.handle));
		storage.closed = true;

		auto cmdl = storage.pair.list;
		cmdl->Close();
		cmdls[0] = cmdl.Get();

		assert(queue == storage.pair.queueType);

		DX12Queue* curr_queue = GetQueue(queue);

		// Wait for incoming sync
		if (incoming_sync.has_value())
		{
			// Lookup sync, and perform GPU wait
			const auto& sync = HandleAllocator::TryGet(m_syncs, HandleAllocator::GetSlot(incoming_sync->handle));
			sync.fence.gpu_wait(*curr_queue);
		}

		curr_queue->execute_command_lists(_countof(cmdls), cmdls);

		// Generate outgoing sync
		std::optional<SyncReceipt> syncReceipt{ std::nullopt };
		if (generate_sync)
		{
			SyncPrimitive sync{};

			// Use recycled syncs
			if (!m_recycledSyncs.empty())
			{
				sync = std::move(m_recycledSyncs.front());
				m_recycledSyncs.pop();
			}
			// Create new sync
			else
			{
				sync.fence = DX12Fence(m_device.Get(), 0);
			}

			curr_queue->insert_signal(sync.fence);

			syncReceipt = m_rhp.Allocate<SyncReceipt>();
			HandleAllocator::TryInsertMove(m_syncs, std::move(sync), HandleAllocator::GetSlot(syncReceipt->handle));
		}
		return syncReceipt;
	}

	u32 RenderDevice_DX12::GetGlobalDescriptor(BufferView view) const
	{
		const auto& res = HandleAllocator::TryGet(m_bufferViews, HandleAllocator::GetSlot(view.handle));
		return (u32)res.view.index_offset_from_base();
	}

	u32 RenderDevice_DX12::GetGlobalDescriptor(TextureView view) const
	{
		const auto& res = HandleAllocator::TryGet(m_textureViews, HandleAllocator::GetSlot(view.handle));
		return (u32)res.view.index_offset_from_base();
	}

	void RenderDevice_DX12::Flush()
	{
		m_directQueue->flush();
		m_copyQueue->flush();
	}

	void RenderDevice_DX12::WaitForGPU(SyncReceipt receipt)
	{
		const auto& sync = HandleAllocator::TryGet(m_syncs, HandleAllocator::GetSlot(receipt.handle));
		sync.fence.cpu_wait();

		RecycleSync(receipt);
	}

	u8* RenderDevice_DX12::Map(Buffer handle, u32 subresource, std::pair<u32, u32> read_range)
	{
		auto& res = HandleAllocator::TryGet(m_buffers, HandleAllocator::GetSlot(handle.handle));

		u8* mapped{ nullptr };

		D3D12_RANGE range{};
		range.Begin = read_range.first;
		range.End = read_range.second;
		HR hr = res.resource->Map(subresource, &range, (void**)&mapped);
		hr.try_fail("Failed to map subresource");

		return mapped;
	}

	void RenderDevice_DX12::Unmap(Buffer handle, u32 subresource, std::pair<u32, u32> written_range)
	{
		auto& res = HandleAllocator::TryGet(m_buffers, HandleAllocator::GetSlot(handle.handle));

		D3D12_RANGE range{};
		range.Begin = written_range.first;
		range.End = written_range.second;
		res.resource->Unmap(subresource, &range);
	}






	void RenderDevice_DX12::Cmd_SetIndexBuffer(CommandList list, Buffer ib)
	{
		auto& cmdlRes = HandleAllocator::TryGet(m_cmdls, HandleAllocator::GetSlot(list.handle));
		auto& bufRes = HandleAllocator::TryGet(m_buffers, HandleAllocator::GetSlot(ib.handle));

		D3D12_INDEX_BUFFER_VIEW ibv{};
		ibv.BufferLocation = bufRes.resource->GetGPUVirtualAddress();
		ibv.Format = DXGI_FORMAT_R32_UINT;
		ibv.SizeInBytes = bufRes.desc.size;

		cmdlRes.pair.list->IASetIndexBuffer(&ibv);
	}

	void RenderDevice_DX12::Cmd_Draw(CommandList list, u32 vertsPerInstance, u32 instanceCount, u32 vertStart, u32 instanceStart)
	{
		auto& res = HandleAllocator::TryGet(m_cmdls, HandleAllocator::GetSlot(list.handle));

		res.pair.list->DrawInstanced(vertsPerInstance, instanceCount, vertStart, instanceStart);
	}

	void RenderDevice_DX12::Cmd_DrawIndexed(CommandList list, u32 indicesPerInstance, u32 instanceCount, u32 indexStart, u32 vertStart, u32 instanceStart)
	{
		auto& res = HandleAllocator::TryGet(m_cmdls, HandleAllocator::GetSlot(list.handle));

		res.pair.list->DrawIndexedInstanced(indicesPerInstance, instanceCount, indexStart, vertStart, instanceStart);
	}

	void RenderDevice_DX12::Cmd_Dispatch(CommandList list,
		u32 threadGroupCountX,
		u32 threadGroupCountY,
		u32 threadGroupCountZ)
	{
		auto& res = HandleAllocator::TryGet(m_cmdls, HandleAllocator::GetSlot(list.handle));
		res.pair.list->Dispatch(threadGroupCountX, threadGroupCountY, threadGroupCountZ);
	}

	void RenderDevice_DX12::Cmd_SetPipeline(CommandList list, Pipeline pipeline)
	{
		auto& cmdlRes = HandleAllocator::TryGet(m_cmdls, HandleAllocator::GetSlot(list.handle));
		const auto& pipeRes = HandleAllocator::TryGet(m_pipelines, HandleAllocator::GetSlot(pipeline.handle));

		if (!pipeRes.isCompute)
			cmdlRes.pair.list->IASetPrimitiveTopology(pipeRes.topology);

		cmdlRes.pair.list->SetPipelineState(pipeRes.pipeline.Get());
	}

	void RenderDevice_DX12::Cmd_Barrier(CommandList list, std::span<GPUBarrier> barriers)
	{
		// Convert to primitive (potential bug here??)
		std::vector<D3D12_RESOURCE_BARRIER> barrs{};
		barrs.resize(barriers.size());
		for (u32 i = 0; i < barriers.size(); ++i)
		{
			const auto& barr = barriers[i];

			barrs[i].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			barrs[i].Type = barr.type;
			switch (barr.type)
			{
			case D3D12_RESOURCE_BARRIER_TYPE_TRANSITION:
			{
				if (barr.isBuffer)
				{
					const auto& res = HandleAllocator::TryGet(m_buffers, HandleAllocator::GetSlot(barr.resource));
					barrs[i].Transition.pResource = res.resource.Get();
				}
				else
				{
					const auto& res = HandleAllocator::TryGet(m_textures, HandleAllocator::GetSlot(barr.resource));
					barrs[i].Transition.pResource = res.resource.Get();
				}

				barrs[i].Transition.StateBefore = barr.stateBefore;
				barrs[i].Transition.StateAfter = barr.stateAfter;
				barrs[i].Transition.Subresource = barr.subresource;
				break;
			}
			case D3D12_RESOURCE_BARRIER_TYPE_ALIASING:
			{
				if (barr.isBuffer)
				{
					assert(false);
				}
				else
				{
					const auto& res1 = HandleAllocator::TryGet(m_textures, HandleAllocator::GetSlot(barr.resource));
					const auto& res2 = HandleAllocator::TryGet(m_textures, HandleAllocator::GetSlot(barr.aliasResourceAfter));

					barrs[i].Aliasing.pResourceBefore = res1.resource.Get();
					barrs[i].Aliasing.pResourceAfter = res2.resource.Get();
				}

				break;
			}
			case D3D12_RESOURCE_BARRIER_TYPE_UAV:
			{
				if (barr.isBuffer)
				{
					const auto& res = HandleAllocator::TryGet(m_buffers, HandleAllocator::GetSlot(barr.resource));
					barrs[i].UAV.pResource = res.resource.Get();
				}
				else
				{
					const auto& res = HandleAllocator::TryGet(m_textures, HandleAllocator::GetSlot(barr.resource));
					barrs[i].UAV.pResource = res.resource.Get();
				}
				break;
			}
			default:
				assert(false);
			}
		}

		auto& cmdRes = HandleAllocator::TryGet(m_cmdls, HandleAllocator::GetSlot(list.handle));
		cmdRes.pair.list->ResourceBarrier((u32)barrs.size(), barrs.data());
	}

	void RenderDevice_DX12::Cmd_BeginRenderPass(CommandList list, RenderPass rp)
	{
		auto& cmdlRes = HandleAllocator::TryGet(m_cmdls, HandleAllocator::GetSlot(list.handle));
		auto& rpRes = HandleAllocator::TryGet(m_renderPasses, HandleAllocator::GetSlot(rp.handle));

		cmdlRes.pair.list->BeginRenderPass((u32)rpRes.renderTargets.size(), rpRes.renderTargets.data(),
			rpRes.depthStencil.has_value() ? &(*rpRes.depthStencil) : nullptr, rpRes.flags);
	}

	void RenderDevice_DX12::Cmd_EndRenderPass(CommandList list)
	{
		auto& cmdl = HandleAllocator::TryGet(m_cmdls, HandleAllocator::GetSlot(list.handle));

		cmdl.pair.list->EndRenderPass();
	}

	void RenderDevice_DX12::Cmd_UpdateShaderArgs(CommandList list, QueueType targetQueue, const ShaderArgs& args)
	{
		auto& cmdlRes = HandleAllocator::TryGet(m_cmdls, HandleAllocator::GetSlot(list.handle));

		if (targetQueue == QueueType::Graphics)
		{
			cmdlRes.pair.list->SetGraphicsRoot32BitConstants(0, args.numConstants, args.constants.data(), 0);
			if (args.mainCBV.handle != 0)
			{
				auto& storage = HandleAllocator::TryGet(m_buffers, HandleAllocator::GetSlot(args.mainCBV.handle));
				cmdlRes.pair.list->SetGraphicsRootConstantBufferView(1, storage.resource->GetGPUVirtualAddress() + args.mainCBVOffset);
			}
			if (args.secondaryCBV.handle != 0)
			{
				auto& storage = HandleAllocator::TryGet(m_buffers, HandleAllocator::GetSlot(args.secondaryCBV.handle));
				cmdlRes.pair.list->SetGraphicsRootConstantBufferView(2, storage.resource->GetGPUVirtualAddress() + args.secondaryCBVOffset);
			}
		}
		else if (targetQueue == QueueType::Compute)
		{
			cmdlRes.pair.list->SetComputeRoot32BitConstants(0, args.numConstants, args.constants.data(), 0);
			if (args.mainCBV.handle != 0)
			{
				auto& storage = HandleAllocator::TryGet(m_buffers, HandleAllocator::GetSlot(args.mainCBV.handle));
				cmdlRes.pair.list->SetComputeRootConstantBufferView(1, storage.resource->GetGPUVirtualAddress() + args.mainCBVOffset);
			}
			if (args.secondaryCBV.handle != 0)
			{
				auto& storage = HandleAllocator::TryGet(m_buffers, HandleAllocator::GetSlot(args.secondaryCBV.handle));
				cmdlRes.pair.list->SetComputeRootConstantBufferView(2, storage.resource->GetGPUVirtualAddress() + args.secondaryCBVOffset);
			}
		}

	}

	void RenderDevice_DX12::Cmd_CopyBuffer(CommandList list, Buffer dst, u32 dstOffset, Buffer src, u32 srcOffset, u32 size)
	{
		auto& cmdlRes = HandleAllocator::TryGet(m_cmdls, HandleAllocator::GetSlot(list.handle));
		const auto& srcRes = HandleAllocator::TryGet(m_buffers, HandleAllocator::GetSlot(src.handle));
		const auto& dstRes = HandleAllocator::TryGet(m_buffers, HandleAllocator::GetSlot(dst.handle));

		cmdlRes.pair.list->CopyBufferRegion(dstRes.resource.Get(), dstOffset, srcRes.resource.Get(), srcOffset, size);
	}

	void RenderDevice_DX12::Cmd_CopyBufferToImage(CommandList list,
		Texture dst, u32 dstSubresource, std::tuple<u32, u32, u32> dstTopLeft,
		Buffer src, u32 srcOffset, DXGI_FORMAT srcFormat, u32 srcWidth, u32 srcHeight, u32 srcDepth, u32 srcRowPitch)
	{
		auto& cmdlRes = HandleAllocator::TryGet(m_cmdls, HandleAllocator::GetSlot(list.handle));
		const auto& srcRes = HandleAllocator::TryGet(m_buffers, HandleAllocator::GetSlot(src.handle));
		const auto& dstRes = HandleAllocator::TryGet(m_textures, HandleAllocator::GetSlot(dst.handle));

		D3D12_TEXTURE_COPY_LOCATION dst_loc{}, src_loc{};

		dst_loc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		dst_loc.pResource = dstRes.resource.Get();
		dst_loc.SubresourceIndex = dstSubresource;

		src_loc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		src_loc.pResource = srcRes.resource.Get();
		src_loc.PlacedFootprint.Offset = srcOffset;				// ======= @todo: is this correct?

		// Assert that the user has placed the data correctly according to alignment rules
		assert(srcOffset % D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT == 0);

		src_loc.PlacedFootprint.Footprint.RowPitch = srcRowPitch;
		src_loc.PlacedFootprint.Footprint.Depth = srcDepth;
		// reinterpret if Block Compressed :)
		src_loc.PlacedFootprint.Footprint.Width = srcFormat == DXGI_FORMAT_BC7_UNORM_SRGB || srcFormat == DXGI_FORMAT_BC7_UNORM ? srcWidth * 4 : srcWidth;			
		src_loc.PlacedFootprint.Footprint.Height = srcFormat == DXGI_FORMAT_BC7_UNORM_SRGB || srcFormat == DXGI_FORMAT_BC7_UNORM ? srcHeight * 4 : srcHeight;
		src_loc.PlacedFootprint.Footprint.Format = srcFormat;

		cmdlRes.pair.list->CopyTextureRegion(&dst_loc, std::get<0>(dstTopLeft), std::get<1>(dstTopLeft), std::get<2>(dstTopLeft), &src_loc, nullptr);
	}

	void RenderDevice_DX12::Cmd_SetViewports(CommandList list, Viewports vps)
	{
		auto& cmdlRes = HandleAllocator::TryGet(m_cmdls, HandleAllocator::GetSlot(list.handle));

		cmdlRes.pair.list->RSSetViewports(vps.numVps, vps.vps.data());
	}

	void RenderDevice_DX12::Cmd_SetScissorRects(CommandList list, ScissorRects rects)
	{
		auto& cmdlRes = HandleAllocator::TryGet(m_cmdls, HandleAllocator::GetSlot(list.handle));

		cmdlRes.pair.list->RSSetScissorRects(rects.numScissors, rects.scissors.data());
	}

	void RenderDevice_DX12::Cmd_ClearUnorderedAccessFLOAT(CommandList list,
		BufferView view, std::array<f32, 4> clear, const ScissorRects& rects)
	{
		auto& cmdlRes = HandleAllocator::TryGet(m_cmdls, HandleAllocator::GetSlot(list.handle));
		auto& viewRes = HandleAllocator::TryGet(m_bufferViews, HandleAllocator::GetSlot(view.handle));
		auto& d12Res = HandleAllocator::TryGet(m_buffers, HandleAllocator::GetSlot(viewRes.buf.handle));
		
		cmdlRes.pair.list->ClearUnorderedAccessViewFloat(viewRes.view.gpu_handle(0), viewRes.uavClear->cpu_handle(0),
			d12Res.resource.Get(), clear.data(), rects.numScissors, rects.scissors.data());
	}

	void RenderDevice_DX12::Cmd_ClearUnorderedAccessFLOAT(CommandList list,
		TextureView view, std::array<f32, 4> clear, const ScissorRects& rects)
	{
		auto& cmdlRes = HandleAllocator::TryGet(m_cmdls, HandleAllocator::GetSlot(list.handle));
		auto& viewRes = HandleAllocator::TryGet(m_textureViews, HandleAllocator::GetSlot(view.handle));
		auto& d12Res = HandleAllocator::TryGet(m_textures, HandleAllocator::GetSlot(viewRes.tex.handle));

		cmdlRes.pair.list->ClearUnorderedAccessViewFloat(viewRes.view.gpu_handle(0), viewRes.uavClear->cpu_handle(0),
			d12Res.resource.Get(), clear.data(), rects.numScissors, rects.scissors.data());
	}

	void RenderDevice_DX12::Cmd_ClearUnorderedAccessUINT(CommandList list, 
		BufferView view, std::array<u32, 4> clear, const ScissorRects& rects)
	{
		auto& cmdlRes = HandleAllocator::TryGet(m_cmdls, HandleAllocator::GetSlot(list.handle));
		auto& viewRes = HandleAllocator::TryGet(m_bufferViews, HandleAllocator::GetSlot(view.handle));
		auto& d12Res = HandleAllocator::TryGet(m_buffers, HandleAllocator::GetSlot(viewRes.buf.handle));

		cmdlRes.pair.list->ClearUnorderedAccessViewUint(viewRes.view.gpu_handle(0), viewRes.view.cpu_handle(0),
			d12Res.resource.Get(), clear.data(), rects.numScissors, rects.scissors.data());
	}

	void RenderDevice_DX12::Cmd_ClearUnorderedAccessUINT(CommandList list, 
		TextureView view, std::array<u32, 4> clear, const ScissorRects& rects)
	{
		auto& cmdlRes = HandleAllocator::TryGet(m_cmdls, HandleAllocator::GetSlot(list.handle));
		auto& viewRes = HandleAllocator::TryGet(m_textureViews, HandleAllocator::GetSlot(view.handle));
		auto& d12Res = HandleAllocator::TryGet(m_textures, HandleAllocator::GetSlot(viewRes.tex.handle));

		cmdlRes.pair.list->ClearUnorderedAccessViewUint(viewRes.view.gpu_handle(0), viewRes.view.cpu_handle(0),
			d12Res.resource.Get(), clear.data(), rects.numScissors, rects.scissors.data());
	}



	ID3D12CommandQueue* RenderDevice_DX12::GetQueue()
	{
		return *m_directQueue;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE RenderDevice_DX12::GetReservedRTV(u8 offset)
	{
		return m_d2dReservedDescriptor.value().cpu_handle(offset);
	}



	// Impl. interface
	ID3D12CommandQueue* RenderDevice_DX12::GetQueue(D3D12_COMMAND_LIST_TYPE type)
	{
		switch (type)
		{
		case D3D12_COMMAND_LIST_TYPE_DIRECT:
			return *m_directQueue;
		case D3D12_COMMAND_LIST_TYPE_COPY:
			return *m_copyQueue;
		case D3D12_COMMAND_LIST_TYPE_COMPUTE:
			assert(false);
		default:
			assert(false);
		}
		return nullptr;
	}

	Texture RenderDevice_DX12::RegisterSwapchainTexture(ComPtr<ID3D12Resource> texture)
	{
		Texture_Storage storage{};
		storage.resource = texture;
		auto desc = texture->GetDesc();

		auto handle = m_rhp.Allocate<Texture>();
		HandleAllocator::TryInsertMove(m_textures, std::move(storage), HandleAllocator::GetSlot(handle.handle));
		return handle;
	}

	void RenderDevice_DX12::SetClearColor(Texture tex, const std::array<float, 4>& clear_color)
	{
		auto& res = HandleAllocator::TryGet(m_textures, HandleAllocator::GetSlot(tex.handle));
		res.desc.clearColor = clear_color;
	}

	std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> RenderDevice_DX12::GetReservedResourceHandle() const
	{
		return { m_reservedDescriptor->cpu_handle(0), m_reservedDescriptor->gpu_handle(0) };
	}

	ID3D12DescriptorHeap* RenderDevice_DX12::GetMainResourceDH() const
	{
		return m_descriptorMgr->get_gpu_dh_resource();
	}

	ID3D12GraphicsCommandList4* RenderDevice_DX12::GetListForExternal(CommandList cmdl)
	{
		const auto& res = HandleAllocator::TryGet(m_cmdls, HandleAllocator::GetSlot(cmdl.handle));
		return res.pair.list.Get();
	}

	void RenderDevice_DX12::CreateQueues()
	{
		m_directQueue = std::make_unique<DX12Queue>(m_device.Get(), D3D12_COMMAND_LIST_TYPE_DIRECT);
		m_copyQueue = std::make_unique<DX12Queue>(m_device.Get(), D3D12_COMMAND_LIST_TYPE_COPY);
		//m_computeQueue = std::make_unique<DX12Queue>(m_device.Get(), D3D12_COMMAND_LIST_TYPE_COMPUTE);
	}

	void RenderDevice_DX12::InitDMA(IDXGIAdapter* adapter)
	{
		HRESULT hr{ S_OK };

		D3D12MA::ALLOCATOR_DESC desc{};
		desc.pDevice = m_device.Get();
		desc.pAdapter = adapter;
		hr = D3D12MA::CreateAllocator(&desc, m_dma.GetAddressOf());
		HR_VFY(hr);
	}

	void RenderDevice_DX12::InitRootsig()
	{
		HRESULT hr{ S_OK };

		const u8 num_constants = 14;

		std::vector<D3D12_ROOT_PARAMETER> params;
		D3D12_ROOT_PARAMETER param{};
		param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
		param.Constants.RegisterSpace = 0;
		param.Constants.ShaderRegister = 0;
		param.Constants.Num32BitValues = num_constants + 1;		// Indirect Set Constant requires 2 for some reason to start working with Debug Validation Layer is on
		params.push_back(param);

		D3D12_ROOT_PARAMETER cbvParam{};
		cbvParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		cbvParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		cbvParam.Descriptor.RegisterSpace = 0;
		cbvParam.Descriptor.ShaderRegister = 1;
		params.push_back(cbvParam);
		cbvParam.Descriptor.ShaderRegister = 2;
		params.push_back(cbvParam);


		D3D12_ROOT_SIGNATURE_DESC rsd{};
		rsd.NumParameters = (UINT)params.size();
		rsd.pParameters = params.data();
		rsd.Flags = D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED;

		auto static_samplers = GrabStaticSamplers();
		rsd.NumStaticSamplers = (u32)static_samplers.size();
		rsd.pStaticSamplers = static_samplers.data();

		ComPtr<ID3DBlob> signature;
		ComPtr<ID3DBlob> error;
		hr = D3D12SerializeRootSignature(&rsd, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
		if (FAILED(hr))
		{
			std::string msg = "Compilation failed with errors:\n";
			msg += std::string((const char*)error->GetBufferPointer());
			msg += "\n";

			OutputDebugStringA(msg.c_str());
			assert(false);
		}

		hr = m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(m_gfxRsig.GetAddressOf()));
		HR_VFY(hr);
	}

	std::vector<D3D12_STATIC_SAMPLER_DESC> RenderDevice_DX12::GrabStaticSamplers()
	{
		std::vector<D3D12_STATIC_SAMPLER_DESC> samplers;
		constexpr uint32_t space = 1;
		uint32_t next_register = 0;
		{
			D3D12_STATIC_SAMPLER_DESC sd1{};
			sd1.Filter = D3D12_FILTER_ANISOTROPIC;
			sd1.AddressU = sd1.AddressV = sd1.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			sd1.MipLODBias = 0.f;
			sd1.MaxAnisotropy = 8;
			sd1.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
			sd1.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
			sd1.MinLOD = 0.f;
			sd1.MaxLOD = D3D12_FLOAT32_MAX;
			sd1.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
			sd1.ShaderRegister = next_register++;
			sd1.RegisterSpace = space;
			samplers.push_back(sd1);
		}

		{
			D3D12_STATIC_SAMPLER_DESC sd1{};
			sd1.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
			sd1.AddressU = sd1.AddressV = sd1.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			sd1.MipLODBias = 0.f;
			sd1.MaxAnisotropy = 8;
			sd1.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
			sd1.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
			sd1.MinLOD = 0.f;
			sd1.MaxLOD = D3D12_FLOAT32_MAX;
			sd1.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
			sd1.ShaderRegister = next_register++;
			sd1.RegisterSpace = space;
			samplers.push_back(sd1);
		}

		{
			// g_point_clamp_samp
			D3D12_STATIC_SAMPLER_DESC sd1{};
			sd1.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
			sd1.AddressU = sd1.AddressV = sd1.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
			sd1.MipLODBias = 0.f;
			sd1.MaxAnisotropy = 0;
			sd1.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
			sd1.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
			sd1.MinLOD = 0.f;
			sd1.MaxLOD = D3D12_FLOAT32_MAX;
			sd1.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
			sd1.ShaderRegister = next_register++;
			sd1.RegisterSpace = space;
			samplers.push_back(sd1);
		}

		{
			// g_bilinear_wrap_samp
			D3D12_STATIC_SAMPLER_DESC sd1{};
			sd1.Filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
			sd1.AddressU = sd1.AddressV = sd1.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			sd1.MipLODBias = 0.f;
			sd1.MaxAnisotropy = 0;
			sd1.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
			sd1.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
			sd1.MinLOD = 0.f;
			sd1.MaxLOD = D3D12_FLOAT32_MAX;
			sd1.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
			sd1.ShaderRegister = next_register++;
			sd1.RegisterSpace = space;
			samplers.push_back(sd1);
		}

		{
			// g_bilinear_clamp_samp
			D3D12_STATIC_SAMPLER_DESC sd1{};
			sd1.Filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
			sd1.AddressU = sd1.AddressV = sd1.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
			sd1.MipLODBias = 0.f;
			sd1.MaxAnisotropy = 0;
			sd1.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
			sd1.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
			sd1.MinLOD = 0.f;
			sd1.MaxLOD = D3D12_FLOAT32_MAX;
			sd1.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
			sd1.ShaderRegister = next_register++;
			sd1.RegisterSpace = space;
			samplers.push_back(sd1);
		}

		//Shadows vanilla:
		{
			D3D12_STATIC_SAMPLER_DESC sd1{};
			sd1.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
			sd1.AddressU = sd1.AddressV = sd1.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
			sd1.MipLODBias = 0.f;
			sd1.MaxAnisotropy = 1;
			sd1.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
			sd1.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
			sd1.MinLOD = 0.f;
			sd1.MaxLOD = D3D12_FLOAT32_MAX;
			sd1.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
			sd1.ShaderRegister = next_register++;
			sd1.RegisterSpace = space;
			samplers.push_back(sd1);
		}

		//Hardware PCF Shadows:
		{
			D3D12_STATIC_SAMPLER_DESC sd1{};
			sd1.Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
			sd1.AddressU = sd1.AddressV = sd1.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
			sd1.MipLODBias = 0.f;
			sd1.MaxAnisotropy = 1;
			sd1.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
			sd1.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
			sd1.MinLOD = 0.f;
			sd1.MaxLOD = D3D12_FLOAT32_MAX;
			sd1.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
			sd1.ShaderRegister = next_register++;
			sd1.RegisterSpace = space;
			samplers.push_back(sd1);
		}

		return samplers;
	}

	DX12Queue* RenderDevice_DX12::GetQueue(QueueType type)
	{
		DX12Queue* curr_queue{ nullptr };
		switch (type)
		{
		case QueueType::Graphics:
			curr_queue = m_directQueue.get();
			break;
		case QueueType::Compute:
			curr_queue = m_computeQueue.get();
			break;
		case QueueType::Copy:
			curr_queue = m_copyQueue.get();
			break;
		default:
			curr_queue = m_directQueue.get();
		}
		return curr_queue;
	}

	D3D12_COMMAND_LIST_TYPE RenderDevice_DX12::GetListType(QueueType queue)
	{
		switch (queue)
		{
		case QueueType::Graphics:
			return D3D12_COMMAND_LIST_TYPE_DIRECT;
		case QueueType::Compute:
			return D3D12_COMMAND_LIST_TYPE_COMPUTE;
		case QueueType::Copy:
			return D3D12_COMMAND_LIST_TYPE_COPY;
		default:
			return D3D12_COMMAND_LIST_TYPE_DIRECT;
		}
	}

}


