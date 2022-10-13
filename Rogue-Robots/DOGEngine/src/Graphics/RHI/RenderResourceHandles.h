#pragma once

namespace DOG::gfx
{

	class TypedHandlePool;

	struct Buffer { friend TypedHandlePool; u64 handle{ 0 }; };
	struct Texture { friend TypedHandlePool; u64 handle{ 0 }; };
	struct Pipeline { friend TypedHandlePool; u64 handle{ 0 }; };
	struct RenderPass { friend TypedHandlePool; u64 handle{ 0 }; };

	struct SyncReceipt { friend TypedHandlePool; u64 handle{ 0 }; };
	struct BufferView { friend TypedHandlePool; u64 handle{ 0 }; };
	struct TextureView { friend TypedHandlePool; u64 handle{ 0 }; };

	struct CommandList { friend TypedHandlePool; u64 handle{ 0 }; };
	struct MemoryPool { friend TypedHandlePool; u64 handle{ 0 }; };
}
