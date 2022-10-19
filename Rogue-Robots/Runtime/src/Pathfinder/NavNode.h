#pragma once
#include "Box.h"

struct NavNode
{
	using Vector3 = DirectX::SimpleMath::Vector3;
	using NavMeshID = size_t;
	using NavNodeID = size_t;

	Vector3	lowCorner;
	Vector3	hiCorner;
	Box corners;
	NavMeshID  iMesh1;
	NavMeshID  iMesh2;
	NavNode(Vector3 low, Vector3 hi, NavMeshID one, NavMeshID two);
	NavNode(Box area, NavMeshID meshIdx);
	NavNode(Vector3 pos, NavMeshID meshIdx);
	NavNode(NavNode&& other) = default;

	// Methods
	Vector3 Midpoint();
	void AddNavMesh(NavMeshID navMesh);
};
