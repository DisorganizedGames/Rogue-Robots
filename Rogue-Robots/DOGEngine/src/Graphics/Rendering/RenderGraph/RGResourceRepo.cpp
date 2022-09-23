#include "RGResourceRepo.h"
#include "../../RHI/RenderDevice.h"
#include "../../Rendering/GPUGarbageBin.h"

namespace DOG::gfx
{
	RGResourceRepo::RGResourceRepo(RenderDevice* rd, GPUGarbageBin* bin) :
		m_rd(rd),
		m_bin(bin)
	{
		m_textures.resize(1);
	}

	void RGResourceRepo::Tick()
	{
		// Condition for resource deletion
		auto shouldDelete = [this](u64 lastFrameAccess) -> bool
		{
			return m_currFrame > (lastFrameAccess + MAX_UNUSED_RESOURCE_LIFETIME);
		};

		++m_currFrame;
	}

	RGResource RGResourceRepo::DeclareResource(const RGTextureDesc& desc)
	{
		Texture_Storage storage{};
		storage.desc = desc;
			
		auto hdl = m_handleAtor.Allocate<RGResource>();
		HandleAllocator::TryInsert(m_textures, storage, HandleAllocator::GetSlot(hdl.handle));
		
		return hdl;
	}

	RGResource RGResourceRepo::AliasResource(RGResource tex)
	{
		auto& res = HandleAllocator::TryGet(m_textures, HandleAllocator::GetSlot(tex.handle));
		assert(!res.hasBeenAliased);		// enforce that a RGResource can only be aliased once!
		res.hasBeenAliased = true;

		Texture_Storage storage{};
		storage.isAnAlias = true;
		storage.aliasOf = tex;

		auto hdl = m_handleAtor.Allocate<RGResource>();
		HandleAllocator::TryInsert(m_textures, storage, HandleAllocator::GetSlot(hdl.handle));

		return hdl;
	}

	RGResource RGResourceRepo::ImportResource(Texture tex, D3D12_RESOURCE_STATES initState)
	{
		Texture_Storage storage{};
		storage.realized = tex;
		storage.imported = true;
		storage.desc.initState = initState;

		auto hdl = m_handleAtor.Allocate<RGResource>();
		HandleAllocator::TryInsert(m_textures, storage, HandleAllocator::GetSlot(hdl.handle));

		return hdl;
	}

	Texture RGResourceRepo::GetTexture(RGResource tex)
	{
		const auto& res = HandleAllocator::TryGet(m_textures, HandleAllocator::GetSlot(tex.handle));
		assert(res.realized);
		return *res.realized;
	}

	D3D12_RESOURCE_STATES RGResourceRepo::GetInitState(RGResource tex)
	{
		const auto& res = HandleAllocator::TryGet(m_textures, HandleAllocator::GetSlot(tex.handle));
		return res.desc.initState;
	}

	D3D12_RESOURCE_STATES RGResourceRepo::GetAliasInitState(RGResource tex)
	{
		const auto& res = HandleAllocator::TryGet(m_textures, HandleAllocator::GetSlot(tex.handle));
		const auto& originalRes = HandleAllocator::TryGet(m_textures, HandleAllocator::GetSlot((res.aliasOf)->handle));
		return originalRes.aliasInitState;
	}


	bool RGResourceRepo::IsImported(RGResource tex)
	{
		const auto& res = HandleAllocator::TryGet(m_textures, HandleAllocator::GetSlot(tex.handle));
		assert(res.realized);
		return res.imported;
	}

	bool RGResourceRepo::IsAnAlias(RGResource tex)
	{
		const auto& res = HandleAllocator::TryGet(m_textures, HandleAllocator::GetSlot(tex.handle));
		return res.isAnAlias;
	}


	std::pair<u32, u32>& RGResourceRepo::GetMutEffectiveLifetime(RGResource tex)
	{
		auto& res = HandleAllocator::TryGet(m_textures, HandleAllocator::GetSlot(tex.handle));
		return res.effectiveLifetime;
	}

	void RGResourceRepo::SetAliasInitState(RGResource tex, D3D12_RESOURCE_STATES nextState)
	{
		auto& res = HandleAllocator::TryGet(m_textures, HandleAllocator::GetSlot(tex.handle));
		res.aliasInitState = nextState;
	}

	void RGResourceRepo::RealizeResources()
	{
		for (auto& res : m_textures)
		{
			if (res && !res->realized && !res->isAnAlias)
			{
				TextureDesc desc(MemoryType::Default, res->desc.format,
					res->desc.width, res->desc.height, res->desc.depth,
					res->desc.flags, res->desc.initState);

				res->realized = m_rd->CreateTexture(desc);
			}
		}

		// Second pass to assign aliases the original resources
		for (auto& res : m_textures)
		{
			if (res && !res->realized && res->isAnAlias)
			{
				const auto original = (*res->aliasOf).handle;
				const auto& originalRes = HandleAllocator::TryGet(m_textures, HandleAllocator::GetSlot(original));

				// Assign the RGResource with the same underlying resource
				res->realized = originalRes.realized;
			}
		}
	}

}