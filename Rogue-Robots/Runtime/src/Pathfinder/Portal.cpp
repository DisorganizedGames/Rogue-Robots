#include "Portal.h"

using Vector3 = DirectX::SimpleMath::Vector3;


Portal::Portal(Vector3 low, Vector3 hi, NavMeshID one, NavMeshID two) :
	lowCorner(low), hiCorner(hi), iMesh1(one), iMesh2(two) {}

Portal::Portal(Box area, NavMeshID meshIdx) :
	corners(area), iMesh1(meshIdx), iMesh2(NavMeshID(-1))
{
	lowCorner = Vector3(static_cast<float>(area.low.x), static_cast<float>(area.low.y), static_cast<float>(area.low.z));
	hiCorner = Vector3(static_cast<float>(area.high.x), static_cast<float>(area.high.y), static_cast<float>(area.high.z));
}

Portal::Portal(Vector3 pos, NavMeshID meshIdx) :
	lowCorner(pos), hiCorner(pos), iMesh1(meshIdx), iMesh2(meshIdx) {}

Vector3 Portal::Midpoint() const
{
	return (hiCorner - lowCorner) / 2;
}

bool Portal::AddNavMesh(NavMeshID navMesh)
{
	bool inserted = true;
	if (iMesh2 == MAX_ID)
		iMesh2 = navMesh;
	else if (iMesh1 == MAX_ID)
		iMesh1 = navMesh;
	else
		inserted = false;
	return inserted;
}

bool Portal::Contains(GridCoord pt) const
{
	return corners.Contains(pt);
}
