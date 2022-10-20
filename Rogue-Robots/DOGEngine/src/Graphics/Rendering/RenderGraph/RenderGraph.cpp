#include "RenderGraph.h"
#include "RGResourceManager.h"
#include "../../RHI/RenderDevice.h"
#include "../GPUGarbageBin.h"
#include <set>
#include <fstream>

#include "Tracy/Tracy.hpp"

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
		m_passDataAllocator = std::make_unique<BumpAllocator>(131'072);	// 128 Kb
	}

	void RenderGraph::Clear()
	{
		m_dirty = true;


		// Clean up views
		for (const auto& pass : m_sortedPasses)
		{
			{
				auto df = [rd = m_rd, views = std::move(pass->passResources.m_bufferViews)]() mutable
				{
					for (const auto& view : views)
						rd->FreeView(view);
				};
				m_bin->PushDeferredDeletion(df);
			}

			{
				auto df = [rd = m_rd, views = std::move(pass->passResources.m_textureViews)]() mutable
				{
					for (const auto& view : views)
						rd->FreeView(view);
				};
				m_bin->PushDeferredDeletion(df);
			}

			if (pass->rp)
			{
				auto df = [rd = m_rd, rp = *pass->rp]()
				{
					rd->FreeRenderPass(rp);
				};
				m_bin->PushDeferredDeletion(df);
			}

			pass->passResources = {};
			pass->rp = std::nullopt;
		}


		// Clear resources declared by this graph
		m_resMan->ClearDeclaredResources();

		// Clear cached local graph data
		m_passBuilderGlobalData = {};
		m_nextPassID = 0;
		m_passes.clear();
		m_adjacencyMap.clear();
		m_sortedPasses.clear();
		m_maxDepth = 0;
		m_dependencyLevels.clear();
	}

	void RenderGraph::TryBuild()
	{
		if (m_dirty)
			Build();
	}

	void RenderGraph::Build()
	{
		{
			ZoneNamedN(RGAddProxies, "RG Building: Add Proxies", true);
			AddProxies();
		}

		{
			ZoneNamedN(RGBuildAdjacencyMap, "RG Building: Build AdjacencyMap", true);
			BuildAdjacencyMap();
		}

		// To help the graph author see what's going on.
		// @TODO: Expand with input/output labels on each pass
#ifdef GENERATE_GRAPHVIZ
		GenerateGraphviz();
#endif

		{
			ZoneNamedN(RGSortTopological, "RG Building: Topological Sort", true);
			SortPassesTopologically();
		}

		{
			ZoneNamedN(RGAssignDepLevels, "RG Building: Assign Dependency Levels", true);
			AssignDependencyLevels();
		}

		{
			ZoneNamedN(RGBuildDepLevels, "RG Building: Build Dependency Levels", true);
			BuildDependencyLevels();
		}

		// Walk graph topologically to obtain all data necessary for sanitizing
		{
			ZoneNamedN(RGTrackLifetimes, "RG Building: Track Lifetimes", true);
			TrackLifetimes();
		}

		{
			ZoneNamedN(RGSanitizeAliasingLifetimes, "RG Building: Sanitize Aliasing Lifetimes", true);
			m_resMan->SanitizeAliasingLifetimes();
		}

		{
			ZoneNamedN(RGRealizeResources, "RG Building: Realize Resources", true);
			m_resMan->RealizeResources();
		}

		// Resources NEED to be realized from this point forward!

		{
			ZoneNamedN(RGRealizeViews, "RG Building: Realize Views", true);
			//RealizeViews();
		}

		{
			ZoneNamedN(RGTrackTransitions, "RG Building: Track Transitions", true);
			TrackTransitions();
		}

		m_dirty = false;
	}

	void RenderGraph::Execute()
	{
		assert(!m_dirty);

		{
			ZoneNamedN(RGAddProxies, "RG Exec: Setup Metadata", true);

			// Clean up views
			for (const auto& pass : m_sortedPasses)
			{
				{
					auto df = [rd = m_rd, views = std::move(pass->passResources.m_bufferViews)]() mutable
					{
						for (const auto& view : views)
							rd->FreeView(view);
					};
					m_bin->PushDeferredDeletion(df);
				}

				{
					auto df = [rd = m_rd, views = std::move(pass->passResources.m_textureViews)]() mutable
					{
						for (const auto& view : views)
							rd->FreeView(view);
					};
					m_bin->PushDeferredDeletion(df);
				}

				if (pass->rp)
				{
					auto df = [rd = m_rd, rp = *pass->rp]()
					{
						rd->FreeRenderPass(rp);
					};
					m_bin->PushDeferredDeletion(df);
				}

				pass->passResources = {};
				pass->rp = std::nullopt;
			}

			// @todo: Track new transitions (for each transition --> Just keep track of ResourceID and update the underlying resource using GetResourceState()
			//for (auto& dep : m_dependencyLevels)
			//	dep.ClearBarriers();
			//TrackTransitions();

			// Recreate views for this new graph
			RealizeViews();
		}





		m_cmdl = m_rd->AllocateCommandList();

		for (auto& pass : m_sortedPasses)
		{
			if (pass->preGraphExecute)
				(*pass->preGraphExecute)();
		}

		for (auto& depLevel : m_dependencyLevels)
			depLevel.Execute(m_rd, m_cmdl);

		m_resMan->ImportedResourceExitTransition(m_cmdl);
		m_resMan->DeclaredResourceTransitionToInit(m_cmdl);

		m_rd->SubmitCommandList(m_cmdl);


		for (auto& pass : m_sortedPasses)
		{
			if (pass->preGraphExecute)
				(*pass->postGraphExecute)();
		}

		// Clean up command list
		auto delFunc = [rd = m_rd, cmdl = m_cmdl]()
		{
			rd->RecycleCommandList(cmdl);
		};
		m_bin->PushDeferredDeletion(delFunc);

		m_passDataAllocator->Clear();

	}

	void RenderGraph::AddProxies()
	{
		u32 proxyID{ 0 };
		for (const auto& trackedProxies : m_passBuilderGlobalData.proxies)
		{
			auto id = RGResourceID("Proxy" + std::to_string(proxyID));
			for (auto& pass : m_passes)
			{
				if (pass->id == trackedProxies.first)
					pass->proxyOutput.push_back(id);
				else if (pass->id == trackedProxies.second)
					pass->proxyInput.push_back(id);
			}
			++proxyID;
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
		m_dependencyLevels.reserve(m_maxDepth + 1);
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
				m_resMan->ResolveLifetime(input.id, pass->depth);
			for (const auto& output : pass->outputs)
				m_resMan->ResolveLifetime(output.id, pass->depth);
		}
	}

	void RenderGraph::TrackTransitions()
	{
		struct TransitionMetadata
		{
			D3D12_RESOURCE_STATES before{ D3D12_RESOURCE_STATE_COMMON };
			D3D12_RESOURCE_STATES after{ D3D12_RESOURCE_STATE_COMMON };
			RGResourceType type{ RGResourceType::Texture };
		};

		// Resolve state transitions PER dependency level (finds read state-combines if any)
		for (auto& depLevel : m_dependencyLevels)
		{
			std::unordered_map<RGResourceID, TransitionMetadata> resourceStates;
			for (const auto& pass : depLevel.GetPasses())
			{
				for (const auto& input : pass->inputs)
				{
					// Alias write is a special case --> Underlying resource WILL be the same
					// This is the only simultaneous read-write we will allow (this will get picked up when iterating over outputs)
					if (input.aliasWrite)
						continue;

					auto& states = resourceStates[input.id];
					states.before = m_resMan->GetCurrentState(input.id);
					states.type = input.type;

					// If tracked state is read combination --> Forbid writes
					if (IsReadState(states.after))
						assert(IsReadState(input.desiredState));
					// If write state --> Exclusive writer pass already exists and all other accesses are forbidden
					else
						assert(states.after == D3D12_RESOURCE_STATE_COMMON);	// Common is synonymous with uninitialized here

					// Read state-combine if all is well
					states.after |= input.desiredState;
				}

				for (const auto& output : pass->outputs)
				{
					// Sanity check that the graph author always puts a write state for outputs
					assert(!IsReadState(output.desiredState));

					auto& states = resourceStates[output.id];
					states.before = m_resMan->GetCurrentState(output.id);
					states.type = output.type;

					// We assert that state has to be COMMON, meaning it is uninitialized (no writers and no readers yet)
					// If this asserts false --> A read or write has already been applied --> Illegal simultaneous read/write detected
					assert(states.after == D3D12_RESOURCE_STATE_COMMON);

					// Set exclusive write state
					states.after = output.desiredState;
				}
				
			}

			for (const auto& [rgResource, states] : resourceStates)
			{
				auto resource = m_resMan->GetResource(rgResource);

				// https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ns-d3d12-d3d12_resource_uav_barrier
				// UAV Barrier not required if user only does read on the resource.
				// But we will always assume a write and insert a UAV barrier to avoid potential user bugs (e.g specifying read only but actually writing to it)
				// We cannot detect if a user writes to a resource or not (since it's on the shader side)
				std::optional<GPUBarrier> uavBarrier;
				GPUBarrier transitionBarrier{};

				if (states.type == RGResourceType::Texture)
				{
					transitionBarrier = GPUBarrier::Transition(
						Texture(resource),
						D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
						states.before, states.after);

					if (states.before == D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
						uavBarrier = GPUBarrier::UAV(Texture(resource));
				}
				else
				{
					transitionBarrier = GPUBarrier::Transition(
						Buffer(resource),
						D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
						states.before, states.after);

					if (states.before == D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
						uavBarrier = GPUBarrier::UAV(Buffer(resource));
				}

				// Assure that any acceses AFTER an unordered access always gets write results
				if (uavBarrier)
					depLevel.AddEntryBarrier(*uavBarrier, rgResource);

				// No need for state transition, return early
				if (states.before != states.after)
					depLevel.AddEntryBarrier(transitionBarrier, rgResource);

				m_resMan->SetCurrentState(rgResource, states.after);
			}
		}
	}

	void RenderGraph::RealizeViews()
	{
		for (auto& pass : m_sortedPasses)
		{
			auto& passResources = pass->passResources;

			// Realize output views
			RenderPassBuilder builder;
			bool rpActive{ false };
			for (const auto& output : pass->outputs)
			{
				const auto lookupID = output.originalID ? *output.originalID : output.id;

				if (output.type == RGResourceType::Texture)
				{
					const auto& viewDesc = std::get<TextureViewDesc>(*output.viewDesc);
					const auto resource = Texture(m_resMan->GetResource(output.id));
					const auto view = m_rd->CreateView(resource, viewDesc);

					// Hold views for deallocation
					passResources.m_textureViews.push_back(view);
					passResources.m_textures[lookupID] = resource;

					if (viewDesc.viewType == ViewType::RenderTarget)
					{
						rpActive = true;

						auto accesses = GetAccessTypes(*output.rpAccessType);
						builder.AppendRT(view, accesses.first, accesses.second);
					}
					else if (viewDesc.viewType == ViewType::DepthStencil)
					{
						rpActive = true;

						auto depthAccesses = GetAccessTypes(*output.rpAccessType);
						auto stencilAccesses = GetAccessTypes(*output.rpStencilAccessType);
						builder.AddDepthStencil(view,
							depthAccesses.first, depthAccesses.second,
							stencilAccesses.first, stencilAccesses.second);
					}
					else if (viewDesc.viewType == ViewType::UnorderedAccess || viewDesc.viewType == ViewType::RaytracingAS)
					{
						passResources.m_views[output.viewID] = m_rd->GetGlobalDescriptor(view);
						passResources.m_textureViewsLookup[output.viewID] = view;
					}
				}
				else
				{
					const auto resource = Buffer(m_resMan->GetResource(output.id));
					passResources.m_buffers[lookupID] = resource;

					if (output.viewDesc)
					{
						// Create view and immediately convert to global descriptor index
						const auto& viewDesc = std::get<BufferViewDesc>(*output.viewDesc);
						auto view = m_rd->CreateView(resource, viewDesc);
						passResources.m_views[output.viewID] = m_rd->GetGlobalDescriptor(view);
						passResources.m_bufferViewsLookup[output.viewID] = view;
						passResources.m_bufferViews.push_back(view);
					}
				}
			}

			// Realize input views
			for (const auto& input : pass->inputs)
			{
				// No view desc supplied --> No view (e.g Imported doesnt have view desc)
				if (!input.viewDesc)
					continue;

				const auto lookupID = input.originalID ? *input.originalID : input.id;

				if (input.type == RGResourceType::Texture)
				{
					const auto& viewDesc = std::get<TextureViewDesc>(*input.viewDesc);
					const auto resource = Texture(m_resMan->GetResource(input.id));
					const auto view = m_rd->CreateView(resource, viewDesc);
					passResources.m_textureViews.push_back(view);
					passResources.m_textures[lookupID] = resource;

					// Read only DSV
					if (viewDesc.viewType == ViewType::DepthStencil)
					{
						rpActive = true;

						auto depthAccesses = GetAccessTypes(*input.rpAccessType);
						auto stencilAccesses = GetAccessTypes(*input.rpStencilAccessType);
						builder.AddDepthStencil(view,
							depthAccesses.first, depthAccesses.second,
							stencilAccesses.first, stencilAccesses.second);
					}
					else 
					{
						passResources.m_views[input.viewID] = m_rd->GetGlobalDescriptor(view);
						passResources.m_textureViewsLookup[input.viewID] = view;
					}
				}
				else
				{
					const auto& viewDesc = std::get<BufferViewDesc>(*input.viewDesc);

					// Create view and immediately convert to global descriptor index
					const auto resource = Buffer(m_resMan->GetResource(input.id));
					auto view = m_rd->CreateView(resource, viewDesc);
					passResources.m_bufferViews.push_back(view);
					passResources.m_buffers[lookupID] = resource;
					passResources.m_views[input.viewID] = m_rd->GetGlobalDescriptor(view);
					passResources.m_bufferViewsLookup[input.viewID] = view;
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
		file.open("Assets\\rendergraph.txt");
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





	void RenderGraph::PassBuilder::DeclareTexture(RGResourceID name, RGTextureDesc desc)
	{
		m_resMan->DeclareTexture(name, desc);
	}

	void RenderGraph::PassBuilder::ImportTexture(RGResourceID name, Texture texture, D3D12_RESOURCE_STATES entryState, D3D12_RESOURCE_STATES exitState)
	{
		m_resMan->ImportTexture(name, texture, entryState, exitState);
	}

	void RenderGraph::PassBuilder::DeclareBuffer(RGResourceID id, RGBufferDesc desc)
	{
		m_resMan->DeclareBuffer(id, desc);
	}

	void RenderGraph::PassBuilder::ImportBuffer(RGResourceID id, Buffer buffer, D3D12_RESOURCE_STATES entryState, D3D12_RESOURCE_STATES exitState)
	{
		m_resMan->ImportBuffer(id, buffer, entryState, exitState);
	}



	RGResourceView RenderGraph::PassBuilder::ReadResource(RGResourceID id, D3D12_RESOURCE_STATES state, TextureViewDesc desc)
	{
		assert(IsReadState(state));
		assert(desc.viewType == ViewType::ShaderResource || desc.viewType == ViewType::RaytracingAS);

		// Assert that the RG has written to this resource before
		assert(m_globalData.writes.contains(id));

		PassIO input;
		input.originalID = id;

		// Automatically deduces the correct read if ID is an aliased resource
		PushPassReader(id);
		id = GetPrevious(id);

		input.id = id;
		input.desiredState = state;
		input.viewDesc = desc;
		input.type = RGResourceType::Texture;
		input.viewID.id = m_globalData.viewCount++;
		m_pass.inputs.push_back(input);

		return input.viewID;
	}

	void RenderGraph::PassBuilder::ReadDepthStencil(RGResourceID id, TextureViewDesc desc)
	{
		assert(desc.viewType == ViewType::DepthStencil);

		// Verify that the user hasn't forgotten the read only flags on views
		assert(desc.depthReadOnly && (desc.depthReadOnly || desc.stencilReadOnly));
		
		PassIO input{};
		input.id = id;
		input.type = RGResourceType::Texture;
		input.viewDesc = desc;
		input.desiredState = D3D12_RESOURCE_STATE_DEPTH_READ;
		input.rpAccessType = RenderPassAccessType::PreservePreserve;
		input.rpStencilAccessType = RenderPassAccessType::PreservePreserve;
		m_pass.inputs.push_back(input);
	}

	void RenderGraph::PassBuilder::WriteDepthStencil(RGResourceID id, RenderPassAccessType depthAccess, TextureViewDesc desc, RenderPassAccessType stencilAccess)
	{
		assert(desc.viewType == ViewType::DepthStencil);
		assert(!desc.depthReadOnly);
		assert(!desc.stencilReadOnly);
		assert(!m_globalData.writes.contains(id));		// No aliasing support for depth stencil

		PassIO output{};
		output.id = id;
		output.type = RGResourceType::Texture;
		output.viewDesc = desc;
		output.desiredState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
		output.rpAccessType = depthAccess;
		output.rpStencilAccessType = stencilAccess;
		m_pass.outputs.push_back(output);

		m_globalData.writeCount[id] = 1;
		m_globalData.writes.insert(id);
	}

	void RenderGraph::PassBuilder::WriteRenderTarget(RGResourceID id, RenderPassAccessType access, TextureViewDesc desc)
	{
		assert(desc.viewType == ViewType::RenderTarget);

		// Automatically aliases if same resource already exists
		if (m_globalData.writes.contains(id))
		{
			// Explicitly connects previous reads on ID to newID
			// The previous reads that are connect are the previous reads SINCE a write.
			const auto ids = ResolveAliasingIDs(id);

			// Explicitly connects prevID to newID
			const auto& prevID = ids.first;
			const auto& newID = ids.second;

			assert(!m_globalData.writes.contains(newID));
			m_globalData.writes.insert(newID);

			m_resMan->AliasResource(newID, prevID, RGResourceType::Texture);

			PassIO input;
			input.originalID = id;
			input.id = prevID;
			input.type = RGResourceType::Texture;
			input.aliasWrite = true;
			m_pass.inputs.push_back(input);

			PassIO output;
			output.originalID = id;
			output.id = newID;
			output.type = RGResourceType::Texture;
			output.desiredState = D3D12_RESOURCE_STATE_RENDER_TARGET;
			output.viewDesc = desc;
			output.aliasWrite = true;
			output.rpAccessType = access;
			m_pass.outputs.push_back(output);
		}
		else
		{
			m_globalData.writeCount[id] = 1;

			PassIO output;
			output.id = id;
			output.type = RGResourceType::Texture;
			output.viewDesc = desc;
			output.desiredState = D3D12_RESOURCE_STATE_RENDER_TARGET;
			output.rpAccessType = access;
			m_pass.outputs.push_back(output);

			m_globalData.writes.insert(id);
		}
	}

	RGResourceView RenderGraph::PassBuilder::ReadWriteTarget(RGResourceID id, TextureViewDesc desc)
	{
		assert(desc.viewType == ViewType::UnorderedAccess);

		// Automatically aliases if same resource already exists
		if (m_globalData.writes.contains(id))
		{
			// Explicitly connects previous reads on ID to newID
			// The previous reads that are connect are the previous reads SINCE a write.
			const auto ids = ResolveAliasingIDs(id);

			// Explicitly connects prevID to newID
			const auto& prevID = ids.first;
			const auto& newID = ids.second;
		
			// Add aliased unordered access
			assert(!m_globalData.writes.contains(newID));
			m_globalData.writes.insert(newID);

			m_resMan->AliasResource(newID, prevID, RGResourceType::Texture);

			PassIO input;
			input.originalID = id;
			input.id = prevID;
			input.type = RGResourceType::Texture;
			input.aliasWrite = true;
			m_pass.inputs.push_back(input);

			PassIO output;
			output.originalID = id;
			output.id = newID;
			output.type = RGResourceType::Texture;
			output.desiredState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
			output.viewDesc = desc;
			output.aliasWrite = true;
			output.viewID.id = m_globalData.viewCount++;
			m_pass.outputs.push_back(output);

			return output.viewID;
		}
		else
		{
			m_globalData.writeCount[id] = 1;

			PassIO output;
			output.id = id;
			output.type = RGResourceType::Texture;
			output.viewDesc = desc;
			output.desiredState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
			output.viewID.id = m_globalData.viewCount++;
			m_pass.outputs.push_back(output);
		
			m_globalData.writes.insert(id);

			return output.viewID;
		}
	}

	RGResourceView RenderGraph::PassBuilder::ReadResource(RGResourceID id, D3D12_RESOURCE_STATES state, BufferViewDesc desc)
	{
		assert(IsReadState(state));
		assert(desc.viewType != ViewType::RenderTarget && desc.viewType != ViewType::UnorderedAccess);

		PassIO input;
		input.originalID = id;

		// Automatically deduces the correct read if ID is an aliased resource
		if (m_globalData.writes.contains(id))
		{
			PushPassReader(id);
			id = GetPrevious(id);
		}

		input.id = id;
		input.desiredState = state;
		input.viewDesc = desc;
		input.type = RGResourceType::Buffer;
		input.viewID.id = m_globalData.viewCount++;
		m_pass.inputs.push_back(input);

		return input.viewID;
	}

	RGResourceView RenderGraph::PassBuilder::ReadWriteTarget(RGResourceID id, BufferViewDesc desc)
	{
		/*
			Code is literal copy of ReadWriteTarget for texture. @todo: Refactor
			the only difference is the Texture/Buffer enums
			@todo: refactor this
		*/

		assert(desc.viewType == ViewType::UnorderedAccess);

		// Automatically aliases if same resource already exists
		if (m_globalData.writes.contains(id))
		{
			// Explicitly connects previous reads on ID to newID
			// The previous reads that are connect are the previous reads SINCE a write.
			const auto ids = ResolveAliasingIDs(id);

			// Explicitly connects prevID to newID
			const auto& prevID = ids.first;
			const auto& newID = ids.second;

			// Add aliased unordered access
			assert(!m_globalData.writes.contains(newID));
			m_globalData.writes.insert(newID);

			m_resMan->AliasResource(newID, prevID, RGResourceType::Buffer);

			PassIO input;
			input.originalID = id;
			input.id = prevID;
			input.type = RGResourceType::Buffer;
			input.aliasWrite = true;
			m_pass.inputs.push_back(input);

			PassIO output;
			output.originalID = id;
			output.id = newID;
			output.type = RGResourceType::Buffer;
			output.desiredState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
			output.viewDesc = desc;
			output.aliasWrite = true;
			output.viewID.id = m_globalData.viewCount++;
			m_pass.outputs.push_back(output);

			return output.viewID;
		}
		else
		{
			m_globalData.writeCount[id] = 1;

			PassIO output;
			output.id = id;
			output.type = RGResourceType::Buffer;
			output.viewDesc = desc;
			output.desiredState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
			output.viewID.id = m_globalData.viewCount++;
			m_pass.outputs.push_back(output);

			m_globalData.writes.insert(id);

			return output.viewID;
		}
	}

	void RenderGraph::PassBuilder::CopyToResource(RGResourceID id, RGResourceType type)
	{
		// Automatically aliases if same resource already exists
		if (m_globalData.writes.contains(id))
		{
			// Explicitly connects previous reads on ID to newID
			// The previous reads that are connect are the previous reads SINCE a write.
			const auto ids = ResolveAliasingIDs(id);

			// Explicitly connects prevID to newID
			const auto& prevID = ids.first;
			const auto& newID = ids.second;

			// Setup new ID
			assert(!m_globalData.writes.contains(newID));
			m_globalData.writes.insert(newID);

			m_resMan->AliasResource(newID, prevID, type);

			PassIO input;
			input.originalID = id;
			input.id = prevID;
			input.type = type;
			input.aliasWrite = true;
			m_pass.inputs.push_back(input);

			PassIO output;
			output.originalID = id;
			output.id = newID;
			output.type = type;
			output.desiredState = D3D12_RESOURCE_STATE_COPY_DEST;
			output.aliasWrite = true;
			m_pass.outputs.push_back(output);
		}
		else
		{
			m_globalData.writes.insert(id);
			m_globalData.writeCount[id] = 1;

			PassIO output;
			output.id = id;
			output.type = type;
			output.desiredState = D3D12_RESOURCE_STATE_COPY_DEST;
			m_pass.outputs.push_back(output);
		}
	}

	void RenderGraph::PassBuilder::CopyFromResource(RGResourceID id, RGResourceType type)
	{
		PassIO input;
		input.originalID = id;

		// Automatically deduces the correct read if ID is an aliased resource
		if (m_globalData.writes.contains(id))
		{
			PushPassReader(id);
			id = GetPrevious(id);
		}

		input.id = id;
		input.desiredState = D3D12_RESOURCE_STATE_COPY_SOURCE;
		input.type = type;
		m_pass.inputs.push_back(input);
	}





	std::pair<RGResourceID, RGResourceID> RenderGraph::PassBuilder::ResolveAliasingIDs(RGResourceID input)
	{
		/*
			Resolves previous and after aliasing IDs
			and adds appropriate proxies
		*/

		FlushReadsAndConnectProxy(input);

		const auto prevID = GetPrevious(input);
		const auto newID = GetNextID(input);

		return { prevID, newID };
	}

	void RenderGraph::PassBuilder::PushPassReader(RGResourceID id)
	{
		// Push reads
		auto& readsUntilWrite = m_globalData.latestRead[id];
		readsUntilWrite.push_back(m_pass.id);
	}

	RGResourceID RenderGraph::PassBuilder::GetPrevious(RGResourceID id)
	{
		auto& writeCount = m_globalData.writeCount[id];

		// If single write --> Use original name --> without (n) 
		auto prevName = id.name + "(" + std::to_string(writeCount) + ")";
		prevName = writeCount == 1 ? id.name : prevName;
		return RGResourceID(prevName);
	}

	RGResourceID RenderGraph::PassBuilder::GetNextID(RGResourceID id)
	{
		// Get next ID
		auto& writeCount = m_globalData.writeCount[id];
		writeCount += 1;
		auto nextName = id.name + "(" + std::to_string(writeCount) + ")";
		return RGResourceID(nextName);
	}

	void RenderGraph::PassBuilder::FlushReadsAndConnectProxy(RGResourceID id)
	{
		auto& prevReads = m_globalData.latestRead[id];
		if (!prevReads.empty())
		{
			// Latest write now connected to latest reads
			for (const auto& read : prevReads)
				m_globalData.proxies.push_back({ read, m_pass.id });
			prevReads.clear();
		}
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






	void RenderGraph::DependencyLevel::Execute(RenderDevice* rd, CommandList cmdl)
	{
		// Resolve potentially changed resources
		for (u32 i = 0; i < m_entryBarriers.size(); ++i)
		{
			m_entryBarriers[i].resource = m_resMan->GetResource(m_barrierResourceIDs[i]);
			m_resMan->SetCurrentState(m_barrierResourceIDs[i], m_entryBarriers[i].stateAfter);
		}

		if (!m_entryBarriers.empty())
			rd->Cmd_Barrier(cmdl, m_entryBarriers);

		for (const auto& pass : m_passes)
		{
			ZoneTransientN(Zone1, pass->name.c_str(), true);

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






	u32 RenderGraph::PassResources::GetView(RGResourceView id) const
	{
		assert(m_views.contains(id));
		return m_views.find(id)->second;
	}

	TextureView RenderGraph::PassResources::GetTextureView(RGResourceView id) const
	{
		assert(m_textureViewsLookup.contains(id));
		return m_textureViewsLookup.find(id)->second;
	}

	BufferView RenderGraph::PassResources::GetBufferView(RGResourceView id) const
	{
		assert(m_bufferViewsLookup.contains(id));
		return m_bufferViewsLookup.find(id)->second;
	}

	Texture RenderGraph::PassResources::GetTexture(RGResourceID id)
	{
		assert(m_textures.contains(id));
		return m_textures.find(id)->second;
	}

	Buffer RenderGraph::PassResources::GetBuffer(RGResourceID id)
	{
		assert(m_buffers.contains(id));
		return m_buffers.find(id)->second;
	}



}