#include "Pathfinder.h"
#include <limits>
#include <unordered_map>
#include "Game/PcgLevelLoader.h"
#include "PathfinderSystems.h"
#include "Game/AgentManager/AgentManager.h"

using namespace DOG;
using namespace DirectX::SimpleMath;
using PortalID = DOG::entity;

Pathfinder Pathfinder::s_instance;
bool Pathfinder::m_initialized = false;

Pathfinder::Pathfinder() noexcept
{
#ifdef _DEBUG
	m_visualizePaths = false;
	m_vizPortals = false;
	m_vizNavMeshes = false;
	m_vizOutlines = true;
#else
	m_visualizePaths = false;
	m_vizPortals = false;
	m_vizNavMeshes = false;
	m_vizOutlines = true;
#endif
}

Pathfinder::~Pathfinder()
{
}

void Pathfinder::Init()
{
	// Register pathfinder systems
	EntityManager& em = EntityManager::Get();
	em.RegisterSystem(std::make_unique<VisualizePathCleanUpSystem>());
	em.RegisterSystem(std::make_unique<PathfinderWalkSystem>());

	m_initialized = true;

	ImGuiMenuLayer::RegisterDebugWindow("Pathfinder", [](bool& open)
		{
			Pathfinder::Get().VisualizePathsMenu(open);
		}, false, std::make_pair(Key::LCtrl, Key::P));
}

void Pathfinder::VisualizePathsMenu(bool& open)
{
	if (ImGui::BeginMenu("View"))
	{
		if (ImGui::MenuItem("Pathfinder", "Ctrl+P"))
		{
			open = true;
		}
		ImGui::EndMenu(); // "View"
	}

	if (open)
	{
		if (ImGui::Begin("Pathfinder", &open, ImGuiWindowFlags_NoFocusOnAppearing))
		{
			ImGui::Checkbox("Visualize paths", &m_visualizePaths);

			if (ImGui::Checkbox("Visualize NavMeshes", &m_vizNavMeshes))
			{
				if (m_vizNavMeshes)
				{
					EntityManager& em = EntityManager::Get();
					em.Collect<NavMeshComponent, BoundingBoxComponent>().Do(
						[&](entity e, NavMeshComponent&, BoundingBoxComponent& bb)
						{
							// visualize NavMesh
							em.AddComponent<TransformComponent>(e).SetPosition(bb.Center()).SetScale(NAVMESH_SCALE);
							em.AddComponent<ModelComponent>(e, AssetManager::Get().LoadShapeAsset(NAVMESH_SHAPE, NAVMESH_TESS));
							if (m_vizOutlines && !em.HasComponent<OutlineComponent>(e))
							{
								auto& comp = em.AddComponent<OutlineComponent>(e);
								comp.color = NAVMESH_COLOR;
								comp.onlyOutline = true;
							}
						}
					);
				}
				else
				{
					EntityManager& em = EntityManager::Get();
					em.Collect<NavMeshComponent, BoundingBoxComponent, TransformComponent, ModelComponent>().Do(
						[&](entity e, NavMeshComponent&, BoundingBoxComponent&, TransformComponent&, ModelComponent&)
						{
							// visualize NavMesh
							em.RemoveComponent<TransformComponent>(e);
							em.RemoveComponent<ModelComponent>(e);
							if (m_vizOutlines && !em.HasComponent<OutlineComponent>(e))
							{
								auto& comp = em.AddComponent<OutlineComponent>(e);
								comp.color = NAVMESH_COLOR;
								comp.onlyOutline = true;
							}
						}
					);
				}
			}
			if (ImGui::Checkbox("Visualize Portals", &m_vizPortals))
			{
				if (m_vizPortals)
				{
					EntityManager& em = EntityManager::Get();
					em.Collect<PortalComponent>().Do(
						[&](entity e, PortalComponent& pc)
						{
							// visualize portal
							if (!em.HasComponent<TransformComponent>(e)) em.AddComponent<TransformComponent>(e).SetPosition(pc.portal).SetScale(PORTAL_SCALE);
							em.AddComponent<ModelComponent>(e, AssetManager::Get().LoadShapeAsset(PORTAL_SHAPE, PORTAL_TESS));
							if (m_vizOutlines && !em.HasComponent<OutlineComponent>(e))
							{
								auto& comp = em.AddComponent<OutlineComponent>(e);
								comp.color = PORTAL_COLOR;
								comp.onlyOutline = true;
							}
						}
					);
				}
				else
				{
					EntityManager& em = EntityManager::Get();
					em.Collect<PortalComponent, TransformComponent, ModelComponent>().Do(
						[&](entity e, PortalComponent&, TransformComponent&, ModelComponent&)
						{
							// visualize portal
							em.RemoveComponent<TransformComponent>(e);
							em.RemoveComponent<ModelComponent>(e);
							if (m_vizOutlines && !em.HasComponent<OutlineComponent>(e))
							{
								auto& comp = em.AddComponent<OutlineComponent>(e);
								comp.color = PORTAL_COLOR;
								comp.onlyOutline = true;
							}
						}
					);
				}
			}
			if (ImGui::Checkbox("Outlines", &m_vizOutlines))
			{
				if (m_vizOutlines)
				{
					EntityManager& em = EntityManager::Get();
					// outline NavMeshes
					em.Collect<NavMeshComponent, BoundingBoxComponent>().Do(
						[&](entity e, NavMeshComponent&, BoundingBoxComponent&)
						{
							auto& comp = em.AddComponent<OutlineComponent>(e);
							comp.color = NAVMESH_COLOR;
							comp.onlyOutline = true;
						}
					);
					// outline Portals
					em.Collect<PortalComponent, TransformComponent, ModelComponent>().Do(
						[&](entity e, PortalComponent&, TransformComponent&, ModelComponent&)
						{
							auto& comp = em.AddComponent<OutlineComponent>(e);
							comp.color = PORTAL_COLOR;
							comp.onlyOutline = true;
						}
					);
				}
				else
				{
					// remove NavMesh outlines
					EntityManager& em = EntityManager::Get();
					em.Collect<NavMeshComponent, TransformComponent, ModelComponent, OutlineComponent>().Do(
						[&](entity e, NavMeshComponent&, TransformComponent&, ModelComponent&, OutlineComponent&)
						{
							em.RemoveComponent<OutlineComponent>(e);
						}
					);
					// remove Portal outlines
					em.Collect<PortalComponent, TransformComponent, ModelComponent, OutlineComponent>().Do(
						[&](entity e, PortalComponent&, TransformComponent&, ModelComponent&, OutlineComponent&)
						{
							em.RemoveComponent<OutlineComponent>(e);
						}
					);
				}
			}
		}
		ImGui::End();
	}
}

