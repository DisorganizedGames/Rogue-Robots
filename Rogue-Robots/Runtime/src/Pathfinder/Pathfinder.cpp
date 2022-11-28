#include "Pathfinder.h"
#include <limits>
#include <unordered_map>
#include "Game/PcgLevelLoader.h"
#include "PathfinderSystems.h"
#include "Game/AgentManager/AgentManager.h"

using namespace DOG;
using namespace DirectX::SimpleMath;
//using NavMeshID = DOG::entity;
using PortalID = DOG::entity;

Pathfinder Pathfinder::s_instance;
bool Pathfinder::m_initialized = false;

Pathfinder::Pathfinder() noexcept
{
	m_visualizePaths = true;
	m_vizNavMeshes = false;
	m_vizPortals = false;
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
					constexpr Vector3 navMeshScale = Vector3(1.f, 1.f, 1.f);
					EntityManager& em = EntityManager::Get();
					em.Collect<NavMeshComponent, BoundingBoxComponent>().Do(
						[&](entity e, NavMeshComponent&, BoundingBoxComponent& bb)
						{
							// visualize NavMesh
							em.AddComponent<TransformComponent>(e).SetPosition(bb.Center()).SetScale(navMeshScale);
							em.AddComponent<ModelComponent>(e, AssetManager::Get().LoadShapeAsset(DOG::Shape::sphere, 16));
						}
					);
				}
				else
				{
					EntityManager& em = EntityManager::Get();
					em.Collect<NavMeshComponent, BoundingBoxComponent, TransformComponent, ModelComponent>().Do(
						[&](entity e, NavMeshComponent&, BoundingBoxComponent& bb, TransformComponent&, ModelComponent&)
						{
							// visualize NavMesh
							em.RemoveComponent<TransformComponent>(e);
							em.RemoveComponent<ModelComponent>(e);
						}
					);
				}
			}
			if (ImGui::Checkbox("Visualize Portals", &m_vizPortals))
			{
				if (m_vizPortals)
				{
					constexpr Vector3 portalScale = Vector3(0.1f, 0.1f, 0.1f);
					EntityManager& em = EntityManager::Get();
					em.Collect<PortalComponent>().Do(
						[&](entity e, PortalComponent& pc)
						{
							// visualize portal
							if (!em.HasComponent<TransformComponent>(e)) em.AddComponent<TransformComponent>(e).SetPosition(pc.portal).SetScale(portalScale);
							em.AddComponent<ModelComponent>(e, AssetManager::Get().LoadShapeAsset(DOG::Shape::prism, 4));
						}
					);
				}
				else
				{
					EntityManager& em = EntityManager::Get();
					em.Collect<PortalComponent, TransformComponent, ModelComponent>().Do(
						[&](entity e, PortalComponent& pc, TransformComponent&, ModelComponent&)
						{
							// visualize portal
							em.RemoveComponent<TransformComponent>(e);
							em.RemoveComponent<ModelComponent>(e);
						}
					);
				}
			}
		}
		ImGui::End();
	}
}

