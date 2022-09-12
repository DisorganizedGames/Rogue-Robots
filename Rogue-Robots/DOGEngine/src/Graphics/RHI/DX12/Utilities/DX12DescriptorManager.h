#pragma once
#include "../CommonIncludes_DX12.h"
#include "DX12DescriptorChunk.h"

class DX12DescriptorAllocatorDMA;
class DX12DescriptorHeap;

class DX12DescriptorManager
{
public:
	DX12DescriptorManager(ID3D12Device* device);
	~DX12DescriptorManager();

	DX12DescriptorChunk allocate(uint32_t num_descriptors, D3D12_DESCRIPTOR_HEAP_TYPE heap_type);
	void free(DX12DescriptorChunk* chunk);

	// Get underlying descriptor heaps (for binding)
	ID3D12DescriptorHeap* get_gpu_dh_resource() const;
	ID3D12DescriptorHeap* get_gpu_dh_sampler() const;

private:
	void init_heaps(ID3D12Device* device);
	void init_allocators();

private:
	std::unique_ptr<DX12DescriptorHeap> m_gpu_dh_resource;		// CBV_SRV_UAV 
	std::unique_ptr<DX12DescriptorHeap> m_gpu_dh_sampler;		// Samplers
	std::unique_ptr<DX12DescriptorHeap> m_cpu_dh_rtv;			// RTV
	std::unique_ptr<DX12DescriptorHeap> m_cpu_dh_dsv;			// DSV

	std::unique_ptr<DX12DescriptorAllocatorDMA> m_gpu_dh_resource_ator;
	std::unique_ptr<DX12DescriptorAllocatorDMA> m_gpu_dh_sampler_ator;
	std::unique_ptr<DX12DescriptorAllocatorDMA> m_cpu_dh_rtv_ator;
	std::unique_ptr<DX12DescriptorAllocatorDMA> m_cpu_dh_dsv_ator;
};

