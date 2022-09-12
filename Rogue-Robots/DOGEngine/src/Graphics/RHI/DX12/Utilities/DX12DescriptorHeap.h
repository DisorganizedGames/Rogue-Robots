#include "DX12DescriptorChunk.h"
#include "../CommonIncludes_DX12.h"

class DX12DescriptorHeap
{
public:
	DX12DescriptorHeap(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE heap_type, uint32_t max_descriptors, bool is_shader_visible);

	// Get a ranged chunk
	DX12DescriptorChunk get_chunk(uint64_t index_start, uint32_t num_descriptors) const;

	// Returns the whole heap as a single chunk
	DX12DescriptorChunk as_chunk() const;

	uint32_t max_descriptors() const { return m_max_descriptors; }
	bool is_shader_visible() const { return m_is_shader_visible; }

	operator ID3D12DescriptorHeap* () const { return m_heap.Get(); }
private:
	ComPtr<ID3D12DescriptorHeap> m_heap;
	D3D12_DESCRIPTOR_HEAP_TYPE m_heap_type;
	uint32_t m_max_descriptors{ 0 };
	uint8_t m_descriptor_size{ 0 };
	bool m_is_shader_visible{ false };
};