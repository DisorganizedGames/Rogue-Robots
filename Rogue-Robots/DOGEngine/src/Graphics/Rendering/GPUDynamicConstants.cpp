#include "GPUDynamicConstants.h"
#include "../RHI/RenderDevice.h"
#include "GPUGarbageBin.h"

namespace DOG::gfx
{
	GPUDynamicConstants::GPUDynamicConstants(RenderDevice* rd, GPUGarbageBin* bin, u32 maxTotalElements) :
		m_rd(rd),
		m_bin(bin)
	{
		m_buffer = m_rd->CreateBuffer(BufferDesc(MemoryType::Upload, ELEMENTSIZE * maxTotalElements * bin->GetMaxVersions()));
		m_ator = RingBuffer(ELEMENTSIZE, maxTotalElements, m_rd->Map(m_buffer));
	}

	GPUDynamicConstant GPUDynamicConstants::Allocate(u32 count)
	{
		if (count == 0)
			return {};
		if (count == 33)
			auto x = 0;
		// Grab memory (base)
		auto [mem, offset] = m_ator.AllocateWithOffset();
		GPUDynamicConstant ret{};
		ret.memory = mem;

		// Allocate rest
		for (u32 i = 0; i < count - 1; ++i)
			m_ator.Allocate();

		// Create temporary view (potential performance hazard?)
		auto view = m_rd->CreateView(m_buffer, BufferViewDesc(ViewType::Constant, (u32)offset, ELEMENTSIZE * count, 1));
		ret.globalDescriptor = m_rd->GetGlobalDescriptor(view);

		// Safely delete later
		auto delFunc = [this, view, toPop = count]()
		{
			for (u32 i = 0; i < toPop; ++i)
				m_ator.Pop();
			m_rd->FreeView(view);
		};
		m_bin->PushDeferredDeletion(delFunc);

		return ret;
	}
}