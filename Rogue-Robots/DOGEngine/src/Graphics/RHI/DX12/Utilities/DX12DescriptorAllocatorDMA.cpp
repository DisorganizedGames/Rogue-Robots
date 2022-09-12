#include "DX12DescriptorAllocatorDMA.h"

DX12DescriptorAllocatorDMA::DX12DescriptorAllocatorDMA(DX12DescriptorChunk&& chunk, bool uses_linear_algorithm) :
	m_chunk(std::move(chunk))
{
	HRESULT hr = S_OK;

	D3D12MA::VIRTUAL_BLOCK_DESC block_desc = {};
	block_desc.Size = m_chunk.num_descriptors();		// treat each descriptor as a 'byte'
	block_desc.Flags = uses_linear_algorithm ? D3D12MA::VIRTUAL_BLOCK_FLAG_ALGORITHM_LINEAR : D3D12MA::VIRTUAL_BLOCK_FLAG_NONE;

	hr = CreateVirtualBlock(&block_desc, &m_dma_block);
	assert(SUCCEEDED(hr));
}

DX12DescriptorAllocatorDMA::~DX12DescriptorAllocatorDMA()
{
	assert(m_active_allocations.size() == 0);
}

DX12DescriptorChunk DX12DescriptorAllocatorDMA::allocate(uint32_t num_descriptors)
{
	HRESULT hr = S_OK;

	D3D12MA::VIRTUAL_ALLOCATION_DESC alloc_desc{};
	alloc_desc.Size = num_descriptors;

	D3D12MA::VirtualAllocation dma_alloc;
	UINT64 alloc_offset;
	hr = m_dma_block->Allocate(&alloc_desc, &dma_alloc, &alloc_offset);
	if (SUCCEEDED(hr))
	{
		m_active_allocations.insert({ dma_alloc.AllocHandle, dma_alloc });

		auto chunk = DX12DescriptorChunk(m_chunk.get_subchunk(alloc_offset, num_descriptors));
		chunk.set_allocator_key(dma_alloc.AllocHandle);	// Hold on to the alloc handle
		return chunk;
	}
	else
	{
		// Allocation failed - no space for it could be found.
		assert(false);
		return DX12DescriptorChunk();
	}
}

void DX12DescriptorAllocatorDMA::free(DX12DescriptorChunk* chunk)
{
	uint64_t handle = chunk->get_allocator_key();
	auto it = m_active_allocations.find(handle);
	D3D12MA::VirtualAllocation alloc = it->second;
	m_dma_block->FreeAllocation(alloc);
	m_active_allocations.erase(it);
}
