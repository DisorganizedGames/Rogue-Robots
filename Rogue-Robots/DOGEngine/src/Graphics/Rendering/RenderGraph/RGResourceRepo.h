#pragma once
#include "RGTypes.h"
#include "../../Handles/HandleAllocator.h"
#include "../../RHI/RenderResourceHandles.h"

namespace DOG::gfx
{


	class RenderDevice;
	class GPUGarbageBin;
	
	class RGResourceRepo
	{
	private:
		static constexpr u8 MAX_UNUSED_RESOURCE_LIFETIME{ 1 };	// Everything is recreated every frame
	public:
		RGResourceRepo(RenderDevice* rd, GPUGarbageBin* bin);
	
		// Called per frame
		void Tick();

		RGResource DeclareResource(const RGTextureDesc& desc);
		RGResource AliasResource(RGResource tex);

		RGResource ImportResource(Texture tex, D3D12_RESOURCE_STATES initState);


	private:
		friend class RenderGraph;

		Texture GetTexture(RGResource tex);
		D3D12_RESOURCE_STATES GetInitState(RGResource tex);
		bool IsImported(RGResource tex);
		std::pair<u32, u32>& GetMutEffectiveLifetime(RGResource tex);

		void RealizeResources();

	private:
		struct Texture_Storage
		{
			RGTextureDesc desc;
			std::optional<Texture> realized;
			std::pair<u32, u32> effectiveLifetime{ 0, 0 };
			bool imported{ false };

			// Points to the original resource
			bool isAnAlias{ false };
			std::optional<RGResource> aliasOf;
		};

	private:
		RenderDevice* m_rd{ nullptr };
		GPUGarbageBin* m_bin{ nullptr };
		u64 m_currFrame{ 0 };

		HandleAllocator m_handleAtor;
		std::vector<std::optional<Texture_Storage>> m_textures;




	};
}