bool Pathfinder::Visualize(Viz type)
{
	if (type == Viz::Paths)
		return m_visualizePaths;
	if (type == Viz::Outlines)
		return m_vizOutlines;
	
	return true;
}

void Pathfinder::BuildNavScene(SceneComponent::Type sceneType)
{
	constexpr float blockDim = pcgBlock::DIMENSION;

	EntityManager& em = EntityManager::Get();

	entity navSceneID = em.CreateEntity();
	em.AddComponent<SceneComponent>(navSceneID, sceneType);
	NavSceneComponent& navScene = em.AddComponent<NavSceneComponent>(navSceneID);

	em.Collect<ModularBlockComponent, TransformComponent>().Do(
		[&](entity e, ModularBlockComponent&, TransformComponent& trans)
		{		
			// find x, y and z coordinates
			Vector3 pos = trans.GetPosition();
			size_t x = static_cast<size_t>(pos.x / blockDim);
			size_t y = static_cast<size_t>(pos.y / blockDim);
			size_t z = static_cast<size_t>(pos.z / blockDim);
			
			// create NavMesh and add to navScene
			entity newMesh = em.CreateEntity();
			navScene.AddIdAt(x, y, z, newMesh);
			em.AddComponent<NavMeshComponent>(newMesh);
			em.AddComponent<SceneComponent>(newMesh, sceneType);
			BoundingBoxComponent& bb = em.AddComponent<BoundingBoxComponent>(newMesh, em.GetComponent<BoundingBoxComponent>(e));
			if (m_vizNavMeshes)
			{
				// visualize NavMesh
				em.AddComponent<TransformComponent>(newMesh).SetPosition(bb.Center()).SetScale(NAVMESH_SCALE);
				em.AddComponent<ModelComponent>(newMesh, AssetManager::Get().LoadShapeAsset(NAVMESH_SHAPE, NAVMESH_TESS));
				if (m_vizOutlines)
				{
					auto& comp = em.AddComponent<OutlineComponent>(newMesh);
					comp.color = NAVMESH_COLOR;
					comp.onlyOutline = true;
				}

			}
		});

	em.Collect<EmptySpaceComponent>().Do(
		[&](entity e, EmptySpaceComponent& empty)
		{
			// find x, y and z coordinates
			size_t x = static_cast<size_t>(empty.pos.x / blockDim);
			size_t y = static_cast<size_t>(empty.pos.y / blockDim);
			size_t z = static_cast<size_t>(empty.pos.z / blockDim);
			
			// create NavMesh and add to navScene
			entity newMesh = em.CreateEntity();
			navScene.AddIdAt(x, y, z, newMesh);
			em.AddComponent<NavMeshComponent>(newMesh);
			em.AddComponent<SceneComponent>(newMesh, sceneType);
			BoundingBoxComponent& bb = em.AddComponent<BoundingBoxComponent>(newMesh, em.GetComponent<BoundingBoxComponent>(e));
			if (m_vizNavMeshes)
			{
				// visualize NavMesh
				em.AddComponent<TransformComponent>(newMesh).SetPosition(bb.Center()).SetScale(NAVMESH_SCALE);
				em.AddComponent<ModelComponent>(newMesh, AssetManager::Get().LoadShapeAsset(NAVMESH_SHAPE, NAVMESH_TESS));
				if (m_vizOutlines)
				{
					auto& comp = em.AddComponent<OutlineComponent>(newMesh);
					comp.color = NAVMESH_COLOR;
					comp.onlyOutline = true;
				}
			}
		});


	// Connect the NavMeshes
	for (size_t y = 0; y < navScene.map.size(); ++y)
	{
		for (size_t z = 0; z < navScene.map[y].size(); ++z)
		{
			for (size_t x = 0; x < navScene.map[y][z].size(); ++x)
			{
				if (navScene.HasNavMesh(x, y, z))
				{
					// connect mesh to neighbors
					entity me = navScene.At(x, y, z);
					NavMeshComponent& myMesh = em.GetComponent<NavMeshComponent>(me);
					BoundingBoxComponent& bb = em.GetComponent<BoundingBoxComponent>(me);

					for (Step dir : {Dir::down, Dir::north, Dir::east, Dir::south, Dir::west, Dir::up})
					{
						Vector3 extDir = dir.Vec3() * bb.Extents();
						// shrink NavMesh if hit detected else connect if neighbor exists
						if (auto hit = PhysicsEngine::RayCast(bb.Center(), bb.Center() + extDir); hit)
						{
							Vector3 juxtaPoint = bb.Center() - extDir;
							bb.Center((hit->hitPosition + juxtaPoint) / 2);
							Vector3 masked = dir.InversePositive().Vec3() * bb.Extents();
							f32 halfDist = Vector3::Distance(hit->hitPosition, juxtaPoint) / 2;
							Vector3 newExt = dir.Positive().Vec3() * halfDist;
							bb.Extents(masked + newExt);
						}
						else if (navScene.HasNavMesh(x + dir.x, y + dir.y, z + dir.z))
						{
							entity other = navScene.At(x + dir.x, y + dir.y, z + dir.z);
							BoundingBoxComponent& obb = em.GetComponent<BoundingBoxComponent>(other);
							extDir = (-dir).Vec3() * obb.Extents();
							if (auto ohit = PhysicsEngine::RayCast(obb.Center(), obb.Center() + extDir); ohit)
							{
								Vector3 juxtaPoint = obb.Center() - extDir;
								obb.Center((ohit->hitPosition + juxtaPoint) / 2);
								Vector3 masked = dir.InversePositive().Vec3() * obb.Extents();
								f32 halfDist = Vector3::Distance(ohit->hitPosition, juxtaPoint) / 2;
								Vector3 newExt = dir.Positive().Vec3() * halfDist;
								obb.Extents(masked + newExt);
							}
							// if meshes not connected, create portal
							else if (!myMesh.Connected(me, other))
							{
								// add new portal to me and other
								PortalID id = myMesh.portals.emplace_back(em.CreateEntity());
								em.GetComponent<NavMeshComponent>(other).portals.push_back(id);

								// add necessary components to portal
								Vector3 pos = em.AddComponent<PortalComponent>(id, me, other).portal;
								em.AddComponent<SceneComponent>(id, sceneType);

								if (m_vizPortals)
								{
									// visualize Portals
									em.AddComponent<TransformComponent>(id).SetPosition(pos).SetScale(PORTAL_SCALE);
									em.AddComponent<ModelComponent>(id, AssetManager::Get().LoadShapeAsset(PORTAL_SHAPE, PORTAL_TESS));
									if (m_vizOutlines)
									{
										auto& comp = em.AddComponent<OutlineComponent>(id);
										comp.color = PORTAL_COLOR;
										comp.onlyOutline = true;
									}
								}
							}
						}
					}
				}
			}
		}
	}
}


