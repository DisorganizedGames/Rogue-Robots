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

		GPUPoolMemoryInfo() = default;
		GPUPoolMemoryInfo(const GPUPoolMemoryInfo& other) = default;
		GPUPoolMemoryInfo& operator=(const GPUPoolMemoryInfo& other) = default;
		GPUPoolMemoryInfo operator+(const GPUPoolMemoryInfo& other) const
		{
			GPUPoolMemoryInfo res;
			res.numUnusedRange = numUnusedRange + other.numUnusedRange;
			res.smallestAllocation = smallestAllocation + other.smallestAllocation;
			res.largestAllocation = largestAllocation + other.largestAllocation;
			res.smallestUnusedRange = smallestUnusedRange + other.smallestUnusedRange;
			res.largestUnusedRange = largestUnusedRange + other.largestUnusedRange;
			res.blocksAllocated = blocksAllocated + other.blocksAllocated;
			res.numAllocations = numAllocations + other.numAllocations;
			res.blockBytes = blockBytes + other.blockBytes;
			res.allocationBytes = allocationBytes + other.allocationBytes;
			res.allocatedButUnusedBytes = allocatedButUnusedBytes + other.allocatedButUnusedBytes;
			return res;
		}
	};



	// GPU information for the whole device
	struct GPUTotalMemoryInfo
	{
		// DEFAULT, UPLOAD, READBACK, CUSTOM
		GPUPoolMemoryInfo heap[4];
		GPUPoolMemoryInfo total;
		DXGI_QUERY_VIDEO_MEMORY_INFO videoMemoryInfo;
	};


}