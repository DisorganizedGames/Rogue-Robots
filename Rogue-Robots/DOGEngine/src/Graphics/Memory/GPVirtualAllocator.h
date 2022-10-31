#pragma once
#include "../RHI/DX12/Utilities/D3D12MemAlloc.h"
#include <wrl/client.h>

namespace DOG::gfx
{
	/*

		General Purpose Virtual Allocator using D3D12MA
		We can emulate different types of allocators: https://gpuopen-librariesandsdks.github.io/D3D12MemoryAllocator/html/linear_algorithm.html
		(if linear algorithm is used)

	*/

	struct GPVirtualAllocation
	{
		u64 offset{ 0 };
		u64 size{ 0 };

	private:
		friend class GPVirtualAllocator;
		D3D12MA::VirtualAllocation alloc{};
	};

	class GPVirtualAllocator
	{
	public:
		GPVirtualAllocator() = default;
		GPVirtualAllocator(u64 maxSize, bool usesLinearAlgorithm);

		GPVirtualAllocation Allocate(u64 size, u32 alignment = 1);
		void Free(GPVirtualAllocation&& alloc);

	private:
		Microsoft::WRL::ComPtr<D3D12MA::VirtualBlock> m_dmaBlock{ nullptr };

	};
}