bool Pathfinder::DrawPaths()
{
	return m_visualizePaths;
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
			em.AddComponent <BoundingBoxComponent>(newMesh, em.GetComponent<BoundingBoxComponent>(e));
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
			em.AddComponent <BoundingBoxComponent>(newMesh, em.GetComponent<BoundingBoxComponent>(e));
		}
	);


	// Connect the NavMeshes
	for (size_t y = 0; y < navScene.map.size(); ++y)
	{
		for (size_t z = 0; z < navScene.map[y].size(); ++z)
		{
			for (size_t x = 0; x < navScene.map[y][z].size(); ++x)
			{
				//std::cout << navScene.At(x, y, z) << " (" << x << ", " << y << ", " << z << ")" << std::endl;
				if (navScene.HasNavMesh(x, y, z))
				{
					// connect mesh to neighbors
					entity me = navScene.At(x, y, z);
					NavMeshComponent& myMesh = em.GetComponent<NavMeshComponent>(me);

					for (Step dir : {Dir::down, Dir::north, Dir::east, Dir::south, Dir::west, Dir::up})
					{
						if (navScene.HasNavMesh(x + dir.x, y + dir.y, z + dir.z))
						{
							entity other = navScene.At(x + dir.x, y + dir.y, z + dir.z);
							// if meshes not connected, create portal
							if (myMesh.Connected(me, other) == false)
							{
								// add new portal to me and other
								PortalID id = myMesh.portals.emplace_back(em.CreateEntity());
								em.GetComponent<NavMeshComponent>(other).portals.push_back(id);

								// add necessary components to portal
								Vector3 pos = em.AddComponent<PortalComponent>(id, me, other).portal;
								em.AddComponent<SceneComponent>(id, sceneType);
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
					if (pfc.path.size() == 0)
						pfc.path.push_back(pfc.goal);

					if (navScene.At(pfc.path.back()) != terminalNavMesh)
					{
						EntityManager& em = EntityManager::Get();
						
						pfc.path.clear();
						for (PortalID id : Astar(start, pfc.goal, heuristicStraightLine))
								pfc.path.push_back(em.GetComponent<PortalComponent>(id).portal);

						pfc.path.push_back(pfc.goal);
					}
					else
					{
						// just update the goal point
						pfc.path[pfc.path.size() - 1] = pfc.goal;
					}

					// remove the first checkpoint if entity is close enough
					constexpr float THRESHOLD = .05f;
					if (pfc.path.size() > 1 && Vector3::DistanceSquared(start, pfc.path[0]) > THRESHOLD)
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
					//std::cout << "A*  " << startMesh << " (" << start.x << ", " << start.y << ", " << start.z << ")" << " ~ " << terminalNavMesh << " (" << goal.x << ", " << goal.y << ", " << goal.z << ")" << std::endl;

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
						
						//std::cout << "A*  " << currentPortal->navMesh1 << " || " << currentPortal->navMesh2 << " == " << terminalNavMesh << std::endl;

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



//void Pathfinder::GenerateNavMeshes(std::vector<std::string>& map, GridCoord origin, char symbol, PortalID currentNode)
//{
//	//constexpr char printif = 'e';
//	size_t gridSizeX = map[0].size();
//	size_t gridSizeY = map.size();
//	//std::cout << "***********************************************************" << std::endl;
//	//std::cout << "origin: " << origin.str() << std::endl;
//	std::vector<GridCoord> left;
//	std::vector<GridCoord> top;
//	std::vector<GridCoord> right;
//	std::vector<GridCoord> bottom;
//
//	// map blocking 
//	GridCoord low = { -1, -1, -1 };
//	GridCoord high = { static_cast<int>(gridSizeX), static_cast<int>(gridSizeY), 1 };
//	
//	auto InNavMesh = [&](GridCoord pt)
//	{
//		for (NavMesh& mesh : m_navMeshes)
//			if (mesh.corners.Contains(pt))
//				return true;
//		return false;
//	};
//
//	// find the maximum possible lower x bound
//	GridCoord pt = origin;
//	while (low.x <= --pt.x && map[pt.y][pt.x] == ' ' && !InNavMesh(pt));
//	low.x = pt.x;
//	left.push_back(pt);
//
//	// find the maximum possible lower y bound
//	pt = origin;
//	while (low.y <= --pt.y && map[pt.y][pt.x] == ' ' && !InNavMesh(pt));
//	low.y = pt.y;
//	top.push_back(pt);
//
//	// find the minimum possible higher x bound
//	pt = origin;
//	while (++pt.x <= high.x && map[pt.y][pt.x] == ' ' && !InNavMesh(pt));
//	high.x = pt.x;
//	right.push_back(pt);
//
//	// find the minimum possible higher y bound
//	pt = origin;
//	while (++pt.y <= high.y && map[pt.y][pt.x] == ' ' && !InNavMesh(pt));
//	high.y = pt.y;
//	bottom.push_back(pt);
//
//	// check each quadrant for blocking squares (later: cubes)
//	// upper left quadrant
//	pt = origin;
//	GridCoord qlim = low;
//	while (low.y < --pt.y)
//	{
//		pt.x = origin.x;
//		while (qlim.x < --pt.x)
//			if (map[pt.y][pt.x] != ' ' || InNavMesh(pt))
//			{
//				qlim.x = pt.x;
//				left.push_back(pt);
//				top.push_back(pt);
//				break;
//			}
//	}
//	// upper right quadrant
//	pt = origin;
//	qlim = GridCoord(high.x, low.y, low.z);
//	while (low.y < --pt.y)
//	{
//		pt.x = origin.x;
//		while (++pt.x < qlim.x)
//			if (map[pt.y][pt.x] != ' ' || InNavMesh(pt))
//			{
//				qlim.x = pt.x;
//				top.push_back(pt);
//				right.push_back(pt);
//				break;
//			}
//	}
//	// lower right quadrant
//	pt = origin;
//	qlim = high;
//	while (++pt.y < high.y)
//	{
//		pt.x = origin.x;
//		while (++pt.x < qlim.x)
//			if (map[pt.y][pt.x] != ' ' || InNavMesh(pt))
//			{
//				qlim.x = pt.x;
//				right.push_back(pt);
//				bottom.push_back(pt);
//				break;
//			}
//	}
//	// lower left quadrant
//	pt = origin;
//	qlim = GridCoord(low.x, high.y, high.z);
//	while (++pt.y < high.y)
//	{
//		pt.x = origin.x;
//		while (qlim.x < --pt.x)
//			if (map[pt.y][pt.x] != ' ' || InNavMesh(pt))
//			{ 
//				qlim.x = pt.x;
//				bottom.push_back(pt);
//				left.push_back(pt);
//				break;
//			}
//	}
//	//if (symbol == printif)
//	//{
//	//	std::cout << "left:   " << std::endl;
//	//	for (GridCoord& point : left)
//	//		std::cout << "       " << point.str() << std::endl;
//	//	std::cout << "top:    " << std::endl;
//	//	for (GridCoord& point : top)
//	//		std::cout << "       " << point.str() << std::endl;
//	//	std::cout << "right:  " << std::endl;
//	//	for (GridCoord& point : right)
//	//		std::cout << "       " << point.str() << std::endl;
//	//	std::cout << "bottom: " << std::endl;
//	//	for (GridCoord& point : bottom)
//	//		std::cout << "       " << point.str() << std::endl;
//	//}
//
//	// Check all boxes defined by left, top, right and bottom,
//	// discard any box that contains one of the other points
//	// and keep the largest of them
//	Box largest;	// default initialized to 0
//	for (size_t il = 0; il < left.size(); ++il)
//		for (size_t it = 0; it < top.size(); ++it)
//			for (size_t ir = 0; ir < right.size(); ++ir)
//				for (size_t ib = 0; ib < bottom.size(); ++ib)
//				{
//					Box b = Box(left[il], top[it], right[ir], bottom[ib]);
//					if (!(b.ContainsAny(left) || b.ContainsAny(top) || b.ContainsAny(right) || b.ContainsAny(bottom)))
//						if (largest < b)
//							largest = b;
//						//else if (symbol == printif)
//						//	std::cout << b.str() << " " << b.Area() << " < " << largest.str() << " " << largest.Area() << std::endl;
//				}
//
//	auto print = [&]()
//	{
//		std::cout << "====================================================" << std::endl;
//		constexpr size_t interval = 2;
//		std::cout << " ";
//		for (size_t x = 0; x < gridSizeX; ++x)
//			if (x % interval == 0)
//				std::cout << x % 10;
//			else
//				std::cout << " ";
//		std::cout << std::endl;
//		for (size_t y = 0; y < gridSizeY; ++y)
//		{
//			if (y % interval == 0)
//				std::cout << y % 10;
//			else
//				std::cout << " ";
//			// print map elements
//			for (size_t x = 0; x < gridSizeX; ++x)
//			{
//				//if (x == origin.x && y == origin.y)
//				//	std::cout << "@";
//				//else
//				{
//					GridCoord pt(x, y);
//					char tile = map[y][x];
//					for (NavMeshID mesh = 0; mesh < m_navMeshes.size(); ++mesh)
//						if (m_navMeshes[mesh].Contains(pt))
//						{
//							tile = mesh % 2 == 0 ? '.' : '`';
//							//tile = char(mesh + 'A');
//							for (PortalID i : m_navMeshes[mesh].navNodes)
//								if (m_navNodes[i].Contains(pt))
//								{
//									//tile = '+';
//									tile = char((m_navNodes[i].iMesh1 + m_navNodes[i].iMesh2)/2 + 'a');
//									//tile = char(mesh + 'A');
//									break;
//								}
//							break;
//						}
//					std::cout << tile;
//				}
//			}
//			// elements printed
//			if (y % interval == 0)
//				std::cout << y % 10;
//			else
//				std::cout << " ";
//			std::cout << std::endl;
//		}
//		std::cout << " ";
//		for (size_t x = 0; x < gridSizeX; ++x)
//			if (x % interval == 0)
//				std::cout << x % 10;
//			else
//				std::cout << " ";
//		std::cout << std::endl;
//		std::cout << "====================================================" << std::endl;
//	};
//
//	Box outside = largest + 1;
//	if (largest.Area() > 0)
//	{
//		NavMeshID thisMesh = NewMesh(largest);
//		//std::cout << "Starting point: " << origin.str() << std::endl;
//		//std::cout << "Starting point: " << origin.str() << "\t" << "Total navmeshes: " << m_navMeshes.size() << "\t" << "Largest navmesh: " << largest.str() << std::endl;
//		//print();
//		if (currentNode != PortalID(-1))
//			ConnectMeshAndNode(thisMesh, currentNode);
//		//std::cout << "border: " << outside.str() << std::endl;
//		// expand left
//		GridCoord nxt1 = outside.low; ++nxt1.y;
//		GridCoord nxt2;
//		//std::cout << "Left " << symbol << std::endl;
//		while (nxt1.y < outside.high.y && nxt2.y < largest.high.y)
//		{
//			while (nxt1.y < outside.high.y && map[nxt1.y][nxt1.x] != ' ') ++nxt1.y;
//			nxt2 = nxt1;
//			while (nxt2.y < largest.high.y && map[nxt2.y + 1][nxt2.x] == ' ') ++nxt2.y;
//			if (nxt1.y < outside.high.y)
//			{
//				// connect any part of the border inside an existing NavMesh
//				// and generate new mesh(es) on remaining open border(s)
//				for (Box& open : ConnectToNeighborsAndReturnOpen(thisMesh, Box(nxt1, nxt2)))
//					//GenerateNavMeshes(map, open.Midpoint(), ++symbol, NewPortal(thisMesh, open));
//				{
//					GridCoord inside = open.high;
//					inside.x += 1;
//					//std::cout << "[" << symbol << "] Open left: " << Box(open.low, inside).str() << std::endl;
//					GenerateNavMeshes(map, open.Midpoint(), symbol + 1, NewPortal(thisMesh, Box(open.low, inside)));
//					//GenerateNavMeshes(map, GridCoord(nxt2.x, nxt1.y + (nxt2.y - nxt1.y) / 2, 0), ++symbol, NewPortal(thisMesh, Box(inside, nxt2)));
//				}
//				nxt1.y = nxt2.y + 1;
//			}
//		}
//		// expand down
//		nxt1.x = outside.low.x + 1; nxt1.y = outside.high.y;
//		//std::cout << "Down " << symbol << std::endl;
//		while (nxt1.x < outside.high.x && nxt2.x < largest.high.x)
//		{
//			while (nxt1.x < outside.high.x && map[nxt1.y][nxt1.x] != ' ') ++nxt1.x;
//			nxt2 = nxt1;
//			while (nxt2.x < largest.high.x && map[nxt2.y][nxt2.x + 1] == ' ') ++nxt2.x;
//			if (nxt1.x < outside.high.x)
//			{
//				// connect any part of the border inside an existing NavMesh
//				// and generate new mesh(es) on remaining open border(s)
//				for (Box& open : ConnectToNeighborsAndReturnOpen(thisMesh, Box(nxt1, nxt2)))
//					//GenerateNavMeshes(map, open.Midpoint(), ++symbol, NewPortal(thisMesh, open));
//				{
//					GridCoord inside = open.low;
//					inside.y -= 1;
//					//std::cout << "[" << symbol << "] Open down: " << Box(open.low, inside).str() << std::endl;
//					GenerateNavMeshes(map, open.Midpoint(), symbol + 1, NewPortal(thisMesh, Box(inside, open.high)));
//					//GenerateNavMeshes(map, GridCoord(nxt2.x, nxt1.y + (nxt2.y - nxt1.y) / 2, 0), ++symbol, NewPortal(thisMesh, Box(inside, nxt2)));
//				}
//				nxt1.x = nxt2.x + 1;
//			}
//		}
//		// expand right
//		nxt1 = outside.high; --nxt1.y;
//		//std::cout << "Right " << symbol << std::endl;
//		while (outside.low.y < nxt1.y && largest.low.y < nxt2.y)
//		{
//			while (outside.low.y < nxt1.y && map[nxt1.y][nxt1.x] != ' ') --nxt1.y;
//			nxt2 = nxt1;
//			while (largest.low.y < nxt2.y && map[nxt2.y - 1][nxt2.x] == ' ') --nxt2.y;
//			if (outside.low.y < nxt1.y)
//			{
//				// connect any part of the border inside an existing NavMesh
//				// and generate new mesh(es) on remaining open border(s)
//				for (Box& open : ConnectToNeighborsAndReturnOpen(thisMesh, Box(nxt2, nxt1)))
//					//GenerateNavMeshes(map, open.Midpoint(), ++symbol, NewPortal(thisMesh, open));
//				{
//					GridCoord inside = open.low;
//					inside.x -= 1;
//					//std::cout << "[" << symbol << "] Open right: " << Box(inside, open.high).str() << std::endl;
//					GenerateNavMeshes(map, open.Midpoint(), symbol + 1, NewPortal(thisMesh, Box(inside, open.high)));
//					//GenerateNavMeshes(map, GridCoord(nxt2.x, nxt1.y + (nxt2.y - nxt1.y) / 2, 0), ++symbol, NewPortal(thisMesh, Box(inside, nxt2)));
//					//std::cout << "return to " << symbol << " | " << nxt1.str() << nxt2.str() << " | " << outside.str() << " | " << largest.str() << std::endl;
//				}
//				nxt1.y = nxt2.y - 1;
//			}
//			//nxt1 = nxt2;
//		}
//		// expand up
//		nxt1.x = outside.high.x - 1; nxt1.y = outside.low.y;
//		while (outside.low.x < nxt1.x && largest.low.x < nxt2.x)
//		{
//			while (outside.low.x < nxt1.x && map[nxt1.y][nxt1.x] != ' ') --nxt1.x;
//			nxt2 = nxt1;
//			while (largest.low.x < nxt2.x && map[nxt2.y][nxt2.x - 1] == ' ') --nxt2.x;
//			if (outside.low.x < nxt1.x)
//			{
//				// connect any part of the border inside an existing NavMesh
//				// and generate new mesh(es) on remaining open border(s)
//				for (Box& open : ConnectToNeighborsAndReturnOpen(thisMesh, Box(nxt2, nxt1)))
//					//GenerateNavMeshes(map, open.Midpoint(), ++symbol, NewPortal(thisMesh, open));
//				{
//					GridCoord inside = open.high;
//					inside.y += 1;
//					//std::cout << "[" << symbol << "] Open up: " << Box(open.low, inside).str() << std::endl;
//					GenerateNavMeshes(map, open.Midpoint(), symbol + 1, NewPortal(thisMesh, Box(open.low, inside)));
//					//GenerateNavMeshes(map, GridCoord(nxt2.x, nxt1.y + (nxt2.y - nxt1.y) / 2, 0), ++symbol, NewPortal(thisMesh, Box(inside, nxt2)));
//				}
//				nxt1.x = nxt2.x - 1;
//			}
//		}
//	}
//	//else
//	//{
//	//	std::cout << "finished generating NavMeshes:" << std::endl;
//	//	print();
//	//}
//	//std::cout << "End " << symbol << std::endl;
//}
//
//
//size_t Pathfinder::FindNavMeshContaining(const Vector3 pos)
//{
//	for (size_t i = 0; i < m_navMeshes.size(); ++i)
//		if (m_navMeshes[i].Contains(pos))
//			return i;
//	// pos not found should be impossible!
//	ASSERT(false, "Pathfinder: pos does not exist in map");
//	return size_t(-1);
//}
