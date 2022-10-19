#include "Pathfinder.h"
#include <limits>
#include <unordered_map>

using namespace DirectX::SimpleMath;


Pathfinder::Pathfinder() noexcept
{
	// For testing purposes
	std::vector<std::string> map1 = {
		"***********************************************",
		"*                         *                   *",
		"*               ***********                   *",
		"*      *        *                *****        *",
		"*      *        *                *****        *",
		"*      *                 *****      **        *",
		"*      ******               **       **********",
		"*           *               **********        *",
		"*           *                                 *",
		"*           ********            ******        *",
		"*               *               *    *        *",
		"*               *               *    *        *",
		"*               *                    *        *",
		"***********************************************",
	};
	size_t gridSizeX = map1[0].size();
	size_t gridSizeY = map1.size();
	size_t startX = gridSizeX / 2;
	size_t startY = gridSizeY / 2;
	while (map1[startY][startX] != ' ')
		++startY; ++startX;
	char symbol = 'a';
	GenerateNavMeshes(map1, GridCoord(startX, startY), symbol);
}

void Pathfinder::GenerateNavMeshes(std::vector<std::string>& map, GridCoord origin, char& symbol, NavNodeID currentNode)
{
	constexpr char printif = 'e';
	size_t gridSizeX = map[0].size();
	size_t gridSizeY = map.size();
	std::cout << "***********************************************************" << std::endl;
	std::cout << "origin: " << origin.str() << std::endl;
	std::vector<GridCoord> left;
	std::vector<GridCoord> top;
	std::vector<GridCoord> right;
	std::vector<GridCoord> bottom;

	// map blocking 
	GridCoord low = { -1, -1, -1 };
	GridCoord high = { static_cast<int>(gridSizeX), static_cast<int>(gridSizeY), 1 };
	
	auto notInNavMesh = [&](GridCoord pt)
	{
		for (NavMesh& mesh : m_navMeshes)
			if (mesh.corners.Contains(pt))
				return false;
		return true;
	};

	// find the maximum possible lower x bound
	GridCoord pt = origin;
	while (low.x <= --pt.x && map[pt.y][pt.x] == ' ' && notInNavMesh(pt));
	low.x = pt.x;
	left.push_back(pt);

	// find the maximum possible lower y bound
	pt = origin;
	while (low.y <= --pt.y && map[pt.y][pt.x] == ' ' && notInNavMesh(pt));
	low.y = pt.y;
	top.push_back(pt);

	// find the minimum possible higher x bound
	pt = origin;
	while (++pt.x <= high.x && map[pt.y][pt.x] == ' ' && notInNavMesh(pt));
	high.x = pt.x;
	right.push_back(pt);

	// find the minimum possible higher y bound
	pt = origin;
	while (++pt.y <= high.y && map[pt.y][pt.x] == ' ' && notInNavMesh(pt));
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
			if (map[pt.y][pt.x] != ' ')
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
			if (map[pt.y][pt.x] != ' ')
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
			if (map[pt.y][pt.x] != ' ')
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
			if (map[pt.y][pt.x] != ' ' && notInNavMesh(pt))
			{ 
				qlim.x = pt.x;
				bottom.push_back(pt);
				left.push_back(pt);
				break;
			}
	}
	if (symbol == printif)
	{
		std::cout << "left:   " << std::endl;
		for (GridCoord& point : left)
			std::cout << "       " << point.str() << std::endl;
		std::cout << "top:    " << std::endl;
		for (GridCoord& point : top)
			std::cout << "       " << point.str() << std::endl;
		std::cout << "right:  " << std::endl;
		for (GridCoord& point : right)
			std::cout << "       " << point.str() << std::endl;
		std::cout << "bottom: " << std::endl;
		for (GridCoord& point : bottom)
			std::cout << "       " << point.str() << std::endl;
	}

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
						else if (symbol == printif)
							std::cout << b.str() << " " << b.Area() << " < " << largest.str() << " " << largest.Area() << std::endl;
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
				if (x == origin.x && y == origin.y)
					std::cout << "@";
				else
				{
					char tile = map[y][x];
					for (NavMesh& mesh : m_navMeshes)
						if (mesh.Contains(GridCoord(x, y)))
						{
							tile = '~';
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
	//print(largest);
	Box outside = largest + 1;
	if (largest.Area() > 0)
	{
		NavMeshID thisMesh = NewMesh(largest);
		if (currentNode != NavNodeID(-1))
			ConnectMeshAndNode(thisMesh, currentNode);
		//std::cout << "border: " << outside.str() << std::endl;
		GridCoord nxt1 = outside.low;
		GridCoord nxt2 = nxt1;
		// expand left
		while (nxt1.y < outside.high.y && nxt2.y < outside.high.y)
		{
			while (++nxt1.y < outside.high.y && map[nxt1.y][nxt1.x] != ' ');
			nxt2 = nxt1;
			while (++nxt2.y < outside.high.y && map[nxt2.y][nxt2.x] == ' ');
			if (nxt1.y < outside.high.y)
			{
				// connect any part of the border inside an existing NavMesh
				// and generate new mesh(es) on remaining open border(s)
				for (Box& open : ConnectToNeighborsAndReturnOpen(thisMesh, Box(nxt1, nxt2)))
				{
					GridCoord inside = nxt1;
					inside.x += 1;
					GenerateNavMeshes(map, GridCoord(nxt2.x, nxt1.y + (nxt2.y - nxt1.y) / 2, 0), ++symbol, NewNode(thisMesh, Box(inside, nxt2)));
				}
			}
		}
		nxt1.x = outside.low.x; nxt1.y = outside.high.y;
		nxt2 = nxt1;
		// expand down
		while (nxt1.x < outside.high.x && nxt2.x < outside.high.x)
		{
			while (++nxt1.x < outside.high.x && map[nxt1.y][nxt1.x] != ' ');
			nxt2 = nxt1;
			while (++nxt2.x < outside.high.x && map[nxt2.y][nxt2.x] == ' ');
			if (nxt1.x < outside.high.x)
			{
				// connect any part of the border inside an existing NavMesh
				// and generate new mesh(es) on remaining open border(s)
				for (Box& open : ConnectToNeighborsAndReturnOpen(thisMesh, Box(nxt1, nxt2)))
				{
					GridCoord inside = nxt1;
					inside.y -= 1;
					GenerateNavMeshes(map, GridCoord(nxt2.x, nxt1.y + (nxt2.y - nxt1.y) / 2, 0), ++symbol, NewNode(thisMesh, Box(inside, nxt2)));
				}
			}
		}
		nxt1 = outside.high;
		nxt2 = nxt1;
		// expand right
		while (outside.low.y < nxt1.y && outside.low.y < nxt2.y)
		{
			while (outside.low.y < --nxt1.y && map[nxt1.y][nxt1.x] != ' ');
			nxt2 = nxt1;
			while (outside.low.y < --nxt2.y && map[nxt2.y][nxt2.x] == ' ');
			if (outside.low.y < nxt1.y)
			{
				// connect any part of the border inside an existing NavMesh
				// and generate new mesh(es) on remaining open border(s)
				for (Box& open : ConnectToNeighborsAndReturnOpen(thisMesh, Box(nxt1, nxt2)))
				{
					GridCoord inside = nxt1;
					inside.x -= 1;
					GenerateNavMeshes(map, GridCoord(nxt2.x, nxt1.y + (nxt2.y - nxt1.y) / 2, 0), ++symbol, NewNode(thisMesh, Box(inside, nxt2)));
				}
			}
		}
		nxt1.x = outside.high.x; nxt1.y = outside.low.y;
		nxt2 = nxt1;
		// expand up
		while (outside.low.x < nxt1.x && outside.low.x < nxt2.x)
		{
			while (outside.low.x < --nxt1.x && map[nxt1.y][nxt1.x] != ' ');
			nxt2 = nxt1;
			while (outside.low.x < --nxt2.x && map[nxt2.y][nxt2.x] == ' ');
			if (outside.low.x < nxt1.x)
			{
				// connect any part of the border inside an existing NavMesh
				// and generate new mesh(es) on remaining open border(s)
				for (Box& open : ConnectToNeighborsAndReturnOpen(thisMesh, Box(nxt1, nxt2)))
				{
					GridCoord inside = nxt1;
					inside.y += 1;
					GenerateNavMeshes(map, GridCoord(nxt2.x, nxt1.y + (nxt2.y - nxt1.y) / 2, 0), ++symbol, NewNode(thisMesh, Box(inside, nxt2)));
				}
			}
		}
	}
	else
	{
		std::cout << "finished generating NavMeshes:" << std::endl;
		print();
	}
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

std::vector<NavNode*> Pathfinder::Astar(const Vector3 start, const Vector3 goal, float(*h)(Vector3, Vector3))
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
	NavNode entry = NavNode(start, FindNavMeshContaining(start));
	constexpr NavNodeID startPoint = NavNodeID(-1);
	NavMeshID terminalNavMesh = FindNavMeshContaining(goal);

	std::vector<NavNodeID> openSet = { startPoint };
	std::unordered_map<NavNodeID, NavNodeID> cameFrom;
	std::unordered_map<NavNodeID, MaxFloat> fScore;
	fScore[startPoint] = h(start, goal);
	std::unordered_map<NavNodeID, MaxFloat> gScore;
	gScore[startPoint] = 0;

	// lambda: pop element with lowest fScore from minheap
	auto popOpenSet = [&]()
	{
		NavNodeID get = openSet[0];
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
				//NavNodeID toSwap = openSet[i];
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
	auto pushOpenSet = [&](NavNodeID iNode)
	{
		openSet.push_back(iNode);
		// percolate up
		size_t i = openSet.size() - 1;
		size_t p = (i + (i % 2)) / 2 - 1;
		while (p >= 0 && fScore[p] > fScore[i])
		{
			//NavNodeID toSwap = openSet[p];
			//openSet[p] = openSet[i];
			//openSet[i] = toSwap;
			std::swap(openSet[p], openSet[i]);
			i = p;
			p = (i + (i % 2)) / 2 - 1;
		}
	};
	// returns true if current node is connected to the NavMesh containing the goal
	auto leadsToGoal = [&](NavNodeID current)
	{
		if (current == startPoint)
			return entry.iMesh1 == terminalNavMesh;
		return m_navNodes[current].iMesh1 == terminalNavMesh || m_navNodes[current].iMesh2 == terminalNavMesh;
	};
	// returns true if openSet does not contain neighbor
	auto notInOpenSet = [&](NavNodeID neighbor)
	{
		for (NavNodeID node : openSet)
			if (node == neighbor)
				return false;
		return true;
	};
	auto getNeighbors = [&](NavNodeID current)
	{
		if (current == startPoint)
			return m_navMeshes[entry.iMesh1].navNodes;
		std::vector<NavNodeID> neighbors;
		for (NavMeshID i : { m_navNodes[current].iMesh1, m_navNodes[current].iMesh2 })
			for (NavNodeID iNode : m_navMeshes[i].navNodes)
				if (iNode != current)
					neighbors.push_back(iNode);
		return neighbors;
	};
	// lambda: d - distance between nodes
	auto d = [&](NavNodeID current, NavNodeID neighbor)
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
	auto reconstructPath = [&](NavNodeID iNode)
	{
		std::vector<NavNode*> path;
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
		NavNodeID current = popOpenSet();
		if (leadsToGoal(current))
			return reconstructPath(current);

		for (NavNodeID neighbor : getNeighbors(current))
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
	return std::vector<NavNode*>();
}


Pathfinder::NavMeshID Pathfinder::NewMesh(Box extents)
{
	NavMeshID id = m_navMeshes.size();
	m_navMeshes.emplace_back(NavMesh(extents));
	return id;
}

Pathfinder::NavNodeID Pathfinder::NewNode(NavMeshID mesh, Box node)
{
	NavNodeID id = m_navNodes.size();
	m_navNodes.emplace_back(NavNode(node, mesh));
	return id;
}

void Pathfinder::ConnectMeshAndNode(NavMeshID mesh, NavNodeID node)
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
			ConnectMeshAndNode(existing, NewNode(mesh, intersection));
			std::vector<Box> newOpen;
			for (Box segment : open)
			{
				if (segment.Contains(intersection))
				{
					newOpen.push_back(Box(segment.low, intersection.low));
					newOpen.push_back(Box(intersection.high, segment.high));
				}
				else
					newOpen.push_back(segment);
			}
			open = newOpen;
		}
	}
	return open;
}


