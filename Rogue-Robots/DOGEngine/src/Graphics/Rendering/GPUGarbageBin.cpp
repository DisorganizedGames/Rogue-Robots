#include "GPUGarbageBin.h"

#include "Tracy/Tracy.hpp"

namespace DOG::gfx
{
	GPUGarbageBin::GPUGarbageBin(u8 maxFramesInFlight) :
		m_maxFramesInFlight(maxFramesInFlight)
	{
	}

	void GPUGarbageBin::PushDeferredDeletion(const std::function<void()>& deletionFunc)
	{
		ZoneScopedN("GPU Garbage Bin: Add Deferred Deletion");


		Deletion_Storage storage{};
		storage.frameIdxOnRequest = m_currFrameIdx;
		storage.func = deletionFunc;

		m_deletes.push(storage);
	}

	void GPUGarbageBin::BeginFrame()
	{
		ZoneScopedN("GPU Garbage Bin: Clear");
		/*
			Assuming deletes are always grouped contiguously:
			[ 0, 0, 0, 1, 1, 1, 2, 2, 2, 0, 0, 0, ...]
		*/
		while (!m_deletes.empty())
		{
			auto& storage = m_deletes.front();
			if (storage.frameIdxOnRequest == m_currFrameIdx)
			{
				storage.func();	// delete
				m_deletes.pop();
			}
			else
			{
				break;
			}
		}
	}

	void GPUGarbageBin::EndFrame()
	{
		m_currFrameIdx = (m_currFrameIdx + 1) % m_maxFramesInFlight;
	}

	void GPUGarbageBin::ForceClear()
	{
		while (!m_deletes.empty())
		{
			auto& storage = m_deletes.front();
			storage.func();	// delete
			m_deletes.pop();
		}
	}
}