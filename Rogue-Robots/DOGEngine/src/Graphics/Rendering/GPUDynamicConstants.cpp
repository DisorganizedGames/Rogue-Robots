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

	GPUDynamicConstants::~GPUDynamicConstants()
	{
		m_bin->PushDeferredDeletion([rd = m_rd, buf = m_buffer]()
			{
				rd->FreeBuffer(buf);
			});
	}


	void GPUDynamicConstants::Tick()
	{
		for (u32 i = 0; i < m_numToPop; ++i)
			m_ator.Pop();
		m_numToPop = 0;
	}

	GPUDynamicConstant GPUDynamicConstants::Allocate(u32 count, bool generateDescriptor)
	{
		if (count == 0)
			return {};
		// Grab memory (base)
		auto [mem, offset] = m_ator.AllocateWithOffset();
		assert(mem);
		++m_numToPop;
		GPUDynamicConstant ret{};
		ret.memory = mem;


		// Allocate rest
		u8* valid{ nullptr };
		for (u32 i = 0; i < count - 1; ++i)
		{
			valid = m_ator.Allocate();
			++m_numToPop;
		}
		if (count > 1)
			assert(valid);

		// Yes it is a big performance hazard
		// Create temporary view (potential performance hazard?)
		if (generateDescriptor)
		{
			auto view = m_rd->CreateView(m_buffer, BufferViewDesc(ViewType::Constant, (u32)offset, ELEMENTSIZE * count, 1));
			ret.globalDescriptor = m_rd->GetGlobalDescriptor(view);

			// Safely delete later
			auto delFunc = [this, view, toPop = count]()
			{
				m_rd->FreeView(view);
			};
			m_bin->PushDeferredDeletion(delFunc);
		}


		//auto delFunc = [this, toPop = count]()
		//{
		//	for (u32 i = 0; i < toPop; ++i)
		//		m_ator.Pop();
		//};
		//m_bin->PushDeferredDeletion(delFunc);

		ret.buffer = m_buffer;
		ret.bufferOffset = (u32)offset;


		return ret;
	}
}