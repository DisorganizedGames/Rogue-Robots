#pragma once
#include "RGTypes.h"
#include "../../RHI/RHITypes.h"
#include "../../Handles/HandleAllocator.h"

namespace DOG::gfx
{
	class RenderDevice;
	class RGResourceRepo;



	class RenderGraph
	{
		// ====== Views
		struct View_Storage
		{
			RGResourceType type{ RGResourceType::Texture };
			std::optional<u64> view;

			D3D12_RESOURCE_STATES desiredState{ D3D12_RESOURCE_STATE_COMMON };

			// View creation (deferred)
			std::function<u64(RenderDevice*, RGResourceRepo*, RenderPassBuilder*, D3D12_RESOURCE_STATES&)> createFunc;

			TextureView GetTextureView()
			{
				assert(type == RGResourceType::Texture);
				assert(view);
				return TextureView{ *view };
			}

			BufferView GetBufferView()
			{
				assert(type == RGResourceType::Buffer);
				assert(view);
				return BufferView{ *view };
			}
		};

		struct ViewRepo
		{
			HandleAllocator handleAtor;
			std::vector<std::optional<View_Storage>> views;

			ViewRepo()
			{
				views.resize(1);
			}

		};


		
		// Passes
	public:
		class PassBuilder
		{
		public:
			PassBuilder() = default;
			PassBuilder(ViewRepo* repo);

			RGResourceView ReadTexture(RGResource res, D3D12_RESOURCE_STATES state, const TextureViewDesc& desc);
			RGResourceView WriteTexture(RGResource res, D3D12_RESOURCE_STATES state, const TextureViewDesc& desc);

			friend class RenderGraph;
		private:
			ViewRepo* m_repo{ nullptr };

			std::vector<RGResource> m_reads;
			std::vector<RGResource> m_writes;

			std::vector<RGResourceView> m_readViews;
			std::vector<RGResourceView> m_writeViews;

			std::vector<D3D12_RESOURCE_STATES> m_readStates;
			std::vector<D3D12_RESOURCE_STATES> m_writeStates;
		};

		class PassResources
		{
		public:
			PassResources() = default;
			PassResources(RenderDevice* rd, RGResourceRepo* repo, ViewRepo* views);

			TextureView GetTexture(RGResourceView view);

		private:
			RenderDevice* m_rd{ nullptr };
			RGResourceRepo* m_repo{ nullptr };
			ViewRepo* m_views{ nullptr };

		};

	private:
		struct Pass
		{
			std::string name;
			PassBuilder builder;
			std::function<void(RenderDevice*, CommandList, PassResources&)> execFunc;

			u32 passDepth{ 0 };
			RenderPass rp;
		};

	public:
		RenderGraph(RenderDevice* rd, RGResourceRepo* repo);
		
		template <typename PassData>
		void AddPass(const std::string& name,
			const std::function<void(PassBuilder&, PassData&)>& buildFunc,
			const std::function<void(RenderDevice*, CommandList, PassResources&, const PassData&)>& execFunc)
		{
			PassData passData{};

			// Build
			PassBuilder builder(&m_views);
			buildFunc(builder, passData);
			
			// Construct pass data
			auto pass = std::make_unique<Pass>();
			pass->name = name;
			pass->builder = std::move(builder);
			pass->execFunc = [execFunc, passData](RenderDevice* rd, CommandList cmdl, PassResources& resources)
			{
				execFunc(rd, cmdl, resources, passData);
			};
			
			m_passes.push_back(std::move(pass));
		}

		void Build();
		void Run();

	private:
		void BuildAdjacencyList();
		void SortPassesTopologically();
		void AssignDepthLevels();

		void RealizeViews();

	private:
		RenderDevice* m_rd{ nullptr };
		RGResourceRepo* m_resources{ nullptr };
		PassResources m_passResources;		// Realizes resources
		ViewRepo m_views;

		std::vector<std::unique_ptr<Pass>> m_passes;
		std::unordered_map<Pass*, std::vector<Pass*>> m_adjacencyMap;
		std::vector<Pass*> m_sortedPasses;

		CommandList m_cmdl;
	};


}