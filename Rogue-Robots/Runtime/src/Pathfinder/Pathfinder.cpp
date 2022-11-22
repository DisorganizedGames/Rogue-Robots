#include "Pathfinder.h"
#include <limits>
#include <unordered_map>
#include "Game/PcgLevelLoader.h"

using namespace DOG;
using namespace DirectX::SimpleMath;

Pathfinder Pathfinder::s_instance;

Pathfinder::Pathfinder() noexcept
{
	//// For testing purposes
	//std::vector<std::string> map1 = {
	//	"***********************************************",
	//	"*                         *                   *",
	//	"*               ***********                   *",
	//	"*      *        *                *****        *",
	//	"*      *        *                *****        *",
	//	"*      *                 *****      **        *",
	//	"*      ******               **       **********",
	//	"*           *               **********        *",
	//	"*           *                                 *",
	//	"*           ********            ******        *",
	//	"*               *               *    *        *",
	//	"*               *               *    *        *",
	//	"*               *                    *        *",
	//	"***********************************************",
	//};
	//size_t gridSizeX = map1[0].size();
	//size_t gridSizeY = map1.size();
	//size_t startX = gridSizeX / 2;
	//size_t startY = gridSizeY / 2;
	//while (map1[startY][startX] != ' ')
	//	++startY; ++startX;
	//char symbol = 'A';
	//GenerateNavMeshes(map1, GridCoord(startX, startY), symbol);
	//std::vector<bool> visited;
	//visited.resize(m_navMeshes.size(), false);

	////for (NavMesh& mesh : m_navMeshes)
	////	print(mesh);
	//print(0, visited, map1);
}

//void Pathfinder::print(NavMeshID mesh, std::vector<bool>& visited, std::vector<std::string>& map)
//{
//	if (visited[mesh])
//		return;
//	size_t gridSizeX = map[0].size();
//	size_t gridSizeY = map.size();
//	visited[mesh] = true;
//	std::cout << "====================================================" << std::endl;
//	constexpr size_t interval = 2;
//	std::cout << " ";
//	for (size_t x = 0; x < gridSizeX; ++x)
//		if (x % interval == 0)
//			std::cout << x % 10;
//		else
//			std::cout << " ";
//	std::cout << std::endl;
//	for (size_t y = 0; y < gridSizeY; ++y)
//	{
//		if (y % interval == 0)
//			std::cout << y % 10;
//		else
//			std::cout << " ";
//		// print map elements
//		for (size_t x = 0; x < gridSizeX; ++x)
//		{
//			//if (x == origin.x && y == origin.y)
//			//	std::cout << "@";
//			//else
//			{
//				GridCoord pt(x, y);
//				char tile = map[y][x];
//				if (m_navMeshes[mesh].Contains(pt))
//				{
//					tile = '~';
//					//tile = char(mesh + 'A');
//					for (PortalID i : m_navMeshes[mesh].navNodes)
//						if (m_navNodes[i].Contains(pt))
//						{
//							tile = '+';
//							//tile = char((m_navNodes[i].iMesh1 + m_navNodes[i].iMesh2) / 2 + 'a');
//							//tile = char(mesh + 'A');
//							break;
//						}
//				}
//				std::cout << tile;
//			}
//		}
//		// elements printed
//		if (y % interval == 0)
//			std::cout << y % 10;
//		else
//			std::cout << " ";
//		std::cout << std::endl;
//	}
//	std::cout << " ";
//	for (size_t x = 0; x < gridSizeX; ++x)
//		if (x % interval == 0)
//			std::cout << x % 10;
//		else
//			std::cout << " ";
//	std::cout << std::endl;
//	std::cout << "====================================================" << std::endl;
//	for (PortalID nxt : m_navMeshes[mesh].navNodes)
//	{
//		print(m_navNodes[nxt].iMesh1, visited, map);
//		print(m_navNodes[nxt].iMesh1, visited, map);
//	}
//};

