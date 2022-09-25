#include "RenderGraph.h"
#include "RGResourceManager.h"
#include "../../RHI/RenderDevice.h"
#include <set>
#include <fstream>

namespace DOG::gfx
{
	bool IsReadState(D3D12_RESOURCE_STATES states);

	RenderGraph::RenderGraph(RenderDevice* rd, RGResourceManager* resMan) :
		m_rd(rd),
		m_resMan(resMan)
	{
	}

	void RenderGraph::Build()
	{
		BuildAdjacencyMap();
		SortPassesTopologically();

		AssignDependencyLevels();
		BuildDependencyLevels();

		// Walk graph topologically to obtain all data necessary for sanitizing
		TrackLifetimes();
		m_resMan->SanitizeAliasingLifetimes();

		m_resMan->RealizeResources();

		// Resources NEED to be realized from this point forward!

		RealizeViews();
		TrackTransitions();

		for (auto& depLevel : m_dependencyLevels)
			depLevel.Finalize();

		// To help the graph author see what's going on.
		// @TODO: Expand with input/output labels on each pass
		GenerateGraphviz();
	}

	void RenderGraph::Execute()
	{
	}

	void RenderGraph::BuildAdjacencyMap()
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

				// Check input/output intersection
				const auto& reads = pdp->inputs;
				for (const auto& output : pass->outputs)
				{
					auto it = std::find_if(reads.begin(), reads.end(), [output](PassIO input) { return input.id == output.id; });
					bool ioIntersects = it != reads.cend();

					if (ioIntersects)
					{
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

	void RenderGraph::AssignDependencyLevels()
	{
		// Assign dependency levels by traversing in topological order
		// It is important to observe that traversing in topological order means that
		// parent nodes always have resolved depth levels!
		for (const auto& pass : m_sortedPasses)
		{
			for (auto& adjacentPass : m_adjacencyMap[pass])
			{
				adjacentPass->depth = (std::max)(pass->depth + 1, adjacentPass->depth);
				m_maxDepth = (std::max)(adjacentPass->depth, m_maxDepth);
			}
		}
	}

	void RenderGraph::BuildDependencyLevels()
	{
		// Add passes to dependency level
		for (u32 i = 0; i < m_maxDepth + 1; ++i)
			m_dependencyLevels.push_back(DependencyLevel(m_resMan));

		for (u32 i = 0; i < m_sortedPasses.size(); ++i)
		{
			const auto& pass = m_sortedPasses[i];
			m_dependencyLevels[pass->depth].AddPass(pass);
		}
	}

	void RenderGraph::TrackLifetimes()
	{
		for (const auto& pass : m_sortedPasses)
		{
			auto& depLevel = m_dependencyLevels[pass->depth];
			for (const auto& input : pass->inputs)
			{
				// Track start lifetime
				auto& resourceLifetime = m_resMan->GetMutableResourceLifetime(input.id);
				resourceLifetime.first = (std::min)(pass->depth, resourceLifetime.first);
				resourceLifetime.second = (std::max)(pass->depth, resourceLifetime.second);

				// Track usage lifetime
				auto& usageLifetime = m_resMan->GetMutableUsageLifetime(input.id);
				usageLifetime.first = (std::min)(pass->depth, usageLifetime.first);
				usageLifetime.second = (std::max)(pass->depth, usageLifetime.second);

			}

			for (const auto& output : pass->outputs)
			{
				// Track end lifetime
				auto& resourceLifetime = m_resMan->GetMutableResourceLifetime(output.id);
				resourceLifetime.first = (std::min)(pass->depth, resourceLifetime.first);
				resourceLifetime.second = (std::max)(pass->depth, resourceLifetime.second);

				// Track usage lifetime
				auto& usageLifetime = m_resMan->GetMutableUsageLifetime(output.id);
				usageLifetime.first = (std::min)(pass->depth, usageLifetime.first);
				usageLifetime.second = (std::max)(pass->depth, usageLifetime.second);
			}

		}
	}

	void RenderGraph::TrackTransitions()
	{
		const auto trackStateTransition = [this](
			u64 resource, RGResourceType type, DependencyLevel& depLevel,
			D3D12_RESOURCE_STATES currState, D3D12_RESOURCE_STATES desiredState)
		{
			GPUBarrier barr{};
			if (type == RGResourceType::Texture)
			{
				barr = GPUBarrier::Transition(
					Texture(resource),
					0,
					currState, desiredState);
			}
			else
			{
				barr = GPUBarrier::Transition(
					Texture(resource),
					0,
					currState, desiredState);
			}
			if (currState != desiredState)
			{
				depLevel.AddEntryBarrier(barr);
				return true;
			}
			else
			{
				return false;
			}
		};

		for (const auto& pass : m_sortedPasses)
		{
			auto& depLevel = m_dependencyLevels[pass->depth];
			for (const auto& input : pass->inputs)
			{
				// Track resource state transitions
				const auto resource = m_resMan->GetResource(input.id);
				const auto currState = m_resMan->GetCurrentState(input.id);
				const auto desiredState = input.desiredState;

				// If aliased, output takes care of picking up state transition
				if (!input.aliasWrite &&
					trackStateTransition(resource, input.type, depLevel, currState, desiredState))
				{
					m_resMan->SetCurrentState(input.id, desiredState);
				}
					

			}

			for (const auto& output : pass->outputs)
			{
				// Track resource state transitions
				const auto resource = m_resMan->GetResource(output.id);
				const auto currState = m_resMan->GetCurrentState(output.id);
				const auto desiredState = output.desiredState;

				if (trackStateTransition(resource, output.type, depLevel, currState, desiredState))
					m_resMan->SetCurrentState(output.id, desiredState);
			}

		}
	}

	void RenderGraph::RealizeViews()
	{
		for (const auto& pass : m_sortedPasses)
		{
			auto& passResources = pass->passResources;
			passResources.m_resMan = m_resMan;

			// Realize output views
			RenderPassBuilder builder;
			for (const auto& output : pass->outputs)
			{
				if (output.type == RGResourceType::Texture)
				{
					const auto& viewDesc = std::get<TextureViewDesc>(*output.viewDesc);

					// Create view and immediately convert to global descriptor index
					auto view = m_rd->CreateView(Texture(m_resMan->GetResource(output.id)), viewDesc);

					if (viewDesc.viewType == ViewType::RenderTarget)
					{
						auto accesses = GetAccessTypes(*output.rpAccessType);
						builder.AppendRT(view, accesses.first, accesses.second);
					}
					else if (viewDesc.viewType == ViewType::DepthStencil)
					{
						auto depthAccesses = GetAccessTypes(*output.rpAccessType);
						auto stencilAccesses = GetAccessTypes(*output.rpStencilAccessType);
						builder.AddDepthStencil(view,
							depthAccesses.first, depthAccesses.second,
							stencilAccesses.first, stencilAccesses.second);
					}
					else
					{
						// We do not expose RTV and DSV to user (these are baked into the render pass)
						passResources.m_views[output.id] = m_rd->GetGlobalDescriptor(view);
					}
				}
				else
				{
					const auto& viewDesc = std::get<BufferViewDesc>(*output.viewDesc);

					// Create view and immediately convert to global descriptor index
					passResources.m_views[output.id] = m_rd->GetGlobalDescriptor(m_rd->CreateView(Buffer(m_resMan->GetResource(output.id)), viewDesc));
				}
			}
		}
	}

	void RenderGraph::GenerateGraphviz()
	{
		/*
			Open .txt
			Use adjacency map to grab
		*/
			
		std::ofstream file;
		file.open("rendergraph.txt");
		file << "digraph G {" << std::endl;

		for (const auto& [pass, adjacents] : m_adjacencyMap)
		{
			for (const auto& adj : adjacents)
			{
				file << "\"" << pass->name << "\" -> \"" << adj->name << "\"" << std::endl;
			}
		}

		file << "}" << std::endl;

		file.close();
	}





	RenderGraph::PassBuilder::PassBuilder(PassBuilderGlobalData& globalData, RGResourceManager* resMan) :
		m_globalData(globalData),
		m_resMan(resMan)
	{
	}

	void RenderGraph::PassBuilder::DeclareTexture(RGResourceID name, RGTextureDesc desc)
	{
		m_resMan->DeclareTexture(name, desc);
	}

	void RenderGraph::PassBuilder::ImportTexture(RGResourceID name, Texture texture, D3D12_RESOURCE_STATES entryState, D3D12_RESOURCE_STATES exitState)
	{
		m_resMan->ImportTexture(name, texture, entryState, exitState);
	}

	void RenderGraph::PassBuilder::WriteRenderTarget(RGResourceID id, RenderPassAccessType access, TextureViewDesc desc)
	{
		// Explicitly forbids writing to the same graph resource more than once
		assert(!m_globalData.writes.contains(id));
		m_globalData.writes.insert(id);
		
		assert(desc.viewType == ViewType::RenderTarget);
		
		PassIO output;
		output.id = id;
		output.type = RGResourceType::Texture;
		output.viewDesc = desc;
		output.desiredState = D3D12_RESOURCE_STATE_RENDER_TARGET;
		output.rpAccessType = access;
		m_pass.outputs.push_back(output);
	}

	void RenderGraph::PassBuilder::WriteAliasedRenderTarget(RGResourceID newID, RGResourceID oldID, RenderPassAccessType access, TextureViewDesc desc)
	{
		assert(desc.viewType == ViewType::RenderTarget);

		assert(!m_globalData.writes.contains(newID));
		m_globalData.writes.insert(newID);

		m_resMan->AliasTexture(newID, oldID);



		PassIO input;
		input.id = oldID;
		input.type = RGResourceType::Texture;
		input.aliasWrite = true;
		m_pass.inputs.push_back(input);

		PassIO output;
		output.id = newID;
		output.type = RGResourceType::Texture;
		output.desiredState = D3D12_RESOURCE_STATE_RENDER_TARGET;
		output.viewDesc = desc;
		output.aliasWrite = true;
		output.rpAccessType = access;
		m_pass.outputs.push_back(output);
	}





	RenderGraph::DependencyLevel::DependencyLevel(RGResourceManager* resMan) :
		m_resMan(resMan)
	{
	}

	void RenderGraph::DependencyLevel::AddPass(Pass* pass)
	{
		m_passes.push_back(pass);
	}

	void RenderGraph::DependencyLevel::AddEntryBarrier(GPUBarrier barrier)
	{
		m_batchedEntryBarriers.push_back(barrier);
	}

	void RenderGraph::DependencyLevel::Finalize()
	{
		// Assuming initial state upon element initialization in the hash map is D3D12_RESOURCE_STATE_COMMON (since value is 0)
		std::unordered_map<u64, D3D12_RESOURCE_STATES> resourceDesiredStates;
		for (const auto& pass : m_passes)
		{
			for (const auto& input : pass->inputs)
			{
				auto resource = m_resMan->GetResource(input.id);
				auto& states = resourceDesiredStates[resource];

				// Alias write is a special case --> Underlying resource WILL be the same
				// This is the only simultaneous read-write we will allow (this will get picked up when iterating over outputs)
				if (input.aliasWrite)
					continue;
				
				// If tracked state is read combination --> Forbid writes
				if (IsReadState(states))
					assert(IsReadState(input.desiredState));
				// If write state --> Exclusive writer pass already exists and all other accesses are forbidden
				else
					assert(false);

				// Read-combine if all is well
				resourceDesiredStates[resource] |= input.desiredState;
			}

			for (const auto& output : pass->outputs)
			{
				auto resource = m_resMan->GetResource(output.id);
				auto& states = resourceDesiredStates[resource];

				// We assert that state has to be COMMON, meaning it is uninitialized (no writers and no readers yet)
				assert(states == D3D12_RESOURCE_STATE_COMMON);

				// Sanity check that the graph author always puts a write state for outputs
				assert(!IsReadState(output.desiredState));

				// Set exclusive write state
				resourceDesiredStates[resource] = output.desiredState;
			}
		}

		// Apply combined read-state (if any) or exclusive write state
		for (auto& barrier : m_batchedEntryBarriers)
		{
			if (barrier.type == D3D12_RESOURCE_BARRIER_TYPE_TRANSITION)
			{
				barrier.stateAfter = resourceDesiredStates[barrier.resource];
			}
		}

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