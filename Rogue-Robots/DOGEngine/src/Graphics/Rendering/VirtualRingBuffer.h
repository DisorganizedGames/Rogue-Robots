#pragma once
namespace DOG::gfx
{
	class VirtualRingBuffer
	{
	public:
		VirtualRingBuffer() = default;
		VirtualRingBuffer(u32 element_size, u32 element_count);

		u64 Allocate();
		u64 Pop();

		u32 GetElementSize() const { return m_elementSize; }

	private:
		bool IsFull() const;
		bool IsEmpty() const;

	private:
		u64 m_totalSize{ 0 };
		u32 m_elementSize{ 0 };
		u32 m_elementCount{ 0 };

		u64 m_head{ 0 };
		u64 m_tail{ 0 };
		bool m_full{ false };
	};
}