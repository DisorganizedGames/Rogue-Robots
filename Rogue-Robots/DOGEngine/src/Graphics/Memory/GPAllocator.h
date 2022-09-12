#pragma once
#include "GPVirtualAllocator.h"

namespace DOG::gfx
{
	struct GPAllocation
	{
		GPVirtualAllocation valloc{};
		u8* memory{ nullptr };
	};

	/*
		General Purpose Allocator
	*/
	class GPAllocator
	{
	public:
		GPAllocator() = default;
		GPAllocator(u64 maxSize, bool usesLinearAlgorithm, u8* memory = nullptr);

		GPAllocation Allocate(u64 size);
		void Free(GPAllocation&& alloc);

		~GPAllocator();

	private:
		bool m_isInternallyManaged{ false };
		u8* m_heapStart{ nullptr };

		GPVirtualAllocator m_vator;
	};
}
