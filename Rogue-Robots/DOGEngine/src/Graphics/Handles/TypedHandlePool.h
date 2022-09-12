#pragma once
#include "HandlePool.h"

namespace DOG::gfx
{
	/*
		Expects a type T which has a u64 member "handle"
		Also expects type T to befriend this class.
	*/
	class TypedHandlePool
	{
	public:
		template <typename T>
		[[nodiscard]] T allocate_handle()
		{
			auto handle = m_hp.allocate_handle();

			T strong_handle{};
			strong_handle.handle = handle;
			return strong_handle;
		}

		template <typename T>
		void free_handle(T&& handle)
		{
			m_hp.free_handle(handle.handle);
		}

	private:
		HandlePool m_hp;
	};
}
