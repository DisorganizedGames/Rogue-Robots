#pragma once
#include "D3D12MemAlloc.h"
#include <unordered_map>
#include "DX12DescriptorChunk.h"

class DX12DescriptorAllocatorDMA
{
public:
	DX12DescriptorAllocatorDMA(DX12DescriptorChunk&& chunk, bool uses_linear_algorithm = false);
	~DX12DescriptorAllocatorDMA();

	DX12DescriptorChunk allocate(uint32_t num_descriptors);
	void free(DX12DescriptorChunk* chunk);

private:
	DX12DescriptorChunk m_chunk;
	D3D12MA::VirtualBlock* m_dma_block{ nullptr };

	std::unordered_map<uint64_t, D3D12MA::VirtualAllocation> m_active_allocations;
};


