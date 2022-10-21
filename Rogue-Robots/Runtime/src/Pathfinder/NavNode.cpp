#include "NavNode.h"

using Vector3 = DirectX::SimpleMath::Vector3;


NavNode::NavNode(Vector3 low, Vector3 hi, NavMeshID one, NavMeshID two) :
	lowCorner(low), hiCorner(hi), iMesh1(one), iMesh2(two) {}

NavNode::NavNode(Box area, NavMeshID meshIdx) :
	corners(area), iMesh1(meshIdx), iMesh2(NavMeshID(-1))
{
	lowCorner = Vector3(static_cast<float>(area.low.x), static_cast<float>(area.low.y), static_cast<float>(area.low.z));
	hiCorner = Vector3(static_cast<float>(area.high.x), static_cast<float>(area.high.y), static_cast<float>(area.high.z));
}

NavNode::NavNode(Vector3 pos, NavMeshID meshIdx) :
	lowCorner(pos), hiCorner(pos), iMesh1(meshIdx), iMesh2(meshIdx) {}

Vector3 NavNode::Midpoint() const
{
	return (hiCorner - lowCorner) / 2;
}

bool NavNode::AddNavMesh(NavMeshID navMesh)
{
	bool inserted = true;
	if (iMesh2 == NavMeshID(-1))
		iMesh2 = navMesh;
	else if (iMesh1 == NavMeshID(-1))
		iMesh1 = navMesh;
	else
		inserted = false;
	return inserted;
}

bool NavNode::Contains(GridCoord pt) const
{
	return corners.Contains(pt);
}
