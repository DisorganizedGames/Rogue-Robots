#pragma once
#include "../RHI/RenderResourceHandles.h"
#include "../Memory/RingBuffer.h"

namespace DOG::gfx
{
	class RenderDevice;
	class GPUGarbageBin;

	struct GPUDynamicConstant
	{
		u8* memory{ nullptr };
		u32 globalDescriptor{ 0 };
	};

	class GPUDynamicConstants
	{
	private:
		static constexpr u16 ELEMENTSIZE = 256;
	public:
		GPUDynamicConstants(RenderDevice* rd, GPUGarbageBin* bin, u32 maxElementsPerVersion);

		// Grab a 256 byte constant data
		GPUDynamicConstant Allocate();

	private:
		RenderDevice* m_rd{ nullptr };
		GPUGarbageBin* m_bin{ nullptr };

		Buffer m_buffer;
		RingBuffer m_ator;
	};
}
