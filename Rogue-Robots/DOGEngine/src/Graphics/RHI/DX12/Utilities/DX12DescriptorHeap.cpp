#include "DX12DescriptorHeap.h"

DX12DescriptorHeap::DX12DescriptorHeap(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE heap_type, uint32_t max_descriptors, bool is_shader_visible) :
	m_heap_type(heap_type),
	m_max_descriptors(max_descriptors),
	m_descriptor_size(device->GetDescriptorHandleIncrementSize(heap_type)),
	m_is_shader_visible(is_shader_visible)
{
	HRESULT hr{ S_OK };

	D3D12_DESCRIPTOR_HEAP_DESC dhd{};
	dhd.Type = heap_type;
	dhd.NumDescriptors = max_descriptors;
	dhd.Flags = is_shader_visible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	hr = device->CreateDescriptorHeap(&dhd, IID_PPV_ARGS(m_heap.GetAddressOf()));

	assert(SUCCEEDED(hr));
}

DX12DescriptorChunk DX12DescriptorHeap::get_chunk(uint64_t index_start, uint32_t num_descriptors) const
{
	D3D12_CPU_DESCRIPTOR_HANDLE cpu_start(m_heap->GetCPUDescriptorHandleForHeapStart().ptr + index_start * m_descriptor_size);
	D3D12_GPU_DESCRIPTOR_HANDLE gpu_start{};
	if (m_is_shader_visible)
		gpu_start = D3D12_GPU_DESCRIPTOR_HANDLE(m_heap->GetGPUDescriptorHandleForHeapStart().ptr + index_start * m_descriptor_size);

	return DX12DescriptorChunk(
		m_heap_type,
		gpu_start,
		cpu_start,
		m_descriptor_size,
		num_descriptors,
		index_start);
};

DX12DescriptorChunk DX12DescriptorHeap::as_chunk() const
{
	return get_chunk(0, m_max_descriptors);
}



