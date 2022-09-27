#include "Pathfinder.h"
#include <limits>
#include <unordered_map>

using namespace DirectX::SimpleMath;


Pathfinder::Pathfinder() noexcept
	: Layer("Pathfinder")
{
}

void Pathfinder::OnAttach()
{
	//...
}

void Pathfinder::OnDetach()
{
	//...
}

void Pathfinder::OnUpdate()
{
	//...
}

void Pathfinder::OnRender()
{
	//...
}

//Place-holder example on how to use event system:
void Pathfinder::OnEvent(DOG::IEvent& event)
{
	using namespace DOG;
	switch (event.GetEventType())
	{
	case EventType::LeftMouseButtonPressedEvent:
	{
		auto [x, y] = EVENT(LeftMouseButtonPressedEvent).coordinates;
		std::cout << GetName() << " received event: Left MB clicked [x,y] = [" << x << "," << y << "]\n";
		break;
	}
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

std::vector<Pathfinder::TransitPlane*> Pathfinder::Astar(const Vector3 start, const Vector3 goal, float(*h)(Vector3, Vector3))
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
	TransitPlane entry = TransitPlane(start, FindNavMeshContaining(start));
	constexpr size_t startPoint = size_t(-1);
	size_t terminalNavMesh = FindNavMeshContaining(goal);

	std::vector<size_t> openSet = { startPoint };
	std::unordered_map<size_t, size_t> cameFrom;
	std::unordered_map<size_t, MaxFloat> fScore;
	fScore[startPoint] = h(start, goal);
	std::unordered_map<size_t, MaxFloat> gScore;
	gScore[startPoint] = 0;

	// lambda: pop element with lowest fScore from minheap
	auto popOpenSet = [&]()
	{
		size_t get = openSet[0];
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
				size_t toSwap = openSet[i];
				openSet[i] = openSet[comp];
				openSet[comp] = toSwap;
				i = comp;
				comp = i * 2 + 1;
			}
			else
				comp = openSet.size();
		}
		return get;
	};
	// lambda: push new element onto minheap
	auto pushOpenSet = [&](size_t iNode)
	{
		openSet.push_back(iNode);
		// percolate up
		size_t i = openSet.size() - 1;
		size_t p = (i + (i % 2)) / 2 - 1;
		while (p >= 0 && fScore[p] > fScore[i])
		{
			size_t toSwap = openSet[p];
			openSet[p] = openSet[i];
			openSet[i] = toSwap;
			i = p;
			p = (i + (i % 2)) / 2 - 1;
		}
	};
	// returns true if current node is connected to the NavMesh containing the goal
	auto leadsToGoal = [&](size_t current)
	{
		if (current == startPoint)
			return entry.iMesh1 == terminalNavMesh;
		return m_transits[current].iMesh1 == terminalNavMesh || m_transits[current].iMesh2 == terminalNavMesh;
	};
	// returns true if openSet does not contain neighbor
	auto notInOpenSet = [&](size_t neighbor)
	{
		for (size_t node : openSet)
			if (node == neighbor)
				return false;
		return true;
	};
	auto getNeighbors = [&](size_t current)
	{
		if (current == startPoint)
			return m_navMeshes[entry.iMesh1].iTransits;
		std::vector<size_t> neighbors;
		for (size_t i : { m_transits[current].iMesh1, m_transits[current].iMesh2 })
			for (size_t iNode : m_navMeshes[i].iTransits)
				if (iNode != current)
					neighbors.push_back(iNode);
		return neighbors;
	};
	// lambda: d - distance between nodes
	auto d = [&](size_t current, size_t neighbor)
	{
		size_t iNavMesh;
		// get the first NavMesh of current
		if (current == startPoint)
			iNavMesh = entry.iMesh1;
		else
			iNavMesh = m_transits[current].iMesh1;
		// compare with first NavMesh of neigbor
		if (neighbor == startPoint)
			if (iNavMesh != entry.iMesh1)
				// only one can have a single connection thus current has two
				iNavMesh = m_transits[current].iMesh2;
		else
			if (iNavMesh != m_transits[neighbor].iMesh1)
				// only one can have a single connection thus neighbor has two
				iNavMesh = m_transits[neighbor].iMesh2;
		// since the first two NavMeshes are different iNavMesh must now contain the righ reference
		return m_navMeshes[iNavMesh].CostWalk(m_transits[current].Midpoint(), m_transits[neighbor].Midpoint());
	};
	auto reconstructPath = [&](size_t iNode)
	{
		std::vector<TransitPlane*> path;
		while (cameFrom.contains(iNode))
		{
			if (iNode != startPoint)
				// exclude starting point from path
				path.push_back(&m_transits[iNode]);
			iNode = cameFrom[iNode];
		}
		// reverse path
		std::reverse(path.begin(), path.end());
		return path;
	};

	// A* implementation
	while (!openSet.empty())
	{
		size_t current = popOpenSet();
		if (leadsToGoal(current))
			return reconstructPath(current);

		for (size_t neighbor : getNeighbors(current))
		{
			float tentativeGScore = gScore[current] + d(current, neighbor);
			if (gScore[neighbor] > tentativeGScore)
			{
				cameFrom[neighbor] = current;
				gScore[neighbor] = tentativeGScore;
				fScore[neighbor] = tentativeGScore + h(m_transits[neighbor].Midpoint(), goal);
				if (notInOpenSet(neighbor))
					pushOpenSet(neighbor);
			}
		}
	}
	// no path found, returning empty vector
	return std::vector<TransitPlane*>();
}

Pathfinder::NavMesh::NavMesh(Vector3 low, Vector3 hi) : lowCorner(low), hiCorner(hi)
{
}

void Pathfinder::NavMesh::AddTransitPlane(size_t iTransit)
{
	iTransits.push_back(iTransit);
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

Vector3 Pathfinder::TransitPlane::Midpoint()
{
	return (hiCorner - lowCorner) / 2;
}
