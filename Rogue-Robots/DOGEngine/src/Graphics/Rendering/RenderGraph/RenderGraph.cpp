#include "RenderGraph.h"
#include "RGResourceManager.h"
#include "../../RHI/RenderDevice.h"
#include "../GPUGarbageBin.h"
#include <set>
#include <fstream>

namespace DOG::gfx
{
	RenderGraph::RenderGraph(RenderDevice* rd, RGResourceManager* resMan, GPUGarbageBin* bin) :
		m_rd(rd),
		m_resMan(resMan),
		m_bin(bin)
	{
		constexpr u32 PASS_RESERVED{ 50 };
		constexpr u32 DEP_LEVELS_RESERVED{ 50 };
		m_passes.reserve(PASS_RESERVED);
		m_sortedPasses.reserve(PASS_RESERVED);
		m_dependencyLevels.reserve(DEP_LEVELS_RESERVED);
	}

	void RenderGraph::Build()
	{
		BuildAdjacencyMap();

		// To help the graph author see what's going on.
		// @TODO: Expand with input/output labels on each pass
#ifdef GENERATE_GRAPHVIZ
		GenerateGraphviz();
#endif

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


	}

	void RenderGraph::Execute()
	{
		m_cmdl = m_rd->AllocateCommandList();

		for (auto& depLevel : m_dependencyLevels)
			depLevel.Execute(m_rd, m_cmdl);

		m_resMan->ImportedResourceExitTransition(m_cmdl);
		m_rd->SubmitCommandList(m_cmdl);

		// Clean up command list
		auto delFunc = [rd = m_rd, cmdl = m_cmdl]()
		{
			rd->RecycleCommandList(cmdl);
		};
		m_bin->PushDeferredDeletion(delFunc);

		// Clean up views
		for (const auto& pass : m_sortedPasses)
		{
			for (const auto& [id, view] : pass->passResources.m_views)
			{
				auto df = [rd = m_rd, resMan = m_resMan, id, view]()
				{
					if (resMan->GetResourceType(id) == RGResourceType::Texture)
						rd->FreeView(TextureView(view));
					else
						rd->FreeView(BufferView(view));
				};
				m_bin->PushDeferredDeletion(df);
			}

			for (const auto& view : pass->rpTextureViews)
			{
				auto df = [rd = m_rd, resMan = m_resMan, view]()
				{
					rd->FreeView(view);
				};
				m_bin->PushDeferredDeletion(df);
			}

			if (pass->rp)
			{
				auto df = [rd = m_rd, rp = pass->rp]()
				{
					rd->FreeRenderPass(*rp);
				};
				m_bin->PushDeferredDeletion(df);
			}
		}


	}