std::vector<Vector3> Pathfinder::Checkpoints(Vector3 start, Vector3 goal)
{
	EntityManager& em = EntityManager::Get();

	std::vector<Vector3> checkpoints;

	for (PortalID id : Astar(start, goal, heuristicStraightLine))
		checkpoints.push_back(em.GetComponent<PortalComponent>(id).portal);

	checkpoints.push_back(goal);

	return checkpoints;
}

void Pathfinder::Checkpoints(Vector3 start, PathfinderWalkComponent& pfc)
{
	EntityManager::Get().Collect<NavSceneComponent>().Do(
		[&](NavSceneComponent& navScene)
		{
			if (NavMeshID startMesh = navScene.At(start); startMesh != NULL_ENTITY)
			{
				if (NavMeshID terminalNavMesh = navScene.At(pfc.goal); terminalNavMesh != NULL_ENTITY)
				{
					// Recalculate each frame...
					EntityManager& em = EntityManager::Get();
					pfc.path.clear();
					for (PortalID id : Astar(start, pfc.goal, heuristicStraightLine))
						pfc.path.push_back(em.GetComponent<PortalComponent>(id).portal);

					pfc.path.push_back(pfc.goal);

					// remove the first checkpoint if entity is close enough
					constexpr float THRESHOLD = .05f;
					constexpr float THRESHOLD_SQ = THRESHOLD * THRESHOLD;
					if (pfc.path.size() > 1 && Vector3::DistanceSquared(start, pfc.path[0]) > THRESHOLD_SQ)
						pfc.path.erase(pfc.path.begin());
				}
			}
		});
}



