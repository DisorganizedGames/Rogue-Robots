#pragma once
#include <DOGEngine.h>
#include "NavMesh.h"
#include "NavNode.h"

class Pathfinder
{
using Vector3 = DirectX::SimpleMath::Vector3;
public:
	using NavNodeID = size_t;
	using NavMeshID = size_t;

	Pathfinder() noexcept;
	virtual ~Pathfinder() = default;

private:

	// The graph representation of the map
	std::vector<NavMesh> m_navMeshes;
	std::vector<NavNode> m_navNodes;

	// Methods
	NavMeshID NewMesh(Box extents);
	NavNodeID NewNode(NavMeshID mesh, Box extents);
	void ConnectMeshAndNode(NavMeshID mesh, NavNodeID node);
	std::vector<Box> ConnectToNeighborsAndReturnOpen(NavMeshID mesh, Box border);
	void GenerateNavMeshes(std::vector<std::string>& map, GridCoord origin, char symbol, NavNodeID currentNode = NavNodeID(-1));
	// newPos Walk(currentPos, goal, speed)
	// newPos Fly(currentPos, goal, speed)
	size_t FindNavMeshContaining(const Vector3 pos);
	std::vector<NavNode*> Astar(const Vector3 start, const Vector3 goal, float (*h)(Vector3, Vector3));
	float heuristicStraightLine(Vector3 start, Vector3 goal);
	void print(NavMeshID mesh, std::vector<bool>& visited, std::vector<std::string>& map);
};
