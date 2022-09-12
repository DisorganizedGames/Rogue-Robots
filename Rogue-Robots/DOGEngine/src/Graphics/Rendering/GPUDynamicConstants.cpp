#include "GPUDynamicConstants.h"
#include "../RHI/RenderDevice.h"
#include "GPUGarbageBin.h"

namespace DOG::gfx
{
	GPUDynamicConstants::GPUDynamicConstants(RenderDevice* rd, GPUGarbageBin* bin, u32 maxElementsPerVersion) :
		m_rd(rd),
		m_bin(bin)
	{
		m_buffer = m_rd->CreateBuffer(BufferDesc(MemoryType::Upload, ELEMENTSIZE * maxElementsPerVersion * bin->GetMaxVersions()));
		m_ator = RingBuffer(ELEMENTSIZE, maxElementsPerVersion, m_rd->Map(m_buffer));
	}

	GPUDynamicConstant GPUDynamicConstants::Allocate()
	{
		// Grab memory
		auto [mem, offset] = m_ator.AllocateWithOffset();
		GPUDynamicConstant ret{};
		ret.memory = mem;

		// Create temporary view (potential performance hazard?)
		auto view = m_rd->CreateView(m_buffer, BufferViewDesc(ViewType::Constant, (u32)offset, ELEMENTSIZE, 1));
		ret.globalDescriptor = m_rd->GetGlobalDescriptor(view);

		// Safely delete later
		auto delFunc = [this, view]()
		{
			m_ator.Pop();
			m_rd->FreeView(view);
		};
		m_bin->PushDeferredDeletion(delFunc);

		return ret;
	}
}