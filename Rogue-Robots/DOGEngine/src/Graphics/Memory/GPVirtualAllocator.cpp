#include "GPVirtualAllocator.h"

namespace DOG::gfx
{
	GPVirtualAllocator::GPVirtualAllocator(u64 maxSize, bool usesLinearAlgorithm)
	{
		D3D12MA::VIRTUAL_BLOCK_DESC blockDesc{};
		blockDesc.Size = maxSize;
		blockDesc.Flags = usesLinearAlgorithm ? D3D12MA::VIRTUAL_BLOCK_FLAG_ALGORITHM_LINEAR : D3D12MA::VIRTUAL_BLOCK_FLAG_NONE;

		HR hr = CreateVirtualBlock(&blockDesc, m_dmaBlock.GetAddressOf());
		hr.try_fail("Failed to create virtual block");
	}

	GPVirtualAllocation GPVirtualAllocator::Allocate(u64 size, u32 alignment)
	{
		HRESULT hr = S_OK;

		D3D12MA::VIRTUAL_ALLOCATION_DESC allocDesc{};
		allocDesc.Size = size;
		allocDesc.Alignment = alignment;

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