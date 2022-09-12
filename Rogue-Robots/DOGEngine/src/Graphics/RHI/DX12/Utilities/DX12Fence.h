#pragma once
#include "../CommonIncludes_DX12.h"

class DX12Fence
{
public:
	DX12Fence() = default;
	DX12Fence(ID3D12Device* dev, u32 init_value, D3D12_FENCE_FLAGS flags = D3D12_FENCE_FLAG_NONE);

	// Signals this fence with 'value' once this position in the queue has been reached
	// (All commands prior to this position are guaranteed to have finished their execution)
	bool signal(ID3D12CommandQueue* queue, u32 value);

	// Waits for a signaled value on the GPU
	bool gpu_wait(ID3D12CommandQueue* queue) const;

	// Waits for a signaled value on the CPU
	bool cpu_wait() const;

	operator ID3D12Fence* () const;

private:
	ComPtr<ID3D12Fence> m_fence;
	HANDLE m_event{ nullptr };
	u32 m_wait_for_value = 0;
};

