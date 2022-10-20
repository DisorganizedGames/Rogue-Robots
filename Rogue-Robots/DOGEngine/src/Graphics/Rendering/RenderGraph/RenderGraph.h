#pragma once
#include "../../RHI/RenderResourceHandles.h"
#include "../../RHI/Types/ResourceDescs.h"
#include "../../RHI/Types/BarrierDesc.h"
#include "../../RHI/Types/RenderPassDesc.h"
#include "../../Memory/BumpAllocator.h"
#include "RGTypes.h"
#include <unordered_set>

//#define GENERATE_GRAPHVIZ

namespace DOG::gfx
{
	class RenderDevice;
	class RGResourceManager;
	class GPUGarbageBin;
	class BumpAllocator;

	class RenderGraph
	{
	public:
		// Specialized interface to return usable GPU primitives in the execution logic in each pass.
		// Only pass local resources are obtainable
		class PassResources
		{
		public:
			PassResources() = default;

			u32 GetView(RGResourceView id) const;			// SRV/UAVs

			TextureView GetTextureView(RGResourceView id) const;
			BufferView GetBufferView(RGResourceView id) const;

			Texture GetTexture(RGResourceID id);
			Buffer GetBuffer(RGResourceID id);

		private:
			friend class RenderGraph;
			std::unordered_map<RGResourceView, u32> m_views;						// Views already converted to global indices for immediate use
			std::unordered_map<RGResourceID, Buffer> m_buffers;						// Underlying buffer resources
			std::unordered_map<RGResourceID, Texture> m_textures;					// Underlying texture resources
			std::unordered_map<RGResourceView, TextureView> m_textureViewsLookup;	// Underlying texture view
			std::unordered_map<RGResourceView, BufferView> m_bufferViewsLookup;		// Underlying buffer view

			// Stores all views associated with this PassResources, held for cleanup
			std::vector<TextureView> m_textureViews;
			std::vector<BufferView> m_bufferViews;
		};

	private:
		struct PassIO
		{
			// I/O data
			std::optional<RGResourceID> originalID;	// If aliased, holds original resource
			RGResourceID id;
			RGResourceType type{ RGResourceType::Texture };
			RGResourceView viewID;
			std::optional<std::variant<TextureViewDesc, BufferViewDesc>> viewDesc;
			D3D12_RESOURCE_STATES desiredState{ D3D12_RESOURCE_STATE_COMMON };

			// Output specific data
			std::optional<RenderPassAccessType> rpAccessType;			// Render Target & Depth
			std::optional<RenderPassAccessType> rpStencilAccessType;	// Stencil
			bool aliasWrite{ false };
		};

		struct Pass
		{
			Pass(const std::string& name, u32 id) :
				name(name),
				id(id)
			{}

			// Declared by user
			std::vector<PassIO> inputs;
			std::vector<PassIO> outputs;

			std::vector<RGResourceID> proxyInput;
			std::vector<RGResourceID> proxyOutput;
			
			// Filled by implementation
			std::string name;
			u32 id{ 0 };
			u32 depth{ 0 };
			std::optional<std::function<void()>> preGraphExecute, postGraphExecute;
			std::function<void(RenderDevice*, CommandList, PassResources&)> execFunc;
			PassResources passResources;
			
			// If pass writes to render target, render pass is sued
			std::optional<RenderPass> rp;
		};

		class DependencyLevel
		{
		public:
			DependencyLevel(RGResourceManager* resMan) : m_resMan(resMan) {}

			void Execute(RenderDevice* rd, CommandList cmdl);
	
			void AddPass(Pass* pass) { m_passes.push_back(pass); }
			void AddEntryBarrier(GPUBarrier barrier) { m_entryBarriers.push_back(barrier); }

			const std::vector<Pass*>& GetPasses() const { return m_passes; }

		private:
			RGResourceManager* m_resMan{ nullptr };
			std::vector<Pass*> m_passes;
			std::vector<GPUBarrier> m_entryBarriers;
		};

