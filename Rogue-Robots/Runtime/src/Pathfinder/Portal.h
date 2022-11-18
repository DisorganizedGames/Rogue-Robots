#pragma once
#include "Box.h"

struct Portal
{
	using Vector3 = DirectX::SimpleMath::Vector3;
	using NavMeshID = size_t;
	using PortalID = size_t;

	static constexpr size_t MAX_ID = size_t(-1);

	Vector3	lowCorner;
	Vector3	hiCorner;
	Box corners;
	NavMeshID iMesh1;
	NavMeshID iMesh2;
	Portal(Vector3 low, Vector3 hi, NavMeshID one, NavMeshID two);
	Portal(Box area, NavMeshID meshIdx);
	Portal(Vector3 pos, NavMeshID meshIdx);
	Portal(Portal&& other) = default;

	// Methods
	bool Contains(GridCoord pt) const;
	Vector3 Midpoint() const;
	bool AddNavMesh(NavMeshID navMesh);
};
