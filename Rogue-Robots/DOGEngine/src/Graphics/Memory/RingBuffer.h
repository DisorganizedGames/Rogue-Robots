#pragma once
#include "VirtualRingBuffer.h"

namespace DOG::gfx
{
	class RingBuffer
	{
	public:
		RingBuffer() = default;
		RingBuffer(u32 element_size, u32 element_count, u8* memory = nullptr);
		~RingBuffer();

		u8* Allocate();
		std::pair<u8*, u64> AllocateWithOffset();
		u8* Pop();

		u32 GetElementSize() const { return m_elementSize; }

	private:
		u8* m_heapStart{ nullptr };

		u32 m_elementSize{ 0 };
		bool m_isInternallyManaged{ false };

		VirtualRingBuffer m_vator;
	};
}