#pragma once
#include "../../RHI/RenderResourceHandles.h"
#include "../../RHI/Types/ResourceDescs.h"
#include "../../RHI/Types/BarrierDesc.h"
#include "../../RHI/Types/RenderPassDesc.h"
#include "RGTypes.h"
#include <unordered_set>

//#define GENERATE_GRAPHVIZ

namespace DOG::gfx
{
	class RenderDevice;
	class RGResourceManager;
	class GPUGarbageBin;

	class RenderGraph
	{
	public:
		/*
			Specialized interface to return usable GPU elements in the execution logic in each pass.
			Views that are retrieved here local to the declared resources! 
		*/
		class PassResources
		{
		public:
			PassResources() = default;

			// Pass local
			u32 GetView(RGResourceID id) const;			// SRV/UAVs

			// Graph global
			Texture GetTexture(RGResourceID id);
			Buffer GetBuffer(RGResourceID id);

		private:
			friend class RenderGraph;
			std::unordered_map<RGResourceID, u32> m_views;			// Views already converted to global indices for immediate use
			std::unordered_map<RGResourceID, Buffer> m_buffers;		// Underlying buffer resources
			std::unordered_map<RGResourceID, Texture> m_textures;	// Underlying texture resources

			// Held for cleanup
			std::vector<TextureView> m_textureViews;
			std::vector<BufferView> m_bufferViews;
		};

	private:
		struct PassIO
		{
			std::optional<RGResourceID> originalID;			// if aliased --> holds original resource name
			RGResourceID id;
			RGResourceType type{ RGResourceType::Texture };
			std::optional<std::variant<TextureViewDesc, BufferViewDesc>> viewDesc;
			D3D12_RESOURCE_STATES desiredState{ D3D12_RESOURCE_STATE_COMMON };
			std::optional<RenderPassAccessType> rpAccessType;			// Render Target & Depth
			std::optional<RenderPassAccessType> rpStencilAccessType;	// Stencil
			bool aliasWrite{ false };
		};

		struct Pass
		{
			// Declared by user
			std::vector<PassIO> inputs;
			std::vector<PassIO> outputs;

			std::vector<RGResourceID> proxyInput;
			std::vector<RGResourceID> proxyOutput;
			
			// Filled by implementation
			std::string name;
			u32 id{ 0 };
			std::function<void(RenderDevice*, CommandList, PassResources&)> execFunc;
			u32 depth{ 0 };
			PassResources passResources;

			std::optional<RenderPass> rp;
		};

		class DependencyLevel
		{
		public:
			DependencyLevel(RGResourceManager* resMan);

			void Execute(RenderDevice* rd, CommandList cmdl);
	
			void AddPass(Pass* pass);
			void AddEntryBarrier(GPUBarrier barrier);
			bool BarrierExists(u64 resource, D3D12_RESOURCE_BARRIER_TYPE type);
			/*
				Called after all barriers have been inserted.
				This checks for any simultaneous read/write on the same resource (forbidden)
				and combines multiple reads on the same resource if possible.
			*/
			void Finalize();

		private:
			RGResourceManager* m_resMan{ nullptr };

			std::vector<Pass*> m_passes;

			// Barriers for all resources upon entry into this dependency level
			std::vector<GPUBarrier> m_batchedEntryBarriers;
		};

		struct PassBuilderGlobalData
		{
			std::unordered_set<RGResourceID> writes;

			// ID holder for auto-aliasing (Backbuffer --> Backbuffer(0) --> Backbuffer(1), etc.)
			std::unordered_map<RGResourceID, u32> writeCount;	

			// Keeps track of the pass IDs which read from this resource up until another read-write
			// E.g --> [Read, Read, Alias, Read, Read, Read, Alias]
			// Vector sized 2 until first Alias where it gets reset to 0
			// Vector sized 3 until second Alias where it gets reset to 0
			std::unordered_map<RGResourceID, std::vector<u32>> latestRead;

			// When alias is reset, a proxy is created as it marks the last read-to-write
			std::vector<std::pair<u32, u32>> proxies;
		};

