#pragma once
#include <functional>
#include "../Handles/HandleAllocator.h"
#include "../Memory/GPVirtualAllocator.h"
#include "../Memory/GPAllocator.h"
#include "../RHI/RenderDevice.h"
#include "UploadContext.h"
#include "GPUGarbageBin.h"

namespace DOG::gfx
{
	/*
		Assumes 'Handle' type has a u64 member named "handle"

		Designed for CPU write-once, GPU read-many data
	*/
	template <typename Handle>
	class GPUTableDeviceLocal
	{
	private:
		struct Storage
		{
			GPVirtualAllocation alloc;			// Virtual allocation to element
			u64 elementOffset{ 0 };				// Offset in DataType strides
		};

	public:
		GPUTableDeviceLocal() = default;
		GPUTableDeviceLocal(RenderDevice* rd, GPUGarbageBin* bin, u32 elementSize, u32 maxElements, bool async = false) :
			m_rd(rd),
			m_bin(bin),
			m_elementSize(elementSize)
		{
			assert(rd != nullptr);
			assert(bin != nullptr);

			m_resources.resize(1);

			auto flag = async ? D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS : D3D12_RESOURCE_FLAG_NONE;
			m_buffer = rd->CreateBuffer(BufferDesc(MemoryType::Default, maxElements * m_elementSize, flag));
			m_vator = GPVirtualAllocator(maxElements * (u64)m_elementSize, false);

			m_fullView = rd->CreateView(m_buffer, BufferViewDesc(ViewType::ShaderResource, 0, elementSize, maxElements));
			m_fullViewDDA = rd->GetGlobalDescriptor(m_fullView);
		}

		~GPUTableDeviceLocal()
		{
			// Cleanup any allocs
			for (auto& res : m_resources)
			{
				if (res)
				{
					m_vator.Free(std::move(res->alloc));
				}
			}
		}

		/*
			Expose a way to allocate multiple elements to one handle!
			This way, we can for example allocate a whole mesh and keep a single handle!
		*/
		Handle Allocate(u32 numElements, void* initData = nullptr)
		{
			Storage res{};

			auto size = numElements * m_elementSize;

			// Allocate from virtual pool allocator
			res.alloc = m_vator.Allocate(size);
			res.elementOffset = res.alloc.offset / m_elementSize;

			// Enqueue data update
			auto updateFunc = [this, initData, size, offset = res.alloc.offset](UploadContext& ctx)
			{
				ctx.PushUpload(m_buffer, (u32)offset, initData, size);
			};
			m_updateRequests.push(updateFunc);

			// Reserve storage
			auto hdl = m_handleAtor.Allocate<Handle>();
			HandleAllocator::TryInsert(m_resources, res, HandleAllocator::GetSlot(hdl.handle));

			return hdl;
		}

		void Free(Handle handle)
		{
			auto& res = HandleAllocator::TryGet(m_resources, HandleAllocator::GetSlot(handle.handle));

			// Push for safe deletion
			auto delFunc = [this, alloc = res.alloc]() mutable
			{
				m_vator.Free(std::move(alloc));
			};
			m_bin->PushDeferredDeletion(delFunc);

			// Free handle immediately to invalidate further use
			HandleAllocator::FreeStorage(m_handleAtor, m_resources, handle);
		}

		u32 GetGlobalDescriptor() const
		{
			return m_fullViewDDA;
		}

		/*
			Assumes CPU data is stored elsewhere.
			The CPU data MUST be valid all the way until SendCopyRequests!
			This table does NOT take ownership of the data!
		*/
		void RequestUpdate(Handle handle, void* data, u32 dataSize)
		{
			auto& res = HandleAllocator::TryGet(m_resources, HandleAllocator::GetSlot(handle.handle));
			assert(dataSize <= res.alloc.size);

			// Invalidate internal GPU data
			auto delFunc = [this, alloc = std::move(res.alloc)]() mutable
			{
				m_vator.Free(std::move(alloc));
			};
			m_bin->PushDeferredDeletion(delFunc);

			// Allocate new one
			res.alloc = m_vator.Allocate(m_elementSize);
			res.elementOffset = res.alloc.offset / m_elementSize;

			// Enqueue data update
			auto updateFunc = [this, data, dataSize, offset = res.alloc.offset](UploadContext& ctx)
			{
				ctx.PushUpload(m_buffer, (u32)offset, data, dataSize);
			};
			m_updateRequests.push(updateFunc);
		}

		/*
			Designed local offset use:
				Binding a single buffer view for the whole table
				Using this offset to index into that local table
		*/
		u32 GetLocalOffset(Handle handle)
		{
			const auto& res = HandleAllocator::TryGet(m_resources, HandleAllocator::GetSlot(handle.handle));
			return (u32)res.elementOffset;
		}

