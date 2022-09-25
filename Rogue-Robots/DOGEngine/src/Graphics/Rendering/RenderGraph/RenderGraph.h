#pragma once
#include "../../RHI/RenderResourceHandles.h"
#include "../../RHI/Types/ResourceDescs.h"
#include "../../RHI/Types/BarrierDesc.h"
#include "../../RHI/Types/RenderPassDesc.h"
#include "RGTypes.h"

#include <unordered_set>

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
			u64 GetView(RGResourceID id) const;			// SRV/UAVs

			// Graph global
			Texture GetTexture(RGResourceID id);
			Buffer GetBuffer(RGResourceID id);

		private:
			friend class RenderGraph;
			std::unordered_map<RGResourceID, u64> m_views;		// Views already converted to global indices

			// Used to retrieve underlying resources if needed by the user (e.g resource copies)
			RGResourceManager* m_resMan{ nullptr };
		};

	private:
		struct PassIO
		{
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
			
			// Filled by implementation
			std::string name;
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
		};

	public:
		struct PassBuilder
		{
		public:
			PassBuilder(PassBuilderGlobalData& globalData, RGResourceManager* resMan);

			// ResourceManager interface
			void DeclareTexture(RGResourceID id, RGTextureDesc desc);
			void ImportTexture(RGResourceID id, Texture texture, D3D12_RESOURCE_STATES entryState, D3D12_RESOURCE_STATES exitState);

			void ReadResource(RGResourceID id, D3D12_RESOURCE_STATES state, TextureViewDesc desc);
			void ReadOrWriteDepth(RGResourceID id, RenderPassAccessType access, TextureViewDesc desc);
			void WriteRenderTarget(RGResourceID id, RenderPassAccessType access, TextureViewDesc desc);
			void WriteAliasedRenderTarget(RGResourceID newID, RGResourceID oldID, RenderPassAccessType access, TextureViewDesc desc);

		private:
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
			PassData passData{};
			buildFunc(passData, builder);

			// Construct pass data
			auto pass = std::make_unique<Pass>(std::move(builder.m_pass));
			pass->name = name;
			pass->execFunc = [execFunc, passData](RenderDevice* rd, CommandList cmdl, PassResources& resources)
			{
				execFunc(passData, rd, cmdl, resources);
			};

			m_passes.push_back(std::move(pass));
		}

		void Build();
		void Execute();

	private:
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

		std::vector<std::unique_ptr<Pass>> m_passes;
		std::unordered_map<Pass*, std::vector<Pass*>> m_adjacencyMap;
		std::vector<Pass*> m_sortedPasses;

		u32 m_maxDepth{ 0 };
		std::vector<DependencyLevel> m_dependencyLevels;

		CommandList m_cmdl;
		


	};
}