void Pathfinder::BuildNavScene(SceneComponent::Type sceneType)
{
	constexpr unsigned char solidBlock = 178;
	constexpr unsigned char emptySpace = 176;

	constexpr float blockDim = pcgBlock::DIMENSION;

	std::vector<std::vector<std::vector<char>>> map;
	std::cout << "Scene type: " << (int)sceneType << std::endl;
	EntityManager& em = EntityManager::Get();

	em.Collect<ModularBlockComponent, TransformComponent>().Do(
		[&](entity e, ModularBlockComponent&, TransformComponent& trans)
		{		
			// find x, y and z coordinates
			Vector3 pos = trans.GetPosition();
			size_t x = static_cast<size_t>(pos.x / blockDim);
			size_t y = static_cast<size_t>(pos.y / blockDim);
			size_t z = static_cast<size_t>(pos.z / blockDim);

			// expand map if necessary
			while (!(z < map.size()))
				map.emplace_back(std::vector<std::vector<char>>());
			while (!(y < map[z].size()))
				map[z].emplace_back(std::vector<char>());
			while (!(x < map[z][y].size()))
				map[z][y].emplace_back(' ');

			// draw block
			map[z][y][x] = solidBlock;
		});

	em.Collect<EmptySpaceComponent>().Do(
		[&](entity e, EmptySpaceComponent& empty)
		{
			// find x, y and z coordinates (empty spaces are always contained inside solid blocks)
			size_t x = static_cast<size_t>(empty.pos.x / blockDim);
			size_t y = static_cast<size_t>(empty.pos.y / blockDim);
			size_t z = static_cast<size_t>(empty.pos.z / blockDim);

			// draw empty space
			map[z][y][x] = emptySpace;
		}
	);

	// print map sliced in z-levels
	for (size_t z = 0; z < map.size(); ++z)
	{
		std::cout << "==================================================" << std::endl;
		std::cout << "                   Level " << z << std::endl;
		std::cout << "==================================================" << std::endl << std::endl;
		for (size_t y = 0; y < map[z].size(); ++y)
		{
			std::cout << "   ";
			for (size_t x = 0; x < map[z][y].size(); ++x)
			{
				std::cout << map[z][y][x];
			}
			std::cout << std::endl;
		}
		std::cout << std::endl;
	}
}

