#pragma once
#include "VirtualBumpAllocator.h"

namespace DOG::gfx
{
	class BumpAllocator
	{
	public:
		BumpAllocator() = default;

		BumpAllocator(u64 size, u8* memory = nullptr) :
			m_vator(size),
			m_size(size),
			m_internallyManagedMemory(memory == nullptr ? true : false)
		{
			if (m_internallyManagedMemory)
			{
				m_heapStart = (u8*)std::malloc(size);
			}
			else
			{
				m_heapStart = memory;
			}

			m_heapEnd = m_heapStart + size;

			if (m_heapStart == nullptr)
			{
				throw std::runtime_error("Bump allocators heap start was nullptr");
			}
			std::memset(m_heapStart, 0, size);
		}

		~BumpAllocator()
		{
			if (m_internallyManagedMemory)
				std::free(m_heapStart);
		}

		[[nodiscard]] u8* Allocate(u64 size, u16 alignment = 1)
		{
			u64 offset = m_vator.Allocate(size, alignment);
			return m_heapStart + offset;
		}

		// { memory, offset from base } 
		[[nodiscard]] std::pair<u8*, u64> AllocateWithOffset(u64 size, u16 alignment = 1)
		{
			u64 offset = m_vator.Allocate(size, alignment);
			return { m_heapStart + offset, offset };
		}

		void Clear()
		{
			m_vator.clear();
		}

	private:
		VirtualBumpAllocator m_vator;

		u64 m_size{ 0 };
		bool m_internallyManagedMemory{ false };

		u8* m_heapStart{ nullptr };
		u8* m_heapEnd{ nullptr };
	};
}