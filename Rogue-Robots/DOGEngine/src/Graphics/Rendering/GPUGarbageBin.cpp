#include "GPUGarbageBin.h"

//#include "Tracy/Tracy.hpp"

namespace DOG::gfx
{
	GPUGarbageBin::GPUGarbageBin(u8 maxFramesInFlight) :
		m_maxFramesInFlight(maxFramesInFlight)
	{
		m_deletes2.resize(maxFramesInFlight);
	}

	void GPUGarbageBin::PushDeferredDeletion(std::function<void()>&& deletionFunc)
	{
		//ZoneScopedN("GPU Garbage Bin: Add Deferred Deletion");


		//Deletion_Storage storage{};
		//storage.frameIdxOnRequest = m_currFrameIdx;
		//storage.func = deletionFunc;

		//m_deletes.push(storage);

		m_deletes2[m_currFrameIdx].push_back(deletionFunc);
	}

	void GPUGarbageBin::BeginFrame()
	{
		/*
			This is REALLY SLOW!
		*/
		//ZoneScopedN("GPU Garbage Bin: Clear");
		/*
			Assuming deletes are always grouped contiguously:
			[ 0, 0, 0, 1, 1, 1, 2, 2, 2, 0, 0, 0, ...]
		*/
		//while (!m_deletes.empty())
		//{
		//	ZoneNamedN(BinPerDelete, "GPU Garbage Bin: Per Deferred Deletion", true);
		//	auto& storage = m_deletes.front();
		//	if (storage.frameIdxOnRequest != m_currFrameIdx)
		//		break;

		//	storage.func();	// delete
		//	m_deletes.pop();
		//}

		for (const auto& f : m_deletes2[m_currFrameIdx])
		{
			//ZoneNamedN(BinPerDelete, "GPU Garbage Bin: Per Deferred Deletion", true);
			f();
		}
		m_deletes2[m_currFrameIdx].clear();
	}

	void GPUGarbageBin::EndFrame()
	{
		m_currFrameIdx = (m_currFrameIdx + 1) % m_maxFramesInFlight;
	}

	void GPUGarbageBin::ForceClear()
	{
		//while (!m_deletes.empty())
		//{
		//	auto& storage = m_deletes.front();
		//	storage.func();	// delete
		//	m_deletes.pop();
		//}

		for (auto& deletes : m_deletes2)
		{
			for (const auto& f : deletes)
				f();
			deletes.clear();
		}
	}
}