void Pathfinder::GenerateNavMeshes(std::vector<std::string>& map, GridCoord origin, char symbol, PortalID currentNode)
{
	//constexpr char printif = 'e';
	size_t gridSizeX = map[0].size();
	size_t gridSizeY = map.size();
	//std::cout << "***********************************************************" << std::endl;
	//std::cout << "origin: " << origin.str() << std::endl;
	std::vector<GridCoord> left;
	std::vector<GridCoord> top;
	std::vector<GridCoord> right;
	std::vector<GridCoord> bottom;

	// map blocking 
	GridCoord low = { -1, -1, -1 };
	GridCoord high = { static_cast<int>(gridSizeX), static_cast<int>(gridSizeY), 1 };
	
	auto InNavMesh = [&](GridCoord pt)
	{
		for (NavMesh& mesh : m_navMeshes)
			if (mesh.corners.Contains(pt))
				return true;
		return false;
	};

	// find the maximum possible lower x bound
	GridCoord pt = origin;
	while (low.x <= --pt.x && map[pt.y][pt.x] == ' ' && !InNavMesh(pt));
	low.x = pt.x;
	left.push_back(pt);

	// find the maximum possible lower y bound
	pt = origin;
	while (low.y <= --pt.y && map[pt.y][pt.x] == ' ' && !InNavMesh(pt));
	low.y = pt.y;
	top.push_back(pt);

	// find the minimum possible higher x bound
	pt = origin;
	while (++pt.x <= high.x && map[pt.y][pt.x] == ' ' && !InNavMesh(pt));
	high.x = pt.x;
	right.push_back(pt);

	// find the minimum possible higher y bound
	pt = origin;
	while (++pt.y <= high.y && map[pt.y][pt.x] == ' ' && !InNavMesh(pt));
	high.y = pt.y;
	bottom.push_back(pt);

	// check each quadrant for blocking squares (later: cubes)
	// upper left quadrant
	pt = origin;
	GridCoord qlim = low;
	while (low.y < --pt.y)
	{
		pt.x = origin.x;
		while (qlim.x < --pt.x)
			if (map[pt.y][pt.x] != ' ' || InNavMesh(pt))
			{
				qlim.x = pt.x;
				left.push_back(pt);
				top.push_back(pt);
				break;
			}
	}
	// upper right quadrant
	pt = origin;
	qlim = GridCoord(high.x, low.y, low.z);
	while (low.y < --pt.y)
	{
		pt.x = origin.x;
		while (++pt.x < qlim.x)
			if (map[pt.y][pt.x] != ' ' || InNavMesh(pt))
			{
				qlim.x = pt.x;
				top.push_back(pt);
				right.push_back(pt);
				break;
			}
	}
	// lower right quadrant
	pt = origin;
	qlim = high;
	while (++pt.y < high.y)
	{
		pt.x = origin.x;
		while (++pt.x < qlim.x)
			if (map[pt.y][pt.x] != ' ' || InNavMesh(pt))
			{
				qlim.x = pt.x;
				right.push_back(pt);
				bottom.push_back(pt);
				break;
			}
	}
	// lower left quadrant
	pt = origin;
	qlim = GridCoord(low.x, high.y, high.z);
	while (++pt.y < high.y)
	{
		pt.x = origin.x;
		while (qlim.x < --pt.x)
			if (map[pt.y][pt.x] != ' ' || InNavMesh(pt))
			{ 
				qlim.x = pt.x;
				bottom.push_back(pt);
				left.push_back(pt);
				break;
			}
	}
	//if (symbol == printif)
	//{
	//	std::cout << "left:   " << std::endl;
	//	for (GridCoord& point : left)
	//		std::cout << "       " << point.str() << std::endl;
	//	std::cout << "top:    " << std::endl;
	//	for (GridCoord& point : top)
	//		std::cout << "       " << point.str() << std::endl;
	//	std::cout << "right:  " << std::endl;
	//	for (GridCoord& point : right)
	//		std::cout << "       " << point.str() << std::endl;
	//	std::cout << "bottom: " << std::endl;
	//	for (GridCoord& point : bottom)
	//		std::cout << "       " << point.str() << std::endl;
	//}

	// Check all boxes defined by left, top, right and bottom,
	// discard any box that contains one of the other points
	// and keep the largest of them
	Box largest;	// default initialized to 0
	for (size_t il = 0; il < left.size(); ++il)
		for (size_t it = 0; it < top.size(); ++it)
			for (size_t ir = 0; ir < right.size(); ++ir)
				for (size_t ib = 0; ib < bottom.size(); ++ib)
				{
					Box b = Box(left[il], top[it], right[ir], bottom[ib]);
					if (!(b.ContainsAny(left) || b.ContainsAny(top) || b.ContainsAny(right) || b.ContainsAny(bottom)))
						if (largest < b)
							largest = b;
						//else if (symbol == printif)
						//	std::cout << b.str() << " " << b.Area() << " < " << largest.str() << " " << largest.Area() << std::endl;
				}

	auto print = [&]()
	{
		std::cout << "====================================================" << std::endl;
		constexpr size_t interval = 2;
		std::cout << " ";
		for (size_t x = 0; x < gridSizeX; ++x)
			if (x % interval == 0)
				std::cout << x % 10;
			else
				std::cout << " ";
		std::cout << std::endl;
		for (size_t y = 0; y < gridSizeY; ++y)
		{
			if (y % interval == 0)
				std::cout << y % 10;
			else
				std::cout << " ";
			// print map elements
			for (size_t x = 0; x < gridSizeX; ++x)
			{
				//if (x == origin.x && y == origin.y)
				//	std::cout << "@";
				//else
				{
					GridCoord pt(x, y);
					char tile = map[y][x];
					for (NavMeshID mesh = 0; mesh < m_navMeshes.size(); ++mesh)
						if (m_navMeshes[mesh].Contains(pt))
						{
							tile = mesh % 2 == 0 ? '.' : '`';
							//tile = char(mesh + 'A');
							for (PortalID i : m_navMeshes[mesh].navNodes)
								if (m_navNodes[i].Contains(pt))
								{
									//tile = '+';
									tile = char((m_navNodes[i].iMesh1 + m_navNodes[i].iMesh2)/2 + 'a');
									//tile = char(mesh + 'A');
									break;
								}
							break;
						}
					std::cout << tile;
				}
			}
			// elements printed
			if (y % interval == 0)
				std::cout << y % 10;
			else
				std::cout << " ";
			std::cout << std::endl;
		}
		std::cout << " ";
		for (size_t x = 0; x < gridSizeX; ++x)
			if (x % interval == 0)
				std::cout << x % 10;
			else
				std::cout << " ";
		std::cout << std::endl;
		std::cout << "====================================================" << std::endl;
	};

	Box outside = largest + 1;
	if (largest.Area() > 0)
	{
		NavMeshID thisMesh = NewMesh(largest);
		//std::cout << "Starting point: " << origin.str() << std::endl;
		//std::cout << "Starting point: " << origin.str() << "\t" << "Total navmeshes: " << m_navMeshes.size() << "\t" << "Largest navmesh: " << largest.str() << std::endl;
		//print();
		if (currentNode != PortalID(-1))
			ConnectMeshAndNode(thisMesh, currentNode);
		//std::cout << "border: " << outside.str() << std::endl;
		// expand left
		GridCoord nxt1 = outside.low; ++nxt1.y;
		GridCoord nxt2;
		//std::cout << "Left " << symbol << std::endl;
		while (nxt1.y < outside.high.y && nxt2.y < largest.high.y)
		{
			while (nxt1.y < outside.high.y && map[nxt1.y][nxt1.x] != ' ') ++nxt1.y;
			nxt2 = nxt1;
			while (nxt2.y < largest.high.y && map[nxt2.y + 1][nxt2.x] == ' ') ++nxt2.y;
			if (nxt1.y < outside.high.y)
			{
				// connect any part of the border inside an existing NavMesh
				// and generate new mesh(es) on remaining open border(s)
				for (Box& open : ConnectToNeighborsAndReturnOpen(thisMesh, Box(nxt1, nxt2)))
					//GenerateNavMeshes(map, open.Midpoint(), ++symbol, NewPortal(thisMesh, open));
				{
					GridCoord inside = open.high;
					inside.x += 1;
					//std::cout << "[" << symbol << "] Open left: " << Box(open.low, inside).str() << std::endl;
					GenerateNavMeshes(map, open.Midpoint(), symbol + 1, NewPortal(thisMesh, Box(open.low, inside)));
					//GenerateNavMeshes(map, GridCoord(nxt2.x, nxt1.y + (nxt2.y - nxt1.y) / 2, 0), ++symbol, NewPortal(thisMesh, Box(inside, nxt2)));
				}
				nxt1.y = nxt2.y + 1;
			}
		}
		// expand down
		nxt1.x = outside.low.x + 1; nxt1.y = outside.high.y;
		//std::cout << "Down " << symbol << std::endl;
		while (nxt1.x < outside.high.x && nxt2.x < largest.high.x)
		{
			while (nxt1.x < outside.high.x && map[nxt1.y][nxt1.x] != ' ') ++nxt1.x;
			nxt2 = nxt1;
			while (nxt2.x < largest.high.x && map[nxt2.y][nxt2.x + 1] == ' ') ++nxt2.x;
			if (nxt1.x < outside.high.x)
			{
				// connect any part of the border inside an existing NavMesh
				// and generate new mesh(es) on remaining open border(s)
				for (Box& open : ConnectToNeighborsAndReturnOpen(thisMesh, Box(nxt1, nxt2)))
					//GenerateNavMeshes(map, open.Midpoint(), ++symbol, NewPortal(thisMesh, open));
				{
					GridCoord inside = open.low;
					inside.y -= 1;
					//std::cout << "[" << symbol << "] Open down: " << Box(open.low, inside).str() << std::endl;
					GenerateNavMeshes(map, open.Midpoint(), symbol + 1, NewPortal(thisMesh, Box(inside, open.high)));
					//GenerateNavMeshes(map, GridCoord(nxt2.x, nxt1.y + (nxt2.y - nxt1.y) / 2, 0), ++symbol, NewPortal(thisMesh, Box(inside, nxt2)));
				}
				nxt1.x = nxt2.x + 1;
			}
		}
		// expand right
		nxt1 = outside.high; --nxt1.y;
		//std::cout << "Right " << symbol << std::endl;
		while (outside.low.y < nxt1.y && largest.low.y < nxt2.y)
		{
			while (outside.low.y < nxt1.y && map[nxt1.y][nxt1.x] != ' ') --nxt1.y;
			nxt2 = nxt1;
			while (largest.low.y < nxt2.y && map[nxt2.y - 1][nxt2.x] == ' ') --nxt2.y;
			if (outside.low.y < nxt1.y)
			{
				// connect any part of the border inside an existing NavMesh
				// and generate new mesh(es) on remaining open border(s)
				for (Box& open : ConnectToNeighborsAndReturnOpen(thisMesh, Box(nxt2, nxt1)))
					//GenerateNavMeshes(map, open.Midpoint(), ++symbol, NewPortal(thisMesh, open));
				{
					GridCoord inside = open.low;
					inside.x -= 1;
					//std::cout << "[" << symbol << "] Open right: " << Box(inside, open.high).str() << std::endl;
					GenerateNavMeshes(map, open.Midpoint(), symbol + 1, NewPortal(thisMesh, Box(inside, open.high)));
					//GenerateNavMeshes(map, GridCoord(nxt2.x, nxt1.y + (nxt2.y - nxt1.y) / 2, 0), ++symbol, NewPortal(thisMesh, Box(inside, nxt2)));
					//std::cout << "return to " << symbol << " | " << nxt1.str() << nxt2.str() << " | " << outside.str() << " | " << largest.str() << std::endl;
				}
				nxt1.y = nxt2.y - 1;
			}
			//nxt1 = nxt2;
		}
		// expand up
		nxt1.x = outside.high.x - 1; nxt1.y = outside.low.y;
		while (outside.low.x < nxt1.x && largest.low.x < nxt2.x)
		{
			while (outside.low.x < nxt1.x && map[nxt1.y][nxt1.x] != ' ') --nxt1.x;
			nxt2 = nxt1;
			while (largest.low.x < nxt2.x && map[nxt2.y][nxt2.x - 1] == ' ') --nxt2.x;
			if (outside.low.x < nxt1.x)
			{
				// connect any part of the border inside an existing NavMesh
				// and generate new mesh(es) on remaining open border(s)
				for (Box& open : ConnectToNeighborsAndReturnOpen(thisMesh, Box(nxt2, nxt1)))
					//GenerateNavMeshes(map, open.Midpoint(), ++symbol, NewPortal(thisMesh, open));
				{
					GridCoord inside = open.high;
					inside.y += 1;
					//std::cout << "[" << symbol << "] Open up: " << Box(open.low, inside).str() << std::endl;
					GenerateNavMeshes(map, open.Midpoint(), symbol + 1, NewPortal(thisMesh, Box(open.low, inside)));
					//GenerateNavMeshes(map, GridCoord(nxt2.x, nxt1.y + (nxt2.y - nxt1.y) / 2, 0), ++symbol, NewPortal(thisMesh, Box(inside, nxt2)));
				}
				nxt1.x = nxt2.x - 1;
			}
		}
	}
	//else
	//{
	//	std::cout << "finished generating NavMeshes:" << std::endl;
	//	print();
	//}
	//std::cout << "End " << symbol << std::endl;
}


