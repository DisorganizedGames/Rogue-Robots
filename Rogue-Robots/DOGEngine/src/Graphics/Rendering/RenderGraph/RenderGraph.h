#pragma once
#include "../../RHI/RenderResourceHandles.h"
#include "../../RHI/Types/ResourceDescs.h"
#include "../../RHI/Types/BarrierDesc.h"
#include "RGTypes.h"

namespace DOG::gfx
{
	class RenderDevice;
	class RGResourceManager;

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
			PassResources(RGResourceManager* resMan, std::unordered_map<RGResourceID, u64> views);

			// Pass local
			u64 GetView(RGResourceID id);			// SRV/UAVs

			// Graph global
			Texture GetTexture(RGResourceID id);
			Texture GetBuffer(RGResourceID id);

		private:
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
			std::vector<GPUBarrier> m_refinedBatchedEntryBarriers;
		};

	public:
		struct PassBuilder
		{
		public:
			void ReadResource(RGResourceID id, D3D12_RESOURCE_STATES state, TextureViewDesc desc);

			void ReadOrWriteDepth(RGResourceID id, TextureViewDesc desc);

			void WriteRenderTarget(RGResourceID id, TextureViewDesc desc);

			void WriteAliasedRenderTarget(RGResourceID newID, RGResourceID oldID, TextureViewDesc desc);

		private:
			friend class RenderGraph;
			Pass m_pass;
		};

	public:
		RenderGraph(RenderDevice* rd, RGResourceManager* resMan);

		template <typename PassData>
		void AddPass(const std::string& name,
			const std::function<void(PassData&, PassBuilder&)>& buildFunc,
			const std::function<void(const PassData&, RenderDevice*, CommandList, PassResources&)>& execFunc)
		{
			PassBuilder builder{};
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

	private:
		RenderDevice* m_rd{ nullptr };
		RGResourceManager* m_resMan{ nullptr };

		std::vector<std::unique_ptr<Pass>> m_passes;
		std::unordered_map<Pass*, std::vector<Pass*>> m_adjacencyMap;
		std::vector<Pass*> m_sortedPasses;

		u32 m_maxDepth{ 0 };
		std::vector<DependencyLevel> m_dependencyLevels;
		


	};
}
