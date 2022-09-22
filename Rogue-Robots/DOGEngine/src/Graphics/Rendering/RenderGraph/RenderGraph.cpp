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
		BuildDependencyLevels();

		//m_resources->FinalizeResourceLifetimes();
		m_resources->RealizeResources();
		RealizeViews();

		InsertTransitions();
	}

	void RenderGraph::Run()
	{
		m_cmdl = m_rd->AllocateCommandList();
		for (auto& dependencyLevel : m_dependencyLevels)
			dependencyLevel.ExecutePasses(m_rd, m_passResources, m_resources, m_cmdl);

		m_rd->SubmitCommandList(m_cmdl);
		m_rd->Flush();

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

	void RenderGraph::InsertTransitions()
	{
		for (const auto& [pass, adjPasses] : m_adjacencyMap)
		{
			const auto dependencyLevel = pass->passDepth;

			for (u32 writeIdx = 0; writeIdx < pass->builder.m_writes.size(); ++writeIdx)
			{
				// Write properties
				const auto& write = pass->builder.m_writes[writeIdx];
				const auto& writeRes = m_resources->GetTexture(write);					// Will break if we do buffers
				const auto& writeView = pass->builder.m_writeViews[writeIdx];
				D3D12_RESOURCE_STATES prevWriteState{ pass->builder.m_writeStates[writeIdx] };

				
				auto& effectiveLifetime = m_resources->GetMutEffectiveLifetime(write);	// Will break if we do buffers
				effectiveLifetime.first = dependencyLevel;
				effectiveLifetime.second = dependencyLevel;
				// We should have some bool that unsubscribes external resources from effective liftime calcs and misc.

				// Read properties
				D3D12_RESOURCE_STATES combinedReadState{ D3D12_RESOURCE_STATE_COMMON };
				bool intersects{ false };
				for (const auto& adjPass : adjPasses)
				{
					const auto adjPassDepth = adjPass->passDepth;

					const auto& reads = adjPass->builder.m_reads;
					const auto it = std::find_if(reads.cbegin(), reads.cend(), [&](RGResource res)
						{
							return write.handle == res.handle;
						});
					intersects = it != reads.cend();
					if (intersects)
					{
						const auto idx = it - reads.cbegin();
						// Grab view to get desired state
						const auto& readViews = adjPass->builder.m_readViews;
						const auto readView = readViews[idx];
						const auto& viewRes = HandleAllocator::TryGet(m_views.views, HandleAllocator::GetSlot(readView.handle));
						combinedReadState |= viewRes.desiredState;

						effectiveLifetime.second = (std::max)(adjPassDepth, effectiveLifetime.second);
					}
				}

				auto& depLevel = m_dependencyLevels[dependencyLevel];

				// If combined desired state is common, we skip.
				if (intersects)
					depLevel.AddStateTransition(writeRes, prevWriteState, combinedReadState);
			}
		}
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





	void RenderGraph::DependencyLevel::AddPass(Pass* pass)
	{
		m_passes.push_back(pass);
	}

	void RenderGraph::DependencyLevel::ExecutePasses(RenderDevice* rd, PassResources& resources, RGResourceRepo* repo, CommandList cmdl)
	{
		ExecuteBarriers(rd, cmdl);

		for (auto& pass : m_passes)
		{	
			rd->Cmd_BeginRenderPass(cmdl, pass->rp);
			pass->execFunc(rd, cmdl, resources);
			rd->Cmd_EndRenderPass(cmdl);
		}
	}

	void RenderGraph::DependencyLevel::AddStateTransition(Buffer buffer, D3D12_RESOURCE_STATES prev, D3D12_RESOURCE_STATES after)
	{
		m_barriers.push_back(GPUBarrier::Transition(buffer, 0, prev, after));
	}

	void RenderGraph::DependencyLevel::AddStateTransition(Texture tex, D3D12_RESOURCE_STATES prev, D3D12_RESOURCE_STATES after)
	{
		m_barriers.push_back(GPUBarrier::Transition(tex, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, prev, after));
	}

	void RenderGraph::DependencyLevel::ExecuteBarriers(RenderDevice* rd, CommandList cmdl)
	{
		rd->Cmd_Barrier(cmdl, m_barriers);
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