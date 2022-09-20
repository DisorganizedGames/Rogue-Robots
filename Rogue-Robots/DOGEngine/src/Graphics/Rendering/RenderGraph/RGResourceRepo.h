#pragma once
#include "RGTypes.h"
#include "../../Handles/HandleAllocator.h"
#include "../../RHI/RenderResourceHandles.h"

namespace DOG::gfx
{
	class RenderDevice;
	class GPUGarbageBin;
	
	/*
		Responsible for render graph resource allocation, automatic deallocation, and total lifetime
	*/
	class RGResourceRepo
	{
	private:
		static constexpr u8 MAX_UNUSED_RESOURCE_LIFETIME{ 3 };
	public:
		RGResourceRepo(RenderDevice* rd, GPUGarbageBin* bin);
	
		// Called per frame
		void Tick();

		RGTexture DeclareResource(const RGTextureDesc& desc);
		RGBuffer DeclareResource(const RGBufferDesc& desc);
		
		RGTexture AliasResource(RGTexture tex);
		RGBuffer AliasResource(RGBuffer buf);


		// Returns a group of resources which are memory aliased.
		// These resources cannot be used simultaneously and require proper aliasing barriers
		std::vector<RGTexture> DeclareAliasedResources(const std::vector<RGTextureDesc>& descs);

		// Realizes resource on use
		// If aliased --> Looks up alias group and initializes all other aliased resource too
		Texture GetTexture(RGTexture tex);
		
		Buffer GetBuffer(RGBuffer buf);

	private:
		struct Texture_Storage
		{
			RGTextureDesc desc;
			std::optional<Texture> realized;
			bool memoryAliased{ false };

			// If aliased, keeps track of who
			std::optional<RGTexture> aliasOf;

			u64 lastFrameAccess{ 0 };
		};

		struct Buffer_Storage
		{
			RGBufferDesc desc;
			std::optional<Buffer> realized;

			// If aliased, keeps track of who
			std::optional<RGBuffer> aliasOf;

			u64 lastFrameAccess{ 0 };
		};

		struct AliasGroup_Storage
		{
			std::vector<RGTextureDesc> descs;	// Find max of descs
			std::vector<RGTexture> textures;	// other textures associated with this alias group
		};

	private:
		RenderDevice* m_rd{ nullptr };
		GPUGarbageBin* m_bin{ nullptr };
		u64 m_currFrame{ 0 };

		HandleAllocator m_handleAtor;
		std::vector<std::optional<Buffer_Storage>> m_buffers;
		std::vector<std::optional<Texture_Storage>> m_textures;
		std::vector<std::optional<AliasGroup_Storage>> m_aliasGroups;




	};
}