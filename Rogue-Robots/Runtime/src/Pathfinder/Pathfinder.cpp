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

void Pathfinder::GenerateNavMeshes(std::vector<std::string>& map, GridCoord origin, char& symbol)
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
	
	// find the maximum possible lower x bound
	GridCoord pt = origin;
	while (low.x <= --pt.x && map[pt.y][pt.x] == ' ');
	low.x = pt.x;
	left.push_back(pt);

	// find the maximum possible lower y bound
	pt = origin;
	while (low.y <= --pt.y && map[pt.y][pt.x] == ' ');
	low.y = pt.y;
	top.push_back(pt);

	// find the minimum possible higher x bound
	pt = origin;
	while (++pt.x <= high.x && map[pt.y][pt.x] == ' ');
	high.x = pt.x;
	right.push_back(pt);

	// find the minimum possible higher y bound
	pt = origin;
	while (++pt.y <= high.y && map[pt.y][pt.x] == ' ');
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
			if (map[pt.y][pt.x] != ' ')
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
	auto print = [&](Box bx)
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
			for (size_t x = 0; x < gridSizeX; ++x)
			{
				if (x == origin.x && y == origin.y)
					map[y][x] = '@';
				else if (bx.Contains(GridCoord(x, y)))
					map[y][x] = symbol;
				std::cout << map[y][x];
			}
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
	print(largest);
	Box border = largest + 1;
	if (largest.Area() > 0)
	{
		std::cout << "border: " << border.str() << std::endl;
		GridCoord nxt1 = border.low;
		GridCoord nxt2 = nxt1;
		// expand left
		while (nxt1.y < border.high.y && nxt2.y < border.high.y)
		{
			while (++nxt1.y < border.high.y && map[nxt1.y][nxt1.x] != ' ');
			nxt2 = nxt1;
			while (++nxt2.y < border.high.y && map[nxt2.y][nxt2.x] == ' ');
			if (nxt1.y < border.high.y)
				GenerateNavMeshes(map, GridCoord(nxt2.x, nxt1.y + (nxt2.y - nxt1.y) / 2, 0), ++symbol);
		}
		nxt1.x = border.low.x; nxt1.y = border.high.y;
		nxt2 = nxt1;
		// expand down
		while (nxt1.x < border.high.x && nxt2.x < border.high.x)
		{
			while (++nxt1.x < border.high.x && map[nxt1.y][nxt1.x] != ' ');
			nxt2 = nxt1;
			while (++nxt2.x < border.high.x && map[nxt2.y][nxt2.x] == ' ');
			if (nxt1.x < border.high.x)
				GenerateNavMeshes(map, GridCoord(nxt1.x + (nxt2.x - nxt1.x) / 2, nxt2.y, 0), ++symbol);
		}
		nxt1 = border.high;
		nxt2 = nxt1;
		// expand right
		while (border.low.y < nxt1.y && border.low.y < nxt2.y)
		{
			while (border.low.y < --nxt1.y && map[nxt1.y][nxt1.x] != ' ');
			nxt2 = nxt1;
			while (border.low.y < --nxt2.y && map[nxt2.y][nxt2.x] == ' ');
			if (border.low.y < nxt1.y)
				GenerateNavMeshes(map, GridCoord(nxt2.x, nxt1.y - (nxt1.y - nxt2.y) / 2, 0), ++symbol);
		}
		nxt1.x = border.high.x; nxt1.y = border.low.y;
		nxt2 = nxt1;
		// expand up
		while (border.low.x < nxt1.x && border.low.x < nxt2.x)
		{
			while (border.low.x < --nxt1.x && map[nxt1.y][nxt1.x] != ' ');
			nxt2 = nxt1;
			while (border.low.x < --nxt2.x && map[nxt2.y][nxt2.x] == ' ');
			if (border.low.x < nxt1.x)
				GenerateNavMeshes(map, GridCoord(nxt1.x - (nxt1.x - nxt2.x) / 2, nxt2.y, 0), ++symbol);
		}
	}
	else
		std::cout << "finished generating NavMeshes" << std::endl;
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

std::vector<Pathfinder::NavNode*> Pathfinder::Astar(const Vector3 start, const Vector3 goal, float(*h)(Vector3, Vector3))
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

Pathfinder::NavMesh::NavMesh(Vector3 low, Vector3 hi) : lowCorner(low), hiCorner(hi)
{
}

void Pathfinder::NavMesh::AddNavNode(NavNodeID nodeID)
{
	navNodes.push_back(nodeID);
}

bool Pathfinder::NavMesh::Contains(const Vector3 pos)
{
	// pos is part of the mesh on the lower border
	// but part of the adjoining mesh on the higher
	return lowCorner.x <= pos.x && pos.x < hiCorner.x
		&& lowCorner.y <= pos.y && pos.y < hiCorner.y
		&& lowCorner.z <= pos.z && pos.z < hiCorner.z;
}

float Pathfinder::NavMesh::CostWalk(const Vector3 enter, const Vector3 exit)
{
	return (exit - enter).Length();
}

float Pathfinder::NavMesh::CostFly(const Vector3 enter, const Vector3 exit)
{
	return (exit - enter).Length();
}

Vector3 Pathfinder::NavNode::Midpoint()
{
	return (hiCorner - lowCorner) / 2;
}
