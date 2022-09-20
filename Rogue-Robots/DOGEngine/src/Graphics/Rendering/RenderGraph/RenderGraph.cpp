#include "RenderGraph.h"
#include "RGResourceRepo.h"
#include "../../RHI/RenderDevice.h"

namespace DOG::gfx
{
	RenderGraph::RenderGraph(RenderDevice* rd, RGResourceRepo* repo) :
		m_rd(rd),
		m_resources(repo),
		m_passResources(m_rd, m_resources, &m_views)
	{
	}

	void RenderGraph::Build()
	{
		BuildAdjacencyList();
		SortPassesTopologically();
		BuildDependencyLevels();
	}

	void RenderGraph::Run()
	{
		for (auto& dependencyLevel : m_dependencyLevels)
			dependencyLevel.ExecutePasses(m_rd, m_passResources);
	}

	void RenderGraph::BuildAdjacencyList()
	{
		if (m_passes.size() == 1)
			m_sortedPasses.push_back(m_passes[0].get());

		for (auto& pass : m_passes)
		{
			// pdp: Potentially dependent pass
			for (auto& pdp : m_passes)
			{
				// Skip self
				if (pass == pdp)
					continue;

				// Check read/write intersection
				const auto& reads = pdp->builder.m_reads;
				for (const auto& write : pass->builder.m_writes)
				{
					if (std::find_if(reads.begin(), reads.end(), 
						[write](RGResource lh) { return lh.handle == write.handle; }) != reads.cend())
					{
						// Found at least one dependency -> Passes dependent
						auto& adjacents = m_adjacencyMap[pass.get()];
						adjacents.push_back(pdp.get());
						break;
					}
				}
			}
		}

	}

	void RenderGraph::SortPassesTopologically()
	{
		// Helper for DFS
		std::unordered_map<RenderPass*, bool> visited;

		// DFS lambda (requires fully specified to call lambda recursively, and need to pass itself to the lambda
		std::function<void(RenderPass*, const std::vector<RenderPass*>&)> dfs =
			[this, &visited, &dfs](RenderPass* vertex, const std::vector<RenderPass*>& edges)
		{
			if (visited[vertex])
				return;
			visited[vertex] = true;

			for (const auto& edge : edges)
			{
				auto next_edges = m_adjacencyMap[edge];
				dfs(edge, next_edges);
			}

			m_sortedPasses.push_back(vertex);
		};

		// Topological sorting --> Call Post-order DFS on each node and reverse
		for (auto& [pass, edges] : m_adjacencyMap)
		{
			dfs(pass, edges);
		}
		std::reverse(m_sortedPasses.begin(), m_sortedPasses.end());
	}

	void RenderGraph::BuildDependencyLevels()
	{
		// Assign dependency levels by traversing in topological order
		// It is important to observe that traversing in topological order means that
		// parent nodes always have resolved depth levels!
		u32 maxDepth{ 0 };
		for (const auto& pass : m_sortedPasses)
		{
			for (auto& adjacentPass : m_adjacencyMap[pass])
			{
				adjacentPass->passDepth = (std::max)(pass->passDepth + 1, adjacentPass->passDepth);
				maxDepth = (std::max)(adjacentPass->passDepth, maxDepth);
			}
		}

		// Add passes to dependency level
		m_dependencyLevels.resize(maxDepth + 1);
		for (u32 i = 0; i < m_sortedPasses.size(); ++i)
		{
			const auto& pass = m_sortedPasses[i];
			m_dependencyLevels[pass->passDepth].AddPass(pass);
		}
	}






	RenderGraph::PassBuilder::PassBuilder(ViewRepo* repo) :
		m_repo(repo)
	{
	}

	RGResourceView RenderGraph::PassBuilder::ReadTexture(RGResource res, const TextureViewDesc& desc)
	{
		m_reads.push_back(res);

		View_Storage storage{};
		storage.createFunc = [res, desc](RenderDevice* rd, RGResourceRepo* repo) -> u64
		{
			return rd->CreateView(repo->GetTexture(res), desc).handle;
		};
		auto ret = m_repo->handleAtor.Allocate<RGResourceView>();
		HandleAllocator::TryInsert(m_repo->views, storage, HandleAllocator::GetSlot(ret.handle));

		return ret;
	}

	RGResourceView RenderGraph::PassBuilder::WriteTexture(RGResource res, const TextureViewDesc& desc)
	{
		m_writes.push_back(res);

		View_Storage storage{};
		storage.createFunc = [res, desc](RenderDevice* rd, RGResourceRepo* repo) -> u64
		{
			return rd->CreateView(repo->GetTexture(res), desc).handle;
		};
		auto ret = m_repo->handleAtor.Allocate<RGResourceView>();
		HandleAllocator::TryInsert(m_repo->views, storage, HandleAllocator::GetSlot(ret.handle));

		return ret;
	}

	RenderGraph::PassResources::PassResources(RenderDevice* rd, RGResourceRepo* repo, ViewRepo* views) :
		m_rd(rd),
		m_repo(repo),
		m_views(views)
	{
	}

	TextureView RenderGraph::PassResources::RealizeTexture(RGResourceView view)
	{
		auto& viewResource = HandleAllocator::TryGet(m_views->views, HandleAllocator::GetSlot(view.handle));

		// Realize view
		viewResource.view = viewResource.createFunc(m_rd, m_repo);

		return viewResource.GetTextureView();
	}
	void RenderGraph::DependencyLevel::AddPass(RenderPass* pass)
	{
		m_passes.push_back(pass);
	}
	void RenderGraph::DependencyLevel::ExecutePasses(RenderDevice* rd, PassResources& resources)
	{
		for (const auto& pass : m_passes)
			pass->execFunc(rd, resources);
	}
}