std::vector<PortalID> Pathfinder::Astar(const Vector3 start, const Vector3 goal, float(*h)(Vector3, Vector3))
{
	struct MaxFloat
	{	// float wrapper that has default value infinity
		float score;
		MaxFloat() : score(std::numeric_limits<float>::infinity()) {}
		void	operator=(const float other)		{ score = other; }
		float	operator=(const MaxFloat& other)	{ return other.score; }
		float	operator+(const float other)		{ return this->score + other; }
		bool	operator<(const float other)		{ return this->score < other; }
		bool	operator<(const MaxFloat& other)	{ return this->score < other.score; }
		bool	operator>(const MaxFloat& other)	{ return this->score > other.score; }
		bool	operator>(const float other)		{ return this->score > other; }
	};

	EntityManager& em = EntityManager::Get();

	std::vector<PortalID> result;

	em.Collect<NavSceneComponent>().Do(
		[&](NavSceneComponent& navScene)
		{
			if (NavMeshID startMesh = navScene.At(start); startMesh != NULL_ENTITY)
			{
				if (NavMeshID terminalNavMesh = navScene.At(goal); terminalNavMesh != NULL_ENTITY)
				{
					// Define the entry and terminal nodes
					// Opportunity to optimize:
					// if FindNavMeshContaining(start) == FindNavMeshContaining(goal) return empty path

					PortalComponent entry = PortalComponent(startMesh, start);
					constexpr PortalID startPoint = NULL_ENTITY;

					std::vector<PortalID> openSet = { startPoint };
					std::unordered_map<PortalID, PortalID> cameFrom;
					std::unordered_map<PortalID, MaxFloat> fScore;
					fScore[startPoint] = h(start, goal);
					std::unordered_map<PortalID, MaxFloat> gScore;
					gScore[startPoint] = 0;

					// Lamdas used in A*
					// ------------------------------------------------------------------------
					// lambda: pop element with lowest fScore from minheap
					auto popOpenSet = [&]()
					{
						PortalID get = openSet[0];
						openSet[0] = openSet.back();
						openSet.pop_back();
						// percolate down
						size_t i = 0;
						size_t comp = i * 2 + 1;		// left child
						while (comp < openSet.size())
						{
							size_t right = i * 2 + 2;	// right child
							if (right < openSet.size())
								if (fScore[openSet[right]] < fScore[openSet[comp]])
									comp = right;
							if (fScore[openSet[comp]] < fScore[openSet[i]])
							{
								std::swap(openSet[i], openSet[comp]);
								i = comp;
								comp = i * 2 + 1;
							}
							else
								comp = openSet.size();
						}
						return get;
					};
					// lambda: push new element onto minheap
					auto pushOpenSet = [&](PortalID id)
					{
						openSet.push_back(id);
						// percolate up
						size_t i = openSet.size() - 1;
						size_t p = (i + (i % 2)) / 2 - 1;
						while (i > 0 && p >= 0 && fScore[openSet[p]] > fScore[openSet[i]])
						{
							std::swap(openSet[p], openSet[i]);
							i = p;
							p = (i + (i % 2)) / 2 - 1;
						}
					};
					// lambda: returns true if current node is connected to the NavMesh containing the goal
					auto leadsToGoal = [&](PortalID current)
					{
						PortalComponent* currentPortal = nullptr;
						if (current == startPoint)
							currentPortal = &entry;
						else
							currentPortal = &em.GetComponent<PortalComponent>(current);
						
						return currentPortal->navMesh1 == terminalNavMesh || currentPortal->navMesh2 == terminalNavMesh;
					};
					// lambda: returns true if openSet does not contain neighbor
					auto notInOpenSet = [&](PortalID neighbor)
					{
						for (PortalID node : openSet)
							if (node == neighbor)
								return false;
						return true;
					};
					auto getNeighbors = [&](PortalID current)
					{
						if (current == startPoint)
							return em.GetComponent<NavMeshComponent>(entry.navMesh1).portals;
						std::vector<PortalID> neighbors;
						PortalComponent& currentPortal = em.GetComponent<PortalComponent>(current);
						for (NavMeshID id : { currentPortal.navMesh1, currentPortal.navMesh2 })
							for (PortalID portalID : em.GetComponent<NavMeshComponent>(id).portals)
								if (portalID != current)
									neighbors.push_back(portalID);
						return neighbors;
					};
					// lambda: d - distance between nodes
					auto d = [&](PortalID current, PortalID neighbor)
					{
						// get the portals
						PortalComponent* currentPortal = nullptr;
						if (current == startPoint)
							currentPortal = &entry;
						else
							currentPortal = &em.GetComponent<PortalComponent>(current);
						PortalComponent* neighborPortal = nullptr;
						if (neighbor == startPoint)
							neighborPortal = &entry;
						else
							neighborPortal = &em.GetComponent<PortalComponent>(neighbor);
						// find the common NavMesh
						NavMeshID meshID;
						meshID = currentPortal->navMesh1;
						if (neighborPortal->navMesh1 != meshID || neighborPortal->navMesh2 != meshID)
							meshID = currentPortal->navMesh2;
						// meshID must now hold the common NavMesh
						return em.GetComponent<NavMeshComponent>(meshID).CostWalk(currentPortal->portal, neighborPortal->portal);
					};
					// lambda: creates the shortest path found
					auto reconstructPath = [&](PortalID portalID)
					{
						//std::vector<PortalID> path;
						while (cameFrom.contains(portalID))
						{
							if (portalID != startPoint)
								// exclude starting point from path
								result.push_back(portalID);
							portalID = cameFrom[portalID];
						}
						// reverse path
						std::reverse(result.begin(), result.end());
						//return path;
					};

					// ---------------------------------------------------------------------
					// A* implementation
					while (!openSet.empty())
					{
						PortalID current = popOpenSet();
						if (leadsToGoal(current))
						{
							reconstructPath(current);	// fills result with PortalIDs
							break;
						}

						for (PortalID neighbor : getNeighbors(current))
						{
							float tentativeGScore = gScore[current] + d(current, neighbor);
							if (gScore[neighbor] > tentativeGScore)
							{
								cameFrom[neighbor] = current;
								gScore[neighbor] = tentativeGScore;
								fScore[neighbor] = tentativeGScore + h(em.GetComponent<PortalComponent>(neighbor).portal, goal);
								if (notInOpenSet(neighbor))
									pushOpenSet(neighbor);
							}
						}
					}
				}
				// else trivial navigation within NavMesh
			}
			// else invalid starting position
		});

	return result;
}

float Pathfinder::heuristicStraightLine(Vector3 start, Vector3 goal)
{
	return (goal - start).Length();
}
