#pragma once
#include <DOGEngine.h>

class Pathfinder
{
using Vector3 = DirectX::SimpleMath::Vector3;
public:
	using NavNodeID = size_t;
	using NavMeshID = size_t;
	Pathfinder() noexcept;
	virtual ~Pathfinder() = default;


private:
	struct GridCoord
	{
		int x, y, z;
		GridCoord() : x(0), y(0), z(0) {}
		GridCoord(int ptx, int pty, int ptz) : x(ptx), y(pty), z(ptz) {}
		GridCoord(size_t ptx, size_t pty) : 
			x(static_cast<int>(ptx)), y(static_cast<int>(pty)), z(0) {}
		GridCoord(const GridCoord& other) : x(other.x), y(other.y), z(other.z) {}
		GridCoord(const Vector3& pos) : 
			x(static_cast<int>(round(pos.x))), y(static_cast<int>(round(pos.y))), z(static_cast<int>(round(pos.z))) {}
		bool operator<(const GridCoord& other) const { return x < other.x&& y < other.y/* && z < other.z*/; }
		bool operator<=(const GridCoord& other) const { return x <= other.x && y <= other.y/* && z <= other.z*/; }
		bool operator==(const GridCoord& other) const { return x == other.y && y == other.y/* && z == other.z*/; }
		void operator++() { ++x; ++y;/* ++z;*/ }
		void operator--() { --x; --y;/* --z;*/ }
		GridCoord operator+(int n) { return GridCoord(x + n, y + n, z + n); }
		GridCoord operator-(int n) { return GridCoord(x - n, y - n, z - n); }
		std::string str() { return "(" + std::to_string(x) + ", " + std::to_string(y) + ")"; }
	};
	struct Box
	{
		GridCoord low, high;
		Box() = default;
		Box(const GridCoord& lo, const GridCoord& hi) : low(lo), high(hi) {}
		Box(const Vector3& lo, const Vector3& hi) : low(lo), high(hi) {}
		Box(const Box& other) : low(other.low), high(other.high) {}
		Box(GridCoord west, GridCoord north, GridCoord east, GridCoord south) : 
			low(GridCoord(west.x, north.y, 0)), high(GridCoord(east.x, south.y, 1)) { --(*this); }
		void operator--() { ++low; --high; }
		void operator++() { --low; ++high; }
		//Box operator-(Box o) { return Box(low + n, high - n); }
		Box operator-(int n) { return Box(low + n, high - n); }
		Box operator+(int n) { return Box(low - n, high + n); }
		int Area() const;
		bool operator<(const Box& other) const { return this->Area() < other.Area(); }
		bool Contains(const GridCoord pt) const { return low <= pt && pt <= high; }
		bool Contains(const Box box) const { return Contains(box.low) && Contains(box.high); }
		bool ContainsAny(const std::vector<GridCoord>& pts) const;
		Box Intersection(const Box other);
		std::string str() { return low.str() + " " + high.str(); }
	};
	struct NavNode
	{
		Vector3	lowCorner;
		Vector3	hiCorner;
		Box corners;
		NavMeshID  iMesh1;
		NavMeshID  iMesh2;
		NavNode(Vector3 low, Vector3 hi, NavMeshID one, NavMeshID two);
		NavNode(Box area, NavMeshID meshIdx);
		NavNode(Vector3 pos, NavMeshID meshIdx);

		// Methods
		Vector3 Midpoint();
		void AddNavMesh(NavMeshID navMesh);
	};
	struct NavMesh
	{
		// Extents
		Vector3 lowCorner;
		Vector3 hiCorner;
		Box corners;
		// Exit zones
		std::vector<NavNodeID> navNodes;

		// Methods
		NavMesh(Vector3 low, Vector3 hi);
		NavMesh(Box extents);
		void AddNavNode(NavNodeID nodeID);
		bool Contains(const Vector3 pos);
		bool Contains(const GridCoord pos);
		float CostWalk(const Vector3 enter, const Vector3 exit);
		float CostFly(const Vector3 enter, const Vector3 exit);
	};

	// The graph representation of the map
	std::vector<NavMesh> m_navMeshes;
	std::vector<NavNode> m_navNodes;

	// Methods
	NavMeshID NewMesh(Box extents);
	NavNodeID NewNode(NavMeshID mesh, Box extents);
	void ConnectMeshAndNode(NavMeshID mesh, NavNodeID node);
	std::vector<Box> ConnectToNeighborsAndReturnOpen(NavMeshID mesh, Box border);
	void GenerateNavMeshes(std::vector<std::string>& map, GridCoord origin, char& symbol, NavNodeID currentNode);
	// newPos Walk(currentPos, goal, speed)
	// newPos Fly(currentPos, goal, speed)
	size_t FindNavMeshContaining(const Vector3 pos);
	std::vector<NavNode*> Astar(const Vector3 start, const Vector3 goal, float (*h)(Vector3, Vector3));
	float heuristicStraightLine(Vector3 start, Vector3 goal);
};