		/*
			Sends the enqueued copy requests to the upload context
		*/
		void SendCopyRequests(UploadContext& ctx)
		{
			// Execute the requests to upload context
			while (!m_updateRequests.empty())
			{
				auto func = m_updateRequests.front();
				func(ctx);
				m_updateRequests.pop();
			}
		}

		// For LIMITED use!!
		Buffer GetBuffer() const { return m_buffer; }

	private:
		RenderDevice* m_rd{ nullptr };
		GPUGarbageBin* m_bin{ nullptr };
		u32 m_elementSize{ 0 };

		std::vector<std::optional<Storage>> m_resources;
		HandleAllocator m_handleAtor;
		std::queue<std::function<void(UploadContext&)>> m_updateRequests;

		Buffer m_buffer;
		BufferView m_fullView;
		u32 m_fullViewDDA{ 0 };
		GPVirtualAllocator m_vator;

	};

	/*
		Host visible version of GPUTableDeviceLocal
		For CPU write-once, GPU read-once data
	*/
	template <typename Handle>
	class GPUTableHostVisible
	{
	private:
		struct Storage
		{
			GPAllocation alloc;					// Allocation for element
			u32 elementOffset{ 0 };
		};

	public:
		GPUTableHostVisible(RenderDevice* rd, GPUGarbageBin* bin, u32 elementSize, u32 maxElements) :
			m_rd(rd),
			m_bin(bin),
			m_elementSize(elementSize)
		{
			assert(rd != nullptr);
			assert(bin != nullptr);

			m_resources.resize(1);

			m_buffer = rd->CreateBuffer(BufferDesc(MemoryType::Upload, maxElements * m_elementSize));
			m_ator = GPAllocator(maxElements * m_elementSize, false, m_rd->Map(m_buffer));

			m_fullView = rd->CreateView(m_buffer, BufferViewDesc(ViewType::ShaderResource, 0, elementSize, maxElements));
			m_fullViewDDA = rd->GetGlobalDescriptor(m_fullView);
		}

		~GPUTableHostVisible()
		{
			// Cleanup any allocs
			for (auto& res : m_resources)
			{
				if (res)
				{
					m_ator.Free(std::move(res->alloc));
				}
			}
		}

		Handle Allocate(u32 numElements, void* initData = nullptr)
		{
			Storage res{};
			res.alloc = m_ator.Allocate(numElements * m_elementSize);
			res.elementOffset = (u32)res.alloc.valloc.offset / m_elementSize;

			// Reserve storage
			auto hdl = m_handleAtor.Allocate<Handle>();
			HandleAllocator::TryInsert(m_resources, res, HandleAllocator::GetSlot(hdl.handle));

			// Direct copy
			if (!res.alloc.memory)
			{
				throw std::runtime_error("No res.alloc.memory");
			}
			if (initData)
				std::memcpy(res.alloc.memory, initData, m_elementSize * numElements);

			return hdl;
		}

		void Free(Handle handle)
		{
			auto& res = HandleAllocator::TryGet(m_resources, HandleAllocator::GetSlot(handle.handle));

			// Free safely
			auto delFunc = [this, alloc = std::move(res.alloc)]() mutable
			{
				m_ator.Free(std::move(alloc));
			};
			m_bin->PushDeferredDeletion(delFunc);

			// Invalidate handle directly
			HandleAllocator::FreeStorage(m_handleAtor, m_resources, handle);
		}

		void UpdateDirect(Handle handle, void* data, u32 dataSize)
		{
			auto& res = HandleAllocator::TryGet(m_resources, HandleAllocator::GetSlot(handle.handle));

			// Invalidate old 
			auto delFunc = [this, alloc = std::move(res.alloc)]() mutable
			{
				m_ator.Free(std::move(alloc));
			};
			m_bin->PushDeferredDeletion(delFunc);

			// Allocate new and copy to host visible memory
			res.alloc = m_ator.Allocate(dataSize);
			res.elementOffset = (u32)res.alloc.valloc.offset / m_elementSize;
			std::memcpy(res.alloc.memory, data, dataSize);
		}

		u32 GetGlobalDescriptor() const
		{
			return m_fullViewDDA;
		}

		u32 GetLocalOffset(Handle handle)
		{
			const auto& res = HandleAllocator::TryGet(m_resources, HandleAllocator::GetSlot(handle.handle));
			return (u32)res.elementOffset;
		}


	private:
		RenderDevice* m_rd{ nullptr };
		GPUGarbageBin* m_bin{ nullptr };
		u32 m_elementSize{ 0 };

		std::vector<std::optional<Storage>> m_resources;
		HandleAllocator m_handleAtor;

		Buffer m_buffer;
		GPAllocator m_ator;
		BufferView m_fullView;
		u32 m_fullViewDDA{ 0 };


	};
}