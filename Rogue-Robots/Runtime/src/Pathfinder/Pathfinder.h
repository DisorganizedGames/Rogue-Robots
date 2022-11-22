#pragma once
#include <DOGEngine.h>
#include "NavMesh.h"
#include "Portal.h"
#include "../Game/GameComponent.h"

class Pathfinder
{
using Vector3 = DirectX::SimpleMath::Vector3;
public:
	using NavMeshID = size_t;
	using PortalID = size_t;

	static constexpr size_t MAX_ID = size_t(-1);

	// public methods
	[[nodiscard]] static constexpr Pathfinder& Get() noexcept { return s_instance; }

	void BuildNavScene(SceneComponent::Type sceneType);

private:
	static Pathfinder s_instance;
	// The graph representation of the map
	std::vector<NavMesh> m_navMeshes;
	std::vector<Portal> m_navNodes;

	Pathfinder() noexcept;
	virtual ~Pathfinder() = default;

	// Methods
	NavMeshID NewMesh(Box extents);
	PortalID NewPortal(NavMeshID mesh, Box extents);
	void ConnectMeshAndNode(NavMeshID mesh, PortalID node);
	std::vector<Box> ConnectToNeighborsAndReturnOpen(NavMeshID mesh, Box border);
	void GenerateNavMeshes(std::vector<std::string>& map, GridCoord origin, char symbol, PortalID currentNode = MAX_ID);
	// newPos Walk(currentPos, goal, speed)
	// newPos Fly(currentPos, goal, speed)
	size_t FindNavMeshContaining(const Vector3 pos);
	std::vector<Portal*> Astar(const Vector3 start, const Vector3 goal, float (*h)(Vector3, Vector3));
	float heuristicStraightLine(Vector3 start, Vector3 goal);
	//void print(NavMeshID mesh, std::vector<bool>& visited, std::vector<std::string>& map);
};
