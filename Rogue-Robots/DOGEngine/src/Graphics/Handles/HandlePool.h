#pragma once
#include <stdint.h>
#include <vector>
#include <stack>
#include <assert.h>

namespace DOG::gfx
{
	/*
		64-bit handle pools

		Lower 32-bits is used as the actual index. Referred to as "keys"
		Upper 32-bits is used as a generational counter. Refered to as "counters"

		Currently NOT threadsafe
	*/

	class HandlePool
	{
	public:
		HandlePool();

		uint64_t allocate_handle();
		void free_handle(uint64_t handle);

	private:
		template <typename T>
		class PrivateStack
		{
		public:
			void push(T value)
			{
				const uint32_t placement = m_end++;

				// manual resizing if needed
				if (m_reusable_keys.size() < m_end)
					m_reusable_keys.resize(m_reusable_keys.size() * 2);

				m_reusable_keys[placement] = value;
			}

			T top()
			{
				assert(!empty());
				return m_reusable_keys[m_end - 1];
			}

			void pop()
			{
				assert(!empty());
				--m_end;
			}

			bool empty()
			{
				return m_end == 0;
			}

		private:
			std::vector<T> m_reusable_keys{ 0 };
			uint32_t m_end{ 0 };
		};

	private:
		PrivateStack<uint32_t> m_reusable_keys;
		std::vector<uint32_t> m_gen_counters;

		const uint64_t INDEX_SHIFT = std::numeric_limits<uint32_t>::digits;
		const uint64_t SLOT_MASK = ((uint64_t)1 << INDEX_SHIFT) - 1;
		const uint64_t GENERATION_MASK = ~SLOT_MASK;

	};
}