size_t Pathfinder::FindNavMeshContaining(const Vector3 pos)
{
	for (size_t i = 0; i < m_navMeshes.size(); ++i)
		if (m_navMeshes[i].Contains(pos))
			return i;
	// pos not found should be impossible!
	ASSERT(false, "Pathfinder: pos does not exist in map");
	return size_t(-1);
}

float Pathfinder::heuristicStraightLine(Vector3 start, Vector3 goal)
{
	return (goal - start).Length();
}

std::vector<Portal*> Pathfinder::Astar(const Vector3 start, const Vector3 goal, float(*h)(Vector3, Vector3))
{
	struct MaxFloat
	{	// float wrapper that has default value infinity
		float score;
		MaxFloat() : score(std::numeric_limits<float>::infinity()) {}
		void operator=(const float other) { score = other; }
		float operator=(const MaxFloat& other) { return other.score; }
		float operator+(const float other) { return this->score + other; }
		bool operator<(const float other) { return this->score < other; }
		bool operator<(const MaxFloat& other) { return this->score < other.score; }
		bool operator>(const MaxFloat& other) { return this->score > other.score; }
		bool operator>(const float other) { return this->score > other; }
	};
	// Define the entry and terminal nodes
	// Opportunity to optimize:
	// if FindNavMeshContaining(start) == FindNavMeshContaining(goal) return empty path
	Portal entry = Portal(start, FindNavMeshContaining(start));
	constexpr PortalID startPoint = MAX_ID;
	NavMeshID terminalNavMesh = FindNavMeshContaining(goal);

	std::vector<PortalID> openSet = { startPoint };
	std::unordered_map<PortalID, PortalID> cameFrom;
	std::unordered_map<PortalID, MaxFloat> fScore;
	fScore[startPoint] = h(start, goal);
	std::unordered_map<PortalID, MaxFloat> gScore;
	gScore[startPoint] = 0;

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
				if (fScore[right] < fScore[comp])
					comp = right;
			if (fScore[comp] < fScore[i])
			{
				//PortalID toSwap = openSet[i];
				//openSet[i] = openSet[comp];
				//openSet[comp] = toSwap;
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
	auto pushOpenSet = [&](PortalID iNode)
	{
		openSet.push_back(iNode);
		// percolate up
		size_t i = openSet.size() - 1;
		size_t p = (i + (i % 2)) / 2 - 1;
		while (p >= 0 && fScore[p] > fScore[i])
		{
			//PortalID toSwap = openSet[p];
			//openSet[p] = openSet[i];
			//openSet[i] = toSwap;
			std::swap(openSet[p], openSet[i]);
			i = p;
			p = (i + (i % 2)) / 2 - 1;
		}
	};
	// returns true if current node is connected to the NavMesh containing the goal
	auto leadsToGoal = [&](PortalID current)
	{
		if (current == startPoint)
			return entry.iMesh1 == terminalNavMesh;
		return m_navNodes[current].iMesh1 == terminalNavMesh || m_navNodes[current].iMesh2 == terminalNavMesh;
	};
	// returns true if openSet does not contain neighbor
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
			return m_navMeshes[entry.iMesh1].navNodes;
		std::vector<PortalID> neighbors;
		for (NavMeshID i : { m_navNodes[current].iMesh1, m_navNodes[current].iMesh2 })
			for (PortalID iNode : m_navMeshes[i].navNodes)
				if (iNode != current)
					neighbors.push_back(iNode);
		return neighbors;
	};
	// lambda: d - distance between nodes
	auto d = [&](PortalID current, PortalID neighbor)
	{
		NavMeshID iNavMesh;
		// get the first NavMesh of current
		if (current == startPoint)
			iNavMesh = entry.iMesh1;
		else
			iNavMesh = m_navNodes[current].iMesh1;
		// compare with first NavMesh of neigbor
		if (neighbor == startPoint)
			if (iNavMesh != entry.iMesh1)
				// only one can have a single connection thus current has two
				iNavMesh = m_navNodes[current].iMesh2;
		else
			if (iNavMesh != m_navNodes[neighbor].iMesh1)
				// only one can have a single connection thus neighbor has two
				iNavMesh = m_navNodes[neighbor].iMesh2;
		// since the first two NavMeshes are different iNavMesh must now contain the righ reference
		return m_navMeshes[iNavMesh].CostWalk(m_navNodes[current].Midpoint(), m_navNodes[neighbor].Midpoint());
	};
	auto reconstructPath = [&](PortalID iNode)
	{
		std::vector<Portal*> path;
		while (cameFrom.contains(iNode))
		{
			if (iNode != startPoint)
				// exclude starting point from path
				path.push_back(&m_navNodes[iNode]);
			iNode = cameFrom[iNode];
		}
		// reverse path
		std::reverse(path.begin(), path.end());
		return path;
	};

	// A* implementation
	while (!openSet.empty())
	{
		PortalID current = popOpenSet();
		if (leadsToGoal(current))
			return reconstructPath(current);

		for (PortalID neighbor : getNeighbors(current))
		{
			float tentativeGScore = gScore[current] + d(current, neighbor);
			if (gScore[neighbor] > tentativeGScore)
			{
				cameFrom[neighbor] = current;
				gScore[neighbor] = tentativeGScore;
				fScore[neighbor] = tentativeGScore + h(m_navNodes[neighbor].Midpoint(), goal);
				if (notInOpenSet(neighbor))
					pushOpenSet(neighbor);
			}
		}
	}
	// no path found, returning empty vector
	return std::vector<Portal*>();
}


