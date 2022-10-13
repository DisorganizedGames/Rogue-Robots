#pragma once
#include "RGTypes.h"


namespace DOG::gfx
{
	class RenderDevice;
	class GPUGarbageBin;


	class RGResourceManager
	{
		friend class RenderGraph;
	public:
		RGResourceManager(RenderDevice* rd, GPUGarbageBin* bin);

		/*
			Discards the resources stored safely and clears map for re-use.
			Assuming that the render graph is rebuilt every frame
		*/
		void Tick();

	private:
		friend class RenderGraph;
		
		// These interfaces are exposed through PassBuilder
		void DeclareTexture(RGResourceID id, RGTextureDesc desc);
		void ImportTexture(RGResourceID id, Texture texture, D3D12_RESOURCE_STATES entryState, D3D12_RESOURCE_STATES exitState, u32 numMips = 1, u32 arraySize = 1);

		void DeclareBuffer(RGResourceID id, RGBufferDesc desc);
		void ImportBuffer(RGResourceID id, Buffer buffer, D3D12_RESOURCE_STATES entryState, D3D12_RESOURCE_STATES exitState);

		void AliasResource(RGResourceID newID, RGResourceID oldID, RGResourceType type);
		void DeclareProxy(RGResourceID id);
	private:


		struct RGResourceDeclared
		{
			std::variant<RGTextureDesc, RGBufferDesc> desc;
			std::pair<u32, u32> resourceLifetime{ std::numeric_limits<u32>::max(), std::numeric_limits<u32>::min() };		// Lifetime of the underlying resource
			D3D12_RESOURCE_STATES currState{ D3D12_RESOURCE_STATE_COMMON };
		
			std::unordered_set<u32> subresources;
			std::vector<D3D12_RESOURCE_STATES> currStates;
		};

		struct RGResourceImported
		{
			D3D12_RESOURCE_STATES importEntryState{ D3D12_RESOURCE_STATE_COMMON };
			D3D12_RESOURCE_STATES importExitState{ D3D12_RESOURCE_STATE_COMMON };
			D3D12_RESOURCE_STATES currState{ D3D12_RESOURCE_STATE_COMMON };
			
			// Infinite lifetime from graphs perspective
			std::pair<u32, u32> resourceLifetime{ std::numeric_limits<u32>::max(), std::numeric_limits<u32>::min() };

			std::unordered_set<u32> subresources;
			std::vector<D3D12_RESOURCE_STATES> entryStates;
			std::vector<D3D12_RESOURCE_STATES> exitStates;
			std::vector<D3D12_RESOURCE_STATES> currStates;
		};

		struct RGResourceAliased
		{
			RGResourceID prevID;
			RGResourceID originalID;		// Original Declared/Imported resource
		};

		struct RGResource
		{
			RGResourceType resourceType{ RGResourceType::Texture };
			RGResourceVariant variantType{ RGResourceVariant::Declared };
			std::variant<RGResourceDeclared, RGResourceImported, RGResourceAliased> variants;

			std::pair<u32, u32> usageLifetime{ std::numeric_limits<u32>::max(), std::numeric_limits<u32>::min() };		// Lifetime of this specific RGResource

			u64 resource{ 0 };	// texture/buffer
			bool hasBeenAliased{ false };

		};

		// ======================

	private:
		// RealizeResources be called after resource lifetimes have been resolved
		void RealizeResources();
		void SanitizeAliasingLifetimes();
		void ImportedResourceExitTransition(CommandList cmdl);

		u64 GetResource(RGResourceID id) const;
		RGResourceType GetResourceType(RGResourceID id) const;
		RGResourceVariant GetResourceVariant(RGResourceID id) const;
		D3D12_RESOURCE_STATES GetCurrentState(RGResourceID id) const;
		D3D12_RESOURCE_STATES GetCurrentState(RGResourceID id, u32 subresource) const;
		std::pair<u32, u32>& GetMutableUsageLifetime(RGResourceID id);		// Lifetime of specific graph resource

		// ========= Declared & Alias specific
		// Lifetime of underlying resource
		std::pair<u32, u32>& GetMutableResourceLifetime(RGResourceID id);
		const std::pair<u32, u32>& GetResourceLifetime(RGResourceID id);

		// Helper for JIT state transitions
		void SetCurrentState(RGResourceID id, D3D12_RESOURCE_STATES state);

		// Subresource JIT state helpers
		void SetCurrentState(RGResourceID id, u32 subresource, D3D12_RESOURCE_STATES state);

		const std::unordered_set<u32>& GetSubresources(RGResourceID id);

	private:
		RenderDevice* m_rd{ nullptr };
		GPUGarbageBin* m_bin{ nullptr };
		std::unordered_map<RGResourceID, RGResource> m_resources;
	};
}