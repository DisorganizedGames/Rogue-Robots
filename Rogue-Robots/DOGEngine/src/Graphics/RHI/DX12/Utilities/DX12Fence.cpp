#include "DX12Fence.h"

DX12Fence::DX12Fence(ID3D12Device* dev, u32 init_value, D3D12_FENCE_FLAGS flags)
{
	HRESULT hr = S_OK;
	hr = dev->CreateFence(
		init_value, flags, IID_PPV_ARGS(m_fence.GetAddressOf()));

	assert(SUCCEEDED(hr));

	// Create dedicated event to use for this fence
	m_event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
}


bool DX12Fence::signal(ID3D12CommandQueue* queue, UINT value)
{
	m_wait_for_value = value;
	HRESULT hr = queue->Signal(m_fence.Get(), value);
	return SUCCEEDED(hr);
}


bool DX12Fence::gpu_wait(ID3D12CommandQueue* queue) const
{
	HRESULT hr = queue->Wait(m_fence.Get(), m_wait_for_value);
	return SUCCEEDED(hr);
}

bool DX12Fence::cpu_wait() const
{
	if (m_fence->GetCompletedValue() < m_wait_for_value)
	{
		// Raise an event when fence reaches the value
		HRESULT hr = m_fence->SetEventOnCompletion(m_wait_for_value, m_event);
		if (FAILED(hr))
			return false;

		// CPU block until event is done
		WaitForSingleObject(m_event, INFINITE);
	}

	return true;
}

DX12Fence::operator ID3D12Fence* () const
{
	return m_fence.Get();
}
