#include "NavMesh.h"

bool NavMesh::Contains(const Vector3 pos) const
{
	// pos is part of the mesh on the lower border
	// but part of the adjoining mesh on the higher
	return lowCorner.x <= pos.x && pos.x < hiCorner.x
		&& lowCorner.y <= pos.y && pos.y < hiCorner.y
		&& lowCorner.z <= pos.z && pos.z < hiCorner.z;
}

bool NavMesh::Contains(const GridCoord pos) const
{
	return corners.Contains(pos);
}

float NavMesh::CostWalk(const Vector3 enter, const Vector3 exit)
{
	return (exit - enter).Length();
}

float NavMesh::CostFly(const Vector3 enter, const Vector3 exit)
{
	return (exit - enter).Length();
}


NavMesh::NavMesh(Vector3 low, Vector3 hi) : lowCorner(low), hiCorner(hi), corners(Box(low, hi))
{
}


NavMesh::NavMesh(Box extents) : corners(extents)
{
	lowCorner = Vector3(static_cast<float>(corners.low.x), static_cast<float>(corners.low.y), static_cast<float>(corners.low.z));
	hiCorner = Vector3(static_cast<float>(corners.high.x), static_cast<float>(corners.high.y), static_cast<float>(corners.high.z));
}

bool NavMesh::AddNavNode(PortalID nodeID)
{
	//auto it = std::find(navNodes.begin(), navNodes.end(), nodeID);
	//if (it == navNodes.end())
	//	navNodes.push_back(nodeID);
	//return it != navNodes.end();
	navNodes.push_back(nodeID);
	return true;
}