		struct PassBuilderGlobalData
		{
			u32 viewCount{ 0 };

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
			PassBuilder(PassBuilderGlobalData& globalData, RGResourceManager* resMan, Pass& pass) : 
				m_globalData(globalData), 
				m_resMan(resMan),
				m_pass(pass)
			{};

			// ResourceManager interface
			void DeclareTexture(RGResourceID id, RGTextureDesc desc);
			void ImportTexture(RGResourceID id, Texture texture, D3D12_RESOURCE_STATES entryState, D3D12_RESOURCE_STATES exitState);
			void DeclareBuffer(RGResourceID id, RGBufferDesc desc);
			void ImportBuffer(RGResourceID id, Buffer buffer, D3D12_RESOURCE_STATES entryState, D3D12_RESOURCE_STATES exitState);

			// Texture read views
			[[nodiscard]] RGResourceView ReadResource(RGResourceID id, D3D12_RESOURCE_STATES state, TextureViewDesc desc);
			[[nodiscard]] RGResourceView ReadResource(RGResourceID id, D3D12_RESOURCE_STATES state, BufferViewDesc desc);
			void ReadDepthStencil(RGResourceID id, TextureViewDesc desc);

			void WriteDepthStencil(RGResourceID id, RenderPassAccessType depthAccess, TextureViewDesc desc, RenderPassAccessType stencilAccess = RenderPassAccessType::DiscardDiscard);
			void WriteRenderTarget(RGResourceID id, RenderPassAccessType access, TextureViewDesc desc);
			[[nodiscard]] RGResourceView ReadWriteTarget(RGResourceID id, TextureViewDesc desc);
			[[nodiscard]] RGResourceView ReadWriteTarget(RGResourceID id, BufferViewDesc desc);

			void CopyToResource(RGResourceID id, RGResourceType type);
			void CopyFromResource(RGResourceID id, RGResourceType type);


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

			PassBuilderGlobalData& m_globalData;
			RGResourceManager* m_resMan{ nullptr };
			Pass& m_pass;
		};

	public:
		RenderGraph(RenderDevice* rd, RGResourceManager* resMan, GPUGarbageBin* bin);

		template <typename PassData>
		void AddPass(const std::string& name,
			const std::function<void(PassData&, PassBuilder&)>& buildFunc,										// Declare resources and populate persistent PassData
			const std::function<void(const PassData&, RenderDevice*, CommandList, PassResources&)>& execFunc,	// Render work (potentially asynchronously)
			std::optional<std::function<void(PassData&)>> preGraphExecuteFunc = {},								// Allocate any transient resources up-front
			std::optional<std::function<void(PassData&)>> postGraphExecuteFunc = {})							// Free any transient resources used)	
		{
			static_assert(std::is_trivially_copyable<PassData>::value && "PassData must be trivially copyable");

			Pass newPass(name, m_nextPassID++);
			PassBuilder builder(m_passBuilderGlobalData, m_resMan, newPass);
			
			u8* passDataMemory = m_passDataAllocator->Allocate(sizeof(PassData));
			buildFunc(*(PassData*)passDataMemory, builder);

			auto pass = std::make_unique<Pass>(std::move(newPass));
			pass->execFunc = [execFunc, memory = passDataMemory](RenderDevice* rd, CommandList cmdl, PassResources& resources)
			{
				execFunc(*(PassData*)memory, rd, cmdl, resources);
			};

			if (preGraphExecuteFunc)
			{
				pass->preGraphExecute = [preGraphExecuteFunc, memory = passDataMemory]()
				{
					(*preGraphExecuteFunc)(*(PassData*)memory);
				};
			}
			
			if (postGraphExecuteFunc)
			{
				pass->postGraphExecute = [postGraphExecuteFunc, memory = passDataMemory]()
				{
					(*postGraphExecuteFunc)(*(PassData*)memory);
				};
			}

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

		// Bump allocator which is reset after each graph execution
		std::unique_ptr<BumpAllocator> m_passDataAllocator;

		CommandList m_cmdl;
	};
}
