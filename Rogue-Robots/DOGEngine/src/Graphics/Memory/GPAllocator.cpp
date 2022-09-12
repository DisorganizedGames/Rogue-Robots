#include "GPAllocator.h"

namespace DOG::gfx
{
	GPAllocator::GPAllocator(u64 maxSize, bool usesLinearAlgorithm, u8* memory) :
		m_isInternallyManaged(memory == nullptr)
	{
		if (!memory)
		{
			m_heapStart = (u8*)std::malloc(maxSize);
			if (m_heapStart)
				std::memset(m_heapStart, 0, maxSize);
		}
		else
		{
			m_heapStart = memory;
		}

		m_vator = GPVirtualAllocator(maxSize, usesLinearAlgorithm);
	}

	GPAllocation GPAllocator::Allocate(u64 size)
	{
		GPAllocation alloc{};

		alloc.valloc = m_vator.Allocate(size);
		alloc.memory = m_heapStart + alloc.valloc.offset;

		return alloc;
	}

	void GPAllocator::Free(GPAllocation&& alloc)
	{
		m_vator.Free(std::move(alloc.valloc));
	}

	GPAllocator::~GPAllocator()
	{
		if (m_isInternallyManaged)
			std::free(m_heapStart);
	}
}