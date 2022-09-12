#include "DX12Queue.h"

DX12Queue::DX12Queue(ID3D12Device* dev, D3D12_COMMAND_LIST_TYPE queueType) :
	m_flush_fence(dev, 0)
{
	HRESULT hr = S_OK;
	D3D12_COMMAND_QUEUE_DESC cqd{};
	cqd.Type = queueType;
	hr = dev->CreateCommandQueue(&cqd, IID_PPV_ARGS(m_queue.GetAddressOf()));

	assert(SUCCEEDED(hr));
}


void DX12Queue::execute_command_lists(UINT num_lists, ID3D12CommandList* const* lists)
{
	m_queue->ExecuteCommandLists(num_lists, lists);
}

void DX12Queue::insert_signal(DX12Fence& fence)
{
	fence.signal(m_queue.Get(), m_next_fence_value++);
}

void DX12Queue::insert_wait(DX12Fence& fence)
{
	fence.gpu_wait(m_queue.Get());
}

void DX12Queue::flush()
{
	DX12Queue::insert_signal(m_flush_fence);
	m_flush_fence.cpu_wait();
}

UINT64 DX12Queue::get_timestamp_freq() const
{
	HRESULT hr = S_OK;
	UINT64 freq;
	hr = m_queue->GetTimestampFrequency(&freq);
	assert(SUCCEEDED(hr));
	return freq;
}

DX12Queue::operator ID3D12CommandQueue* () const
{
	return m_queue.Get();
}