	public:
		struct PassBuilder
		{
		public:
			PassBuilder(PassBuilderGlobalData& globalData, RGResourceManager* resMan);

			// ResourceManager interface
			void DeclareTexture(RGResourceID id, RGTextureDesc desc);
			void ImportTexture(RGResourceID id, Texture texture, D3D12_RESOURCE_STATES entryState, D3D12_RESOURCE_STATES exitState);
			void DeclareBuffer(RGResourceID id, RGBufferDesc desc);
			void ImportBuffer(RGResourceID id, Buffer buffer, D3D12_RESOURCE_STATES entryState, D3D12_RESOURCE_STATES exitState);

			// Texture reads
			void ReadResource(RGResourceID id, D3D12_RESOURCE_STATES state, TextureViewDesc desc);
			void ReadDepthStencil(RGResourceID id, TextureViewDesc desc);

			// Buffer reads
			void ReadResource(RGResourceID id, D3D12_RESOURCE_STATES state, BufferViewDesc desc);

			// Texture writes
			void WriteDepthStencil(RGResourceID id, RenderPassAccessType depthAccess, TextureViewDesc desc, RenderPassAccessType stencilAccess = RenderPassAccessType::DiscardDiscard);
			void WriteRenderTarget(RGResourceID id, RenderPassAccessType access, TextureViewDesc desc);
			void ReadWriteTarget(RGResourceID id, TextureViewDesc desc);
			
			// Buffer writes
			void ReadWriteTarget(RGResourceID id, BufferViewDesc desc);
			void CopyToBuffer(RGResourceID id);



		private:
			// Auto-proxy and auto-alias helpers
			std::pair<RGResourceID, RGResourceID> ResolveAliasingIDs(RGResourceID input);
			void PushPassReader(RGResourceID id);
			RGResourceID GetPrevious(RGResourceID id);
			RGResourceID GetNextID(RGResourceID id);
			void FlushReadsAndConnectProxy(RGResourceID id);

		

			// Proxy now handled internally
			void ProxyWrite(RGResourceID id);
			void ProxyRead(RGResourceID id);

			friend class RenderGraph;
			PassBuilderGlobalData& m_globalData;
			RGResourceManager* m_resMan{ nullptr };
			Pass m_pass;
		};

	public:
		RenderGraph(RenderDevice* rd, RGResourceManager* resMan, GPUGarbageBin* bin);

		template <typename PassData>
		void AddPass(const std::string& name,
			const std::function<void(PassData&, PassBuilder&)>& buildFunc,
			const std::function<void(const PassData&, RenderDevice*, CommandList, PassResources&)>& execFunc)
		{
			PassBuilder builder(m_passBuilderGlobalData, m_resMan);
			builder.m_pass.id = m_nextPassID++;
			builder.m_pass.name = name;

			PassData passData{};
			buildFunc(passData, builder);

			// Construct pass data
			auto pass = std::make_unique<Pass>(std::move(builder.m_pass));
			pass->execFunc = [execFunc, passData](RenderDevice* rd, CommandList cmdl, PassResources& resources)
			{
				execFunc(passData, rd, cmdl, resources);
			};

			m_passes.push_back(std::move(pass));
		}

		void Build();
		void Execute();

	private:
		void AddProxies();
		void BuildAdjacencyMap();
		void SortPassesTopologically();
		void AssignDependencyLevels();
		void BuildDependencyLevels();

		void TrackLifetimes();
		void TrackTransitions();

		void RealizeViews();

		void GenerateGraphviz();

	private:
		RenderDevice* m_rd{ nullptr };
		RGResourceManager* m_resMan{ nullptr };
		GPUGarbageBin* m_bin{ nullptr };
		PassBuilderGlobalData m_passBuilderGlobalData;

		u32 m_nextPassID{ 0 };
		std::vector<std::unique_ptr<Pass>> m_passes;
		std::unordered_map<Pass*, std::vector<Pass*>> m_adjacencyMap;
		std::vector<Pass*> m_sortedPasses;

		u32 m_maxDepth{ 0 };
		std::vector<DependencyLevel> m_dependencyLevels;

		CommandList m_cmdl;
		


	};
}
