#include "RenderGraph.h"
#include "RGResourceRepo.h"
#include "../../RHI/RenderDevice.h"

namespace DOG::gfx
{
	bool IsReadState(D3D12_RESOURCE_STATES states);

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
		AssignDepthLevels();
		
		//m_resources->FinalizeResourceLifetimes();
		m_resources->RealizeResources();
		RealizeViews();
	}

	void RenderGraph::Run()
	{
		m_cmdl = m_rd->AllocateCommandList();
		
		std::vector<GPUBarrier> barriers;
		for (const auto& pass : m_sortedPasses)
		{
			// JIT barriers
			for (u32 readIdx = 0; readIdx < pass->builder.m_reads.size(); ++readIdx)
			{
				const auto& read = pass->builder.m_reads[readIdx];
				auto prevState = m_resources->GetState(read);
				auto afterState = pass->builder.m_readStates[readIdx];
				auto tex = m_resources->GetTexture(read);
				m_resources->SetState(read, afterState);
	
				if (prevState != afterState)
					barriers.push_back(GPUBarrier::Transition(tex, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, prevState, afterState));
			}
			for (u32 writeIdx = 0; writeIdx < pass->builder.m_writes.size(); ++writeIdx)
			{
				const auto& write = pass->builder.m_writes[writeIdx];
				auto prevState = m_resources->GetState(write);
				auto afterState = pass->builder.m_writeStates[writeIdx];
				auto tex = m_resources->GetTexture(write);
				m_resources->SetState(write, afterState);

				if (prevState != afterState)
					barriers.push_back(GPUBarrier::Transition(tex, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, prevState, afterState));
			}
			if (!barriers.empty())
				m_rd->Cmd_Barrier(m_cmdl, barriers);

			// Exec pass
			m_rd->Cmd_BeginRenderPass(m_cmdl, pass->rp);
			pass->execFunc(m_rd, m_cmdl, m_passResources);
			m_rd->Cmd_EndRenderPass(m_cmdl);

			barriers.clear();
		}

		m_resources->TransitionImportedState(m_cmdl);

		m_rd->SubmitCommandList(m_cmdl);
		m_rd->Flush();


		for (const auto& storage : m_views.views)
		{
			if (storage)
			{
				m_rd->FreeView(TextureView(*storage->view));
			}
		}

		m_rd->RecycleCommandList(m_cmdl);
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

				const auto& writes = pdp->builder.m_writes;
				// Check write/write intersection
				for (const auto& write : pass->builder.m_writes)
				{
					if (std::find_if(writes.begin(), writes.end(),
						[write](RGResource lh) { return lh.handle == write.handle; }) != writes.cend())
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
		std::unordered_map<Pass*, bool> visited;

		// DFS lambda (requires fully specified to call lambda recursively, and need to pass itself to the lambda
		std::function<void(Pass*, const std::vector<Pass*>&)> dfs =
			[this, &visited, &dfs](Pass* vertex, const std::vector<Pass*>& edges)
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

	void RenderGraph::AssignDepthLevels()
	{
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
	}

	void RenderGraph::RealizeViews()
	{
		// { RG, states }
		std::unordered_map<u64, D3D12_RESOURCE_STATES> readStatesAccum;

		// Realize views in backwards order to accumulate read-states
		for (auto it = m_sortedPasses.rbegin(); it < m_sortedPasses.rend(); ++it)
		{
			const auto& pass = *it;

			// Realize write views (and construct render passes)
			RenderPassBuilder rpBuilder;
			// @todo Expose UAV flags in rp builder later
			
			for (u32 i = 0; i < pass->builder.m_writes.size(); ++i)
			{
				const auto& writeView = pass->builder.m_writeViews[i];
				const auto& write = pass->builder.m_writes[i];

				auto& viewRes = HandleAllocator::TryGet(m_views.views, HandleAllocator::GetSlot(writeView.handle));
				auto& states = readStatesAccum[write.handle];

				// Get view and desired state
				viewRes.view = viewRes.createFunc(m_rd, m_resources, &rpBuilder, viewRes.desiredState);
				
				// Reset accum
				states = D3D12_RESOURCE_STATE_COMMON;					

			}
			pass->rp = m_rd->CreateRenderPass(rpBuilder.Build());

			// Realize read views
			for (u32 i = 0; i < pass->builder.m_reads.size(); ++i)
			{
				const auto& readView = pass->builder.m_readViews[i];
				const auto& read = pass->builder.m_reads[i];

				auto& viewRes = HandleAllocator::TryGet(m_views.views, HandleAllocator::GetSlot(readView.handle));
				auto& states = readStatesAccum[read.handle];

				// Get view and desired state
				viewRes.view = viewRes.createFunc(m_rd, m_resources, nullptr, viewRes.desiredState);

				// Accumulate combined-read states
				states |= viewRes.desiredState;
				viewRes.desiredState = states;	
			}
		}
	}






	RenderGraph::PassBuilder::PassBuilder(ViewRepo* repo) :
		m_repo(repo)
	{
	}

	RGResourceView RenderGraph::PassBuilder::ReadTexture(RGResource res, D3D12_RESOURCE_STATES state, const TextureViewDesc& desc)
	{
		m_reads.push_back(res);

		View_Storage storage{};
		storage.createFunc = [res, desc, state](RenderDevice* rd, RGResourceRepo* repo, RenderPassBuilder*, D3D12_RESOURCE_STATES& outStates) -> u64
		{
			outStates = state;
			return rd->CreateView(repo->GetTexture(res), desc).handle;
		};
		auto ret = m_repo->handleAtor.Allocate<RGResourceView>();
		HandleAllocator::TryInsert(m_repo->views, storage, HandleAllocator::GetSlot(ret.handle));

		m_readViews.push_back(ret);
		m_readStates.push_back(state);

		return ret;
	}

	RGResourceView RenderGraph::PassBuilder::WriteTexture(RGResource res, D3D12_RESOURCE_STATES state, const TextureViewDesc& desc)
	{
		m_writes.push_back(res);

		View_Storage storage{};
		storage.createFunc = [res, desc, state](RenderDevice* rd, RGResourceRepo* repo, RenderPassBuilder* builder, D3D12_RESOURCE_STATES& outStates) -> u64
		{
			assert(builder != nullptr);

			auto view = rd->CreateView(repo->GetTexture(res), desc);
			
			if (desc.format == DXGI_FORMAT_D16_UNORM || desc.format == DXGI_FORMAT_D32_FLOAT)
			{
				builder->AddDepth(view, RenderPassBeginAccessType::Clear, RenderPassEndingAccessType::Discard);
			}
			else if (desc.format == DXGI_FORMAT_D24_UNORM_S8_UINT || desc.format == DXGI_FORMAT_D32_FLOAT_S8X24_UINT)
			{
				builder->AddDepthStencil(view, 
					RenderPassBeginAccessType::Clear, RenderPassEndingAccessType::Discard,	// depth
					RenderPassBeginAccessType::Clear, RenderPassEndingAccessType::Discard);	// stencil
			}
			else
			{
				builder->AppendRT(view, RenderPassBeginAccessType::Clear, RenderPassEndingAccessType::Preserve);
			}

			outStates = state;

			return view.handle;
		};
		auto ret = m_repo->handleAtor.Allocate<RGResourceView>();
		HandleAllocator::TryInsert(m_repo->views, storage, HandleAllocator::GetSlot(ret.handle));
		
		m_writeViews.push_back(ret);
		m_writeStates.push_back(state);

		return ret;
	}

	RenderGraph::PassResources::PassResources(RenderDevice* rd, RGResourceRepo* repo, ViewRepo* views) :
		m_rd(rd),
		m_repo(repo),
		m_views(views)
	{
	}

	TextureView RenderGraph::PassResources::GetTexture(RGResourceView view)
	{
		auto& viewResource = HandleAllocator::TryGet(m_views->views, HandleAllocator::GetSlot(view.handle));
		return viewResource.GetTextureView();
	}







	bool IsReadState(D3D12_RESOURCE_STATES states)
	{
		if ((states & D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER) == D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER ||
			(states & D3D12_RESOURCE_STATE_INDEX_BUFFER) == D3D12_RESOURCE_STATE_INDEX_BUFFER ||
			(states & D3D12_RESOURCE_STATE_DEPTH_READ) == D3D12_RESOURCE_STATE_DEPTH_READ ||
			(states & D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE) == D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE ||
			(states & D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE) == D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE ||
			(states & D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT) == D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT ||
			(states & D3D12_RESOURCE_STATE_COPY_SOURCE) == D3D12_RESOURCE_STATE_COPY_SOURCE)
		{
			return true;
		}
		return false;
	}
}