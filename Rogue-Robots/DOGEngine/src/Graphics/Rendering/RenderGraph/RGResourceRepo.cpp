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

	Texture RGResourceRepo::GetTexture(RGResource tex)
	{
		auto& res = HandleAllocator::TryGet(m_textures, HandleAllocator::GetSlot(tex.handle));
		if (!res.realized)
		{
			TextureDesc desc(MemoryType::Default, res.desc.format,
				res.desc.width, res.desc.height, res.desc.depth,
				res.desc.flags, res.desc.initState);

			res.realized = m_rd->CreateTexture(desc);
		}
		return *res.realized;
	}

}