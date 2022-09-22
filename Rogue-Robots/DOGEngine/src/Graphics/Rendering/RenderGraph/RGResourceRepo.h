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

		// Import external (state must resource initial state)
		RGResource ImportResource(Texture tex, D3D12_RESOURCE_STATES initState);

		Texture GetTexture(RGResource tex);

	private:
		friend class RenderGraph;
		void RealizeResources();
		void SetState(RGResource resource, D3D12_RESOURCE_STATES states);
		D3D12_RESOURCE_STATES GetState(RGResource resource);

	private:
		struct Texture_Storage
		{
			RGTextureDesc desc;
			std::optional<Texture> realized;
			D3D12_RESOURCE_STATES currState{ D3D12_RESOURCE_STATE_COMMON };
		};

	private:
		RenderDevice* m_rd{ nullptr };
		GPUGarbageBin* m_bin{ nullptr };
		u64 m_currFrame{ 0 };

		HandleAllocator m_handleAtor;
		std::vector<std::optional<Texture_Storage>> m_textures;




	};
}