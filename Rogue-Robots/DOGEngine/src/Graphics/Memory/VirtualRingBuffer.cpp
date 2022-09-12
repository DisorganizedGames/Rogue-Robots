#include "VirtualRingBuffer.h"

namespace DOG::gfx
{
	VirtualRingBuffer::VirtualRingBuffer(u32 element_size, u32 element_count) :
		m_totalSize((u64)element_size* element_count),
		m_elementSize(element_size),
		m_elementCount(element_count)
	{
		assert(m_elementCount != 0);
		assert(m_elementSize != 0);
	}

	u64 VirtualRingBuffer::Allocate()
	{
		const u64 requestedStart = m_head;
		const u64 offset = requestedStart * m_elementSize;
		const u64 nextHead = (requestedStart + 1) % m_elementCount;

		if (requestedStart == m_tail && IsFull())
			return (u64)-1;

		m_head = nextHead;
		m_full = nextHead == m_tail;

		return offset;
	}

	u64 VirtualRingBuffer::Pop()
	{
		if (IsEmpty())
			return (u64)-1;

		const u64 offset = m_tail * m_elementSize;

		m_tail = (m_tail + 1) % m_elementCount;
		m_full = false;

		return offset;
	}

	bool VirtualRingBuffer::IsFull() const
	{
		return m_full;
	}

	bool VirtualRingBuffer::IsEmpty() const
	{
		return !m_full && (m_head == m_tail);
	}
}