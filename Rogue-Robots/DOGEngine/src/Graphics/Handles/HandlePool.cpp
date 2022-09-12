#include "HandlePool.h"

namespace DOG::gfx
{

	HandlePool::HandlePool()
	{
		// Reserve index 0 as invalid handle
		m_gen_counters.push_back(0);
	}

	uint64_t HandlePool::allocate_handle()
	{
		uint32_t key{ 0 };
		uint32_t gen{ 0 };

		if (m_reusable_keys.empty())
		{
			// add new usable handle to counters list
			m_gen_counters.push_back(gen);

			// assign key
			key = (uint32_t)m_gen_counters.size() - 1;
		}
		else
		{
			key = m_reusable_keys.top();
			m_reusable_keys.pop();

			gen = m_gen_counters[key];
		}

		// construct full key (generation is upper 32-bits, slot is lower 32-bits)
		const uint64_t handle = (((uint64_t)gen) << INDEX_SHIFT) | (((uint64_t)key) & SLOT_MASK);

		return handle;
	}

	void HandlePool::free_handle(uint64_t handle)
	{
		const uint32_t key = (uint32_t)(handle & SLOT_MASK);
		const uint32_t gen = (uint32_t)((handle & GENERATION_MASK) >> INDEX_SHIFT);

		// most recent generation
		const uint32_t curr_gen = m_gen_counters[key];

		// tackle double-free
		// we check that the handle passed in is the most recent generation (current)
		assert(curr_gen == gen);

		// increase generational counter for next-reuse
		// also handle overflow, which in this case skips pushing it to reusable queue
		const uint32_t next_gen = ++m_gen_counters[key];
		if (next_gen < curr_gen)
			return;

		m_reusable_keys.push(key);
	}
}