#include "RingBuffer.h"

namespace DOG::gfx
{
	RingBuffer::RingBuffer(u32 element_size, u32 element_count, u8* memory) :
		m_heapStart(memory),
		m_elementSize(element_size),
		m_isInternallyManaged(memory == nullptr ? true : false),
		m_vator(element_size, element_count)
	{
		if (!memory)
		{
			m_heapStart = (u8*)std::malloc(element_size * element_count);
			if (m_heapStart)
				std::memset(m_heapStart, 0, element_size * element_count);
		}

		m_vator = std::move(VirtualRingBuffer(element_size, element_count));
	}

	RingBuffer::~RingBuffer()
	{
		if (m_isInternallyManaged)
			std::free(m_heapStart);
	}

	u8* RingBuffer::Allocate()
	{
		auto offset = m_vator.Allocate();
		if (offset == std::numeric_limits<u64>::max())
			return nullptr;
		return m_heapStart + offset;
	}

	std::pair<u8*, u64> RingBuffer::AllocateWithOffset()
	{
		auto offset = m_vator.Allocate();
		if (offset == std::numeric_limits<u64>::max())
			return { nullptr, std::numeric_limits<u64>::max() };
		return { m_heapStart + offset, offset };
	}

	u8* RingBuffer::Pop()
	{
		auto offset = m_vator.Pop();
		if (offset == (u64)-1)
			return nullptr;
		return m_heapStart + offset;
	}
}