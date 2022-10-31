#pragma once

namespace DOG::gfx
{

	class GPUGarbageBin
	{
	public:
		GPUGarbageBin(u8 maxFramesInFlight);

		void PushDeferredDeletion(std::function<void()>&& deletionFunc);

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



	private:
		u8 m_maxFramesInFlight{ 0 };
		u8 m_currFrameIdx{ 0 };
		std::queue<Deletion_Storage> m_deletes;
		//PrivateQueue<Deletion_Storage> m_deletes;
		//std::deque<Deletion_Storage> m_deletes;

		std::vector<std::vector<std::function<void()>>> m_deletes2;

	};
}