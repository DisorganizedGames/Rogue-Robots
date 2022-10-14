#pragma once

namespace DOG::gfx
{
	// GPU information per memory pool
	struct GPUPoolMemoryInfo
	{
		// Number of free ranges of memory between allocations
		u32 numUnusedRange{ 0 };
		/// `UINT64_MAX` if there are 0 allocations.
		u64 smallestAllocation{ 0 };
		/// 0 if there are 0 allocations.
		u64 largestAllocation{ 0 };
		// `UINT64_MAX` if there are 0 empty ranges.
		u64 smallestUnusedRange{ 0 };
		// 0 if there are 0 empty ranges.
		u64 largestUnusedRange{ 0 };

		u32 blocksAllocated{ 0 };		// Num GPU heaps
		u32 numAllocations{ 0 };		// Num D3D12MA::Allocation
		u32 blockBytes{ 0 };			// Bytes allocated in memory blocks
		u32 allocationBytes{ 0 };		// Size occupied by all D3D12MA::Allocations on this pool

		// Memory unused by D3D12MA::Allocations are calculated by: (blockBytes - allocationBytes)
		u32 allocatedButUnusedBytes{ 0 };
	};

	// GPU information for the whole device
	struct GPUTotalMemoryInfo
	{
		// DEFAULT, UPLOAD, READBACK, CUSTOM
		GPUPoolMemoryInfo heap[4];
		GPUPoolMemoryInfo total;
	};


}