#include "RGResourceRepo.h"
#include "../../RHI/RenderDevice.h"
#include "../../Rendering/GPUGarbageBin.h"

namespace DOG::gfx
{
	RGResourceRepo::RGResourceRepo(RenderDevice* rd, GPUGarbageBin* bin) :
		m_rd(rd),
		m_bin(bin)
	{
		m_buffers.resize(1);
		m_textures.resize(1);
		m_aliasGroups.resize(1);
	}

	void RGResourceRepo::Tick()
	{
		// Condition for resource deletion
		auto shouldDelete = [this](u64 lastFrameAccess) -> bool
		{
			return m_currFrame > (lastFrameAccess + MAX_UNUSED_RESOURCE_LIFETIME);
		};

		// Delete if possible (naive iteration here)
		for (auto& res : m_buffers)
		{	
			if (res && res->realized && shouldDelete(res->lastFrameAccess))
			{
				auto delFunc = [resMoved = std::move(res), this]() mutable
				{
					m_rd->FreeBuffer(*resMoved->realized);
				};
				m_bin->PushDeferredDeletion(delFunc);

			}
		}

		for (auto& res : m_textures)
		{
			if (res && res->realized && shouldDelete(res->lastFrameAccess))
			{
				auto delFunc = [resMoved = std::move(res), this]() mutable
				{
					m_rd->FreeTexture(*resMoved->realized);
				};
				m_bin->PushDeferredDeletion(delFunc);
			}
		}

	}

	RGTexture RGResourceRepo::DeclareResource(const RGTextureDesc& desc)
	{
		Texture_Storage storage{};
		storage.desc = desc;
			
		auto hdl = m_handleAtor.Allocate<RGTexture>();
		HandleAllocator::TryInsert(m_textures, storage, HandleAllocator::GetSlot(hdl.handle));
		
		return hdl;
	}

	RGBuffer RGResourceRepo::DeclareResource(const RGBufferDesc& desc)
	{
		Buffer_Storage storage{};
		storage.desc = desc;

		auto hdl = m_handleAtor.Allocate<RGBuffer>();
		HandleAllocator::TryInsert(m_buffers, storage, HandleAllocator::GetSlot(hdl.handle));

		return hdl;
	}

	RGTexture RGResourceRepo::AliasResource(RGTexture tex)
	{
		// Get existing (will fail if none is found)
		const auto& existing = HandleAllocator::TryGet(m_textures, HandleAllocator::GetSlot(tex.handle));

		Texture_Storage storage{};
		storage.desc = existing.desc;
		storage.aliasOf = tex;

		auto hdl = m_handleAtor.Allocate<RGTexture>();
		HandleAllocator::TryInsert(m_textures, storage, HandleAllocator::GetSlot(hdl.handle));

		return hdl;
	}

	RGBuffer RGResourceRepo::AliasResource(RGBuffer buf)
	{
		// Get existing (will fail if none is found)
		const auto& existing = HandleAllocator::TryGet(m_buffers, HandleAllocator::GetSlot(buf.handle));

		Buffer_Storage storage{};
		storage.desc = existing.desc;
		storage.aliasOf = buf;

		auto hdl = m_handleAtor.Allocate<RGBuffer>();
		HandleAllocator::TryInsert(m_buffers, storage, HandleAllocator::GetSlot(hdl.handle));

		return hdl;
	}

	
	std::vector<RGTexture> RGResourceRepo::DeclareAliasedResources(const std::vector<RGTextureDesc>&)
	{
		// @todo
		assert(false);
		return {};
	}
	Texture RGResourceRepo::GetTexture(RGTexture tex)
	{
		// @todo
		assert(false);

		return Texture();
	}
	Buffer RGResourceRepo::GetBuffer(RGBuffer buf)
	{
		return Buffer();
	}
}