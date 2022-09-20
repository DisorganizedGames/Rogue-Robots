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

			// View creation (deferred)
			std::function<u64(RenderDevice*, RGResourceRepo*)> createFunc;

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



	public:
		class PassBuilder
		{
		public:
			PassBuilder() = default;
			PassBuilder(ViewRepo* repo);

			// No texture desc assumes default D3D12 view settings
			RGResourceView ReadTexture(RGResource res, std::optional<TextureViewDesc> desc = {});
			RGResourceView WriteTexture(RGResource res, std::optional<TextureViewDesc> desc = {});

			friend class RenderGraph;
		private:
			ViewRepo* m_repo{ nullptr };

			// Resolve dependencies
			std::vector<RGResource> m_reads;
			std::vector<RGResource> m_writes;

		};

		class PassResources
		{
		public:
			PassResources() = default;
			PassResources(RenderDevice* rd, RGResourceRepo* repo, ViewRepo* views);

			TextureView RealizeTexture(RGResourceView view);

		private:
			RenderDevice* m_rd{ nullptr };
			RGResourceRepo* m_repo{ nullptr };
			ViewRepo* m_views{ nullptr };

		};

		struct RenderPass
		{
			std::string name;
			PassBuilder builder;
			std::function<void(RenderDevice*, PassResources&)> execFunc;

			u32 passDepth{ 0 };
		};

	private:
		class DependencyLevel
		{
		public:
			void AddPass(RenderPass* pass);
			void ExecutePasses(RenderDevice* rd, PassResources& resources);

		private:
			std::vector<RenderPass*> m_passes;
		};

	public:
		RenderGraph(RenderDevice* rd, RGResourceRepo* repo);
		
		template <typename PassData>
		void AddPass(const std::string& name,
			const std::function<void(PassBuilder&, PassData&)>& buildFunc,
			const std::function<void(RenderDevice*, PassResources&, const PassData&)>& execFunc)
		{
			PassData passData{};

			// Build
			PassBuilder builder(&m_views);
			buildFunc(builder, passData);
			
			// Construct pass data
			auto pass = std::make_unique<RenderPass>();
			pass->name = name;
			pass->builder = std::move(builder);
			pass->execFunc = [execFunc, passData](RenderDevice* rd, PassResources& resources)
			{
				execFunc(rd, resources, passData);
			};
			
			m_passes.push_back(std::move(pass));
		}

		void Build();
		void Run();

	private:
		void BuildAdjacencyList();
		void SortPassesTopologically();
		void BuildDependencyLevels();

	private:
		RenderDevice* m_rd{ nullptr };
		RGResourceRepo* m_resources{ nullptr };
		PassResources m_passResources;		// Realizes resources
		ViewRepo m_views;

		std::vector<std::unique_ptr<RenderPass>> m_passes;
		std::unordered_map<RenderPass*, std::vector<RenderPass*>> m_adjacencyMap;
		std::vector<RenderPass*> m_sortedPasses;
		std::vector<DependencyLevel> m_dependencyLevels;



	};


}