	void RenderGraph::BuildAdjacencyMap()
	{
		static constexpr u32 ADJACENTS_RESERVED{ 10 };

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
				const auto& inputs = pdp->inputs;
				for (const auto& output : pass->outputs)
				{
					auto it = std::find_if(inputs.begin(), inputs.end(), [output](PassIO input) { return input.id == output.id; });
					bool ioIntersects = it != inputs.cend();

					if (ioIntersects)
					{
						auto& adjacents = m_adjacencyMap[pass.get()];
						if (adjacents.empty())
							adjacents.reserve(ADJACENTS_RESERVED);
						adjacents.push_back(pdp.get());
						break;
					}
				}

				// Check input/output proxy intersection
				const auto& proxyInput = pdp->proxyInput;
				for (const auto& output : pass->proxyOutput)
				{
					auto it = std::find_if(proxyInput.begin(), proxyInput.end(), [output](RGResourceID input) { return input == output; });
					bool ioIntersects = it != proxyInput.cend();

					if (ioIntersects)
					{
						auto& adjacents = m_adjacencyMap[pass.get()];
						if (adjacents.empty())
							adjacents.reserve(ADJACENTS_RESERVED);
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
					D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
					currState, desiredState);
			}
			else
			{
				barr = GPUBarrier::Transition(
					Texture(resource),
					D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
					currState, desiredState);
			}
			if (currState != desiredState)
			{
				if (depLevel.BarrierExists(resource))
					return false;

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
		for (auto& pass : m_sortedPasses)
		{
			auto& passResources = pass->passResources;
			passResources.m_resMan = m_resMan;

			// Realize output views
			RenderPassBuilder builder;
			bool rpActive{ false };
			for (const auto& output : pass->outputs)
			{
				if (output.type == RGResourceType::Texture)
				{
					const auto& viewDesc = std::get<TextureViewDesc>(*output.viewDesc);

					// Create view and immediately convert to global descriptor index
					auto view = m_rd->CreateView(Texture(m_resMan->GetResource(output.id)), viewDesc);

					if (viewDesc.viewType == ViewType::RenderTarget)
					{
						rpActive = true;
						pass->rpTextureViews.push_back(view);

						auto accesses = GetAccessTypes(*output.rpAccessType);
						builder.AppendRT(view, accesses.first, accesses.second);
					}
					else if (viewDesc.viewType == ViewType::DepthStencil)
					{
						rpActive = true;
						pass->rpTextureViews.push_back(view);

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

			if (rpActive)
				pass->rp = m_rd->CreateRenderPass(builder.Build());
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

	void RenderGraph::PassBuilder::ReadResource(RGResourceID id, D3D12_RESOURCE_STATES state, TextureViewDesc desc)
	{
		assert(IsReadState(state));

		PassIO input;
		input.id = id;
		input.desiredState = state;
		input.viewDesc = desc;
		input.type = RGResourceType::Texture;
		m_pass.inputs.push_back(input);
	}

	void RenderGraph::PassBuilder::ReadOrWriteDepth(RGResourceID id, RenderPassAccessType access, TextureViewDesc desc)
	{
		// DSV read
		if (desc.depthReadOnly)
		{
			assert(false);		// @TODO
		}
		// DSV write
		else
		{
			PassIO output{};
			output.id = id;
			output.type = RGResourceType::Texture;
			output.viewDesc = desc;
			output.desiredState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
			output.rpAccessType = access;
			output.rpStencilAccessType = RenderPassAccessType::Discard_Discard;
			m_pass.outputs.push_back(output);
		}
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

	void RenderGraph::PassBuilder::ProxyWrite(RGResourceID id)
	{
		// Explicitly forbids outputting the same proxy more than once
		assert(!m_globalData.writes.contains(id));
		m_globalData.writes.insert(id);
		
		m_resMan->DeclareProxy(id);
		m_pass.proxyOutput.push_back(id);
	}

	void RenderGraph::PassBuilder::ProxyRead(RGResourceID id)
	{
		m_pass.proxyInput.push_back(id);
	}





	RenderGraph::DependencyLevel::DependencyLevel(RGResourceManager* resMan) :
		m_resMan(resMan)
	{
	}

	void RenderGraph::DependencyLevel::Execute(RenderDevice* rd, CommandList cmdl)
	{
		if (!m_batchedEntryBarriers.empty())
			rd->Cmd_Barrier(cmdl, m_batchedEntryBarriers);

		for (const auto& pass : m_passes)
		{
			//std::cout << "Doing: " << pass->name << "\n";

			if (pass->rp)
			{
				rd->Cmd_BeginRenderPass(cmdl, *pass->rp);
				pass->execFunc(rd, cmdl, pass->passResources);
				rd->Cmd_EndRenderPass(cmdl);
			}
			else
			{
				pass->execFunc(rd, cmdl, pass->passResources);
			}
		}
	}

	void RenderGraph::DependencyLevel::AddPass(Pass* pass)
	{
		m_passes.push_back(pass);
	}

	void RenderGraph::DependencyLevel::AddEntryBarrier(GPUBarrier barrier)
	{
		m_batchedEntryBarriers.push_back(barrier);
	}

	bool RenderGraph::DependencyLevel::BarrierExists(u64 resource)
	{
		for (const auto& barrier : m_batchedEntryBarriers)
			if (barrier.resource == resource)
				return true;
		return false;
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
				{
					if (states != D3D12_RESOURCE_STATE_COMMON)	// Common is currently is synonymous with uninitialized (for the resourceDesiredStates)
						assert(false);
				}

				// Read-combine if all is well
				resourceDesiredStates[resource] |= input.desiredState;
			}

			for (const auto& output : pass->outputs)
			{
				auto resource = m_resMan->GetResource(output.id);
				//auto& states = resourceDesiredStates[resource];

				// We assert that state has to be COMMON, meaning it is uninitialized (no writers and no readers yet)
				assert(resourceDesiredStates[resource] == D3D12_RESOURCE_STATE_COMMON);
				//assert(states == D3D12_RESOURCE_STATE_COMMON);

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





	u64 RenderGraph::PassResources::GetView(RGResourceID id) const
	{
		assert(m_views.contains(id));
		return m_views.find(id)->second;
	}

	Texture RenderGraph::PassResources::GetTexture(RGResourceID id)
	{
		assert(m_resMan->GetResourceType(id) == RGResourceType::Texture);
		return Texture(m_resMan->GetResource(id));
	}

	Buffer RenderGraph::PassResources::GetBuffer(RGResourceID id)
	{
		assert(m_resMan->GetResourceType(id) == RGResourceType::Buffer);
		return Buffer(m_resMan->GetResource(id));
	}



}