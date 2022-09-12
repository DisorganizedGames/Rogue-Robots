#include "GPVirtualAllocator.h"

namespace DOG::gfx
{
	GPVirtualAllocator::GPVirtualAllocator(u64 maxSize, bool usesLinearAlgorithm)
	{
		D3D12MA::VIRTUAL_BLOCK_DESC blockDesc{};
		blockDesc.Size = maxSize;
		blockDesc.Flags = usesLinearAlgorithm ? D3D12MA::VIRTUAL_BLOCK_FLAG_ALGORITHM_LINEAR : D3D12MA::VIRTUAL_BLOCK_FLAG_NONE;

		HRESULT hr = CreateVirtualBlock(&blockDesc, m_dmaBlock.GetAddressOf());
		assert(SUCCEEDED(hr));
	}

	GPVirtualAllocation GPVirtualAllocator::Allocate(u64 size)
	{
		HRESULT hr = S_OK;

		D3D12MA::VIRTUAL_ALLOCATION_DESC allocDesc{};
		allocDesc.Size = size;

		D3D12MA::VirtualAllocation dmaAlloc;
		UINT64 allocOffset;
		hr = m_dmaBlock->Allocate(&allocDesc, &dmaAlloc, &allocOffset);
		if (SUCCEEDED(hr))
		{
			GPVirtualAllocation alloc{};
			alloc.offset = allocOffset;
			alloc.size = size;
			alloc.alloc = dmaAlloc;
			return alloc;
		}
		else
		{
			// Allocation failed - no space for it could be found.
			assert(false);
			return GPVirtualAllocation();		// size == 0
		}
	}

	void GPVirtualAllocator::Free(GPVirtualAllocation&& alloc)
	{
		m_dmaBlock->FreeAllocation(alloc.alloc);
	}
}