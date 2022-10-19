#pragma once
#include "Box.h"

struct NavMesh
{
	using Vector3 = DirectX::SimpleMath::Vector3;
	using NavMeshID = size_t;
	using NavNodeID = size_t;

	// Extents
	Vector3 lowCorner;
	Vector3 hiCorner;
	Box corners;
	// Exit zones
	std::vector<NavNodeID> navNodes;

	// Methods
	NavMesh(Vector3 low, Vector3 hi);
	NavMesh(Box extents);
	bool Contains(const Vector3 pos);
	bool Contains(const GridCoord pos);
	float CostWalk(const Vector3 enter, const Vector3 exit);
	float CostFly(const Vector3 enter, const Vector3 exit);
	void AddNavNode(NavNodeID nodeID);
};
