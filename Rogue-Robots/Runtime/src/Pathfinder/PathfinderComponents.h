#pragma once
#include <DOGEngine.h>

struct NavSceneComponent
{
	using Vector3 = DirectX::SimpleMath::Vector3;
	using NavMeshID = DOG::entity;

	std::vector<std::vector<std::vector<DOG::entity>>> map;

	void AddIdAt(size_t x, size_t y, size_t z, DOG::entity e);
	bool HasNavMesh(size_t x, size_t y, size_t z);
	bool HasNavMesh(int x, int y, int z);
	NavMeshID At(size_t x, size_t y, size_t z);
	NavMeshID At(int x, int y, int z);
	NavMeshID At(Vector3 pos);
};


struct NavMeshComponent
{
	using Vector3 = DirectX::SimpleMath::Vector3;
	using NavMeshID = DOG::entity;
	using PortalID = DOG::entity;

	// Portals to other NavMeshes
	std::vector<PortalID> portals;

	// Methods
	bool Connected(NavMeshID mesh1, NavMeshID mesh2);
	//bool Contains(const Vector3 pos) const;
	float CostWalk(const Vector3 enter, const Vector3 exit);
	//float CostFly(const Vector3 enter, const Vector3 exit);
	//bool AddPortal(PortalID nodeID);
};


struct PortalComponent
{
	using Vector3 = DirectX::SimpleMath::Vector3;
	using NavMeshID = DOG::entity;
	using PortalID = DOG::entity;

	// Portal - for now a single point in the middle of the intersecting surface
	Vector3 portal;

	// Portal connects
	NavMeshID navMesh1;
	NavMeshID navMesh2;

	PortalComponent(NavMeshID mesh1, NavMeshID mesh2);
	PortalComponent(NavMeshID mesh, Vector3 pos);
	//Portal(Vector3 low, Vector3 hi, NavMeshID one, NavMeshID two);
	//Portal(Box area, NavMeshID meshIdx);
	//Portal(Vector3 pos, NavMeshID meshIdx);
	//Portal(Portal&& other) = default;

	// Methods
	bool Connects(NavMeshID mesh1, NavMeshID mesh2);
	//bool Contains(GridCoord pt) const;
	//Vector3 Midpoint() const;
	//bool AddNavMesh(NavMeshID navMesh);
};


struct PathfinderWalkComponent
{
	using Vector3 = DirectX::SimpleMath::Vector3;

	Vector3 goal;
	std::vector<Vector3> path;
};

struct VisualizePathComponent
{};