Pathfinder::NavMeshID Pathfinder::NewMesh(Box extents)
{
	NavMeshID id = m_navMeshes.size();
	m_navMeshes.emplace_back(NavMesh(extents));
	return id;
}

Pathfinder::PortalID Pathfinder::NewPortal(NavMeshID mesh, Box node)
{
	PortalID id = m_navNodes.size();
	m_navNodes.emplace_back(Portal(node, mesh));
	return id;
}

void Pathfinder::ConnectMeshAndNode(NavMeshID mesh, PortalID node)
{
	m_navNodes[node].AddNavMesh(mesh);
	m_navMeshes[mesh].AddNavNode(node);
}

std::vector<Box> Pathfinder::ConnectToNeighborsAndReturnOpen(NavMeshID mesh, Box border)
{
	std::vector<Box> open{border};
	for (NavMeshID existing = 0; existing < m_navMeshes.size(); ++existing)
	{
		Box intersection = m_navMeshes[existing].corners.Intersection(border);
		if (intersection.Area() > 0)
		{
			ConnectMeshAndNode(existing, NewPortal(mesh, intersection));
			std::vector<Box> newOpen;
			for (Box& segment : open)
			{
				if (segment.Contains(intersection))
				{
					//std::cout << "Found intersection: " << intersection.str() << " on border " << border.str() << std::endl;
					Box lower(segment.low, GridCoord(std::min(intersection.low.x, segment.high.x), std::min(intersection.low.y, segment.high.y)));
					if (lower.RealArea() > 0)
						newOpen.push_back(lower);
					Box higher(GridCoord(std::max(intersection.high.x, segment.low.x), std::max(intersection.high.y, segment.low.y)), segment.high);
					if (higher.RealArea() > 0)
						newOpen.push_back(higher);
				}
				else
					newOpen.push_back(segment);
			}
			open = newOpen;
		}
	}
	//std::cout << "Open borders [" << open.size() << "]:\t";
	//for (Box& b : open)
	//	std::cout << b.str() << "\t";
	//std::cout << std::endl;
	return open;
}


