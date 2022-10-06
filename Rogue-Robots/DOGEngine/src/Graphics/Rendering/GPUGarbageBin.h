#pragma once
#include <functional>

namespace DOG::gfx
{

	class GPUGarbageBin
	{
	public:
		GPUGarbageBin(u8 maxFramesInFlight);

		void PushDeferredDeletion(const std::function<void()>& deletionFunc);

		void BeginFrame();
		void EndFrame();

		// Force clears all buffered delete requests
		void ForceClear();

		u8 GetMaxVersions() const { return m_maxFramesInFlight; }

	private:
		struct Deletion_Storage
		{
			u32 frameIdxOnRequest{ 0 };
			std::function<void()> func;
		};

		template <typename T>
		struct PrivateQueue
		{
			std::vector<T> m_reusable_keys;
			uint32_t m_tail{ 0 };
			uint32_t m_head{ 0 };

			PrivateQueue()
			{
				m_reusable_keys.resize(1);
			}

			void push(T value)
			{
				const uint32_t placement = m_head++;
				if (m_reusable_keys.size() < (m_head - m_tail))			// manual resizing
					m_reusable_keys.resize(m_reusable_keys.size() * 2);
				m_reusable_keys[placement] = value;
			}

			T& front()
			{
				//return m_reusable_keys[m_tail++];
				return m_reusable_keys[m_tail];
			}

			void pop()
			{
				++m_tail;
			}

			bool empty()
			{
				auto ret = m_head == m_tail;
				// if empty, re-use memory
				if (ret)
					m_head = m_tail = 0;
				return ret;
			}
		};

	private:
		u8 m_maxFramesInFlight{ 0 };
		u8 m_currFrameIdx{ 0 };
		//std::queue<Deletion_Storage> m_deletes;
		PrivateQueue<Deletion_Storage> m_deletes;

	};
}