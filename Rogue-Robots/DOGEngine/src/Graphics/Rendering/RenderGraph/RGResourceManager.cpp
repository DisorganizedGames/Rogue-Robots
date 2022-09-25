#include "RGResourceManager.h"
#include "../../RHI/RenderDevice.h"
#include "../GPUGarbageBin.h"

namespace DOG::gfx
{
	RGResourceManager::RGResourceManager(RenderDevice* rd, GPUGarbageBin* bin) :
		m_rd(rd),
		m_bin(bin)
	{
	}
	void RGResourceManager::DeclareTexture(RGResourceID id, RGTextureDesc desc)
	{
		assert(!m_resources.contains(id));

		RGResourceDeclared decl;
		decl.desc = desc;
		decl.currState = desc.initState;

		auto& res = m_resources[id];
		res.variantType = RGResourceVariant::Declared;
		res.resourceType = RGResourceType::Texture;
		res.variants = decl;
	}

	void RGResourceManager::ImportTexture(RGResourceID id, Texture texture, D3D12_RESOURCE_STATES entryState, D3D12_RESOURCE_STATES exitState)
	{
		assert(!m_resources.contains(id));

		auto& res = m_resources[id];

		RGResourceImported imported;
		imported.importEntryState = entryState;
		imported.currState = entryState;
		imported.importExitState = exitState;

		res.resource = texture.handle;
		res.resourceType = RGResourceType::Texture;
		res.variantType = RGResourceVariant::Imported;
		res.variants = imported;
	}

	void RGResourceManager::AliasTexture(RGResourceID newID, RGResourceID oldID)
	{
		assert(!m_resources.contains(newID));
		assert(m_resources.contains(oldID));

		auto& oldRes = m_resources[oldID];
		assert(oldRes.hasBeenAliased == false);		// Enforce single-alias rule
		oldRes.hasBeenAliased = true;

		RGResourceAliased alias;
		alias.prevID = oldID;

		auto& newRes = m_resources[newID];
		newRes.variantType = RGResourceVariant::Aliased;
		newRes.resourceType = RGResourceType::Texture;
		newRes.variants = alias;
	}

	void RGResourceManager::RealizeResources()
	{
		// https://github.com/DisorganizedGames/Rogue-Robots/blob/RenderGraph/Rogue-Robots/DOGEngine/src/Graphics/Rendering/RenderGraph/RGResourceRepo.cpp
		// Create textures

		for (auto& [_, resource] : m_resources)
		{
			// If not a Declared resource, we are not interested in creating a resource for it
			if (resource.variantType != RGResourceVariant::Declared)
				continue;

			const RGResourceDeclared& decl = std::get<RGResourceDeclared>(resource.variants);


			if (resource.resourceType == RGResourceType::Texture)
			{
				//resource.resource = m_rd->CreateTexture(decl.desc).handle
			}
			else
			{

			}
		}

		// initialize aliasing resource too?
		// we need to give the RGResource 'resource' a value for aliased things...
		// @TODO
	}

	void RGResourceManager::SanitizeAliasingLifetimes()
	{
		for (auto& [_, resource] : m_resources)
		{
			// If not a Declared resource, we are not interested in creating a resource for it
			if (resource.variantType != RGResourceVariant::Aliased)
				continue;

			const RGResourceAliased& decl = std::get<RGResourceAliased>(resource.variants);

			// Parent usage lifetime must be < aliased usage lifetime since parent is consumed when the aliased lifetime starts.
			const auto& res = m_resources.find(decl.prevID)->second;
			const auto& parentLifetime = res.usageLifetime;
			const auto& thisLifetime = resource.usageLifetime;

			const auto parentEnd = parentLifetime.second;
			const auto aliasBegin = thisLifetime.first;
			assert(parentEnd <= aliasBegin);

			// Additionally, only read accesses should be allowed in the range (parentBegin, aliasBegin) (exclusive)
			/*
				e.g: After a resource has been aliased, that resource should not be able to be written to. (Forbid targets and other writes)
			*/
		}
	}

	u64 RGResourceManager::GetResource(RGResourceID id) const
	{
		return m_resources.find(id)->second.resource;
	}

	RGResourceVariant RGResourceManager::GetResourceVariant(RGResourceID id) const
	{
		return m_resources.find(id)->second.variantType;
	}

	D3D12_RESOURCE_STATES RGResourceManager::GetCurrentState(RGResourceID id) const
	{
		const auto& res = m_resources.find(id)->second;

		// Get all the way back to the Declared/Imported resource
		const RGResourceManager::RGResource* resource{ &res };
		while (resource->variantType == RGResourceVariant::Aliased)
			resource = &m_resources.find(std::get<RGResourceAliased>(resource->variants).prevID)->second;

		if (resource->variantType == RGResourceVariant::Declared)
			return std::get<RGResourceDeclared>(resource->variants).currState;
		else 
			return std::get<RGResourceImported>(resource->variants).currState;
	}

	std::pair<u32, u32>& RGResourceManager::GetMutableUsageLifetime(RGResourceID id)
	{
		return m_resources.find(id)->second.usageLifetime;
	}

	std::pair<u32, u32>& RGResourceManager::GetMutableResourceLifetime(RGResourceID id)
	{
		auto& res = m_resources.find(id)->second;

		// Get all the way back to the Declared/Imported resource
		RGResourceManager::RGResource* resource{ &res };
		while (resource->variantType == RGResourceVariant::Aliased)
			resource = &m_resources.find(std::get<RGResourceAliased>(resource->variants).prevID)->second;

		if (resource->variantType == RGResourceVariant::Declared)
			return std::get<RGResourceDeclared>(resource->variants).resourceLifetime;
		else
			return std::get<RGResourceImported>(resource->variants).resourceLifetime;

	}

	void RGResourceManager::SetCurrentState(RGResourceID id, D3D12_RESOURCE_STATES state)
	{
		auto& res = m_resources.find(id)->second;

		// Get all the way back to the Declared/Imported resource
		RGResourceManager::RGResource* resource{ &res };
		while (resource->variantType == RGResourceVariant::Aliased)
			resource = &m_resources.find(std::get<RGResourceAliased>(resource->variants).prevID)->second;

		if (resource->variantType == RGResourceVariant::Declared)
			std::get<RGResourceDeclared>(resource->variants).currState = state;
		else
			std::get<RGResourceImported>(resource->variants).currState = state;
	}
}