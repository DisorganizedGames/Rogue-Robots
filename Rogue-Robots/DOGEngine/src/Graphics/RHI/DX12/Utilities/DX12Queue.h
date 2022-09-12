#pragma once
#include "../CommonIncludes_DX12.h"
#include "DX12Fence.h"

class DX12Queue
{
public:
	DX12Queue() = default;
	DX12Queue(ID3D12Device* dev, D3D12_COMMAND_LIST_TYPE queue_type);

	void execute_command_lists(UINT num_lists, ID3D12CommandList* const* lists);
	void insert_signal(DX12Fence& fence);
	void insert_wait(DX12Fence& fence);

	// Inserts immediate CPU-wait
	void flush();

	UINT64 get_timestamp_freq() const;

	operator ID3D12CommandQueue* () const;

private:
	ComPtr<ID3D12CommandQueue> m_queue;

	DX12Fence m_flush_fence;
	UINT m_next_fence_value{ 1 };
};

