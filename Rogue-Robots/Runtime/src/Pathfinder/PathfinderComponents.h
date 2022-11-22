#include <DOGEngine.h>

struct NavSceneComponent
{
	std::vector<std::vector<std::vector<DOG::entity>>> map;
};


struct NavMeshComponent
{
	using Vector3 = DirectX::SimpleMath::Vector3;
	using NavMeshID = size_t;
	using PortalID = size_t;

	// Extents
	Vector3 lowCorner;
	Vector3 hiCorner;
	Box corners;
	// Exit zones
	std::vector<PortalID> navNodes;

	// Methods
	NavMesh(Vector3 low, Vector3 hi);
	NavMesh(Box extents);
	bool Contains(const Vector3 pos) const;
	bool Contains(const GridCoord pos) const;
	float CostWalk(const Vector3 enter, const Vector3 exit);
	float CostFly(const Vector3 enter, const Vector3 exit);
	bool AddNavNode(PortalID nodeID);
};


struct PortalComponent
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

