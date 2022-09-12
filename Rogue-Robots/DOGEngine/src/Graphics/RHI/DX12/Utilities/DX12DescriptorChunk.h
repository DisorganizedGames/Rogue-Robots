#pragma once
#include "../CommonIncludes_DX12.h"
#include <optional>

class DX12DescriptorChunk
{
public:
	DX12DescriptorChunk() = default;
	DX12DescriptorChunk(
		D3D12_DESCRIPTOR_HEAP_TYPE heap_type,
		std::optional<D3D12_GPU_DESCRIPTOR_HANDLE> gpu_handle,
		D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle,
		uint32_t descriptor_size,
		uint32_t num_descriptors,
		uint64_t index_offset_from_base) :
		m_heap_type(heap_type),
		m_cpu_handle(cpu_handle),
		m_gpu_handle(gpu_handle),
		m_num_descriptors(num_descriptors),
		m_descriptor_size(descriptor_size),
		m_index_offset_from_base(index_offset_from_base) {}

	uint32_t num_descriptors() const { return m_num_descriptors; }
	uint32_t descriptor_size() const { return m_descriptor_size; }
	uint64_t index_offset_from_base(uint32_t offset = 0) const { return m_index_offset_from_base + offset; }
	D3D12_DESCRIPTOR_HEAP_TYPE heap_type() const { return m_heap_type; }

	bool is_gpu_visible() const { return m_gpu_handle.has_value(); }

	D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle(uint64_t index_offset) const
	{
		assert(index_offset < m_num_descriptors);
		return D3D12_CPU_DESCRIPTOR_HANDLE(m_cpu_handle.ptr + m_descriptor_size * index_offset);
	}
	D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle(uint64_t index_offset) const
	{
		assert(is_gpu_visible());
		assert(index_offset < m_num_descriptors);
		return D3D12_GPU_DESCRIPTOR_HANDLE((*m_gpu_handle).ptr + m_descriptor_size * index_offset);
	}

	DX12DescriptorChunk get_subchunk(uint64_t index_offset, uint32_t num_descriptors) const
	{
		assert((index_offset + num_descriptors - 1) < m_num_descriptors);

		return DX12DescriptorChunk(
			m_heap_type,
			gpu_handle(index_offset),
			cpu_handle(index_offset),
			m_descriptor_size,
			num_descriptors,
			m_index_offset_from_base + index_offset);
	}

	void set_allocator_key(uint64_t key) { m_private_data = key; }
	uint64_t get_allocator_key() { return m_private_data; }

private:
	D3D12_DESCRIPTOR_HEAP_TYPE m_heap_type;

	D3D12_CPU_DESCRIPTOR_HANDLE m_cpu_handle{};
	std::optional<D3D12_GPU_DESCRIPTOR_HANDLE> m_gpu_handle{};

	uint32_t m_num_descriptors{ 0 };
	uint32_t m_descriptor_size{ 0 };
	uint64_t m_index_offset_from_base{ 0 };

	uint64_t m_private_data{ UINT_MAX };


};


