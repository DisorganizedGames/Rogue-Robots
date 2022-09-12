#pragma once
#include "TypedHandlePool.h"
#include <unordered_map>
#include <typeindex>

namespace DOG::gfx
{
	/*
		This allocator intended for use per 'domain'.

		For example, the graphics API may provide "Buffer", "Texture", "Pipeline" types, which can all fall under a single allocator.

		A higher level API may instead provide "Mesh" or "Material" which can fall under another allocator.
	*/
	class HandleAllocator
	{
	public:
		template <typename Handle>
		Handle Allocate()
		{
			// creates new pool if none exists, otherwise uses existing
			auto& pool = m_pools[typeid(Handle)];
			return pool.allocate_handle<Handle>();
		}

		template <typename Handle>
		void Free(Handle&& handle)
		{
			auto& pool = m_pools[typeid(Handle)];
			pool.free_handle<Handle>(handle);
		}

		// Static helpers
	public:
		static u32 GetSlot(u64 handle)
		{
			static const u64 SLOT_MASK = ((u64)1 << std::numeric_limits<uint32_t>::digits) - 1; // Mask of the lower 32-bits
			return (u32)(handle & SLOT_MASK);
		}

		// Helper for inserting/getting resources in an array-like manner
		template <typename T>
		static void TryInsert(std::vector<std::optional<T>>& vec, const T& element, u32 index)
		{
			// resize if needed
			if (vec.size() <= index)
				vec.resize(vec.size() * 2);

			assert(!vec[index].has_value());
			vec[index] = element;
		}

		// move version
		template <typename T>
		static void TryInsertMove(std::vector<std::optional<T>>& vec, T&& element, u32 index)
		{
			// resize if needed
			if (vec.size() <= index)
				vec.resize(vec.size() * 4);

			assert(!vec[index].has_value());
			vec[index] = std::move(element);
		}

		template <typename T>
		static const T& TryGet(const std::vector<std::optional<T>>& vec, u32 index)
		{
			assert(vec.size() > index);
			assert(vec[index].has_value());
			return *(vec[index]);
		}

		template <typename T>
		static T& TryGet(std::vector<std::optional<T>>& vec, u32 index)
		{
			assert(vec.size() > index);
			assert(vec[index].has_value());
			return *(vec[index]);
		}

		template <typename StorageType, typename HandleType>
		static void FreeStorage(HandleAllocator& ator, std::vector<std::optional<StorageType>>& storage, HandleType handle)
		{
			const auto slot = HandleAllocator::GetSlot(handle.handle);
			//auto& res = HandleAllocator::TryGet(storage, slot);

			storage[slot] = std::nullopt;
			ator.Free(handle);
		}

	private:
		std::unordered_map<std::type_index, TypedHandlePool> m_pools;

	};
}

