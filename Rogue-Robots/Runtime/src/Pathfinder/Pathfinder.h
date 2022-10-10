#pragma once
#include <DOGEngine.h>

class Pathfinder
{
using Vector3 = DirectX::SimpleMath::Vector3;
public:
	Pathfinder() noexcept;
	virtual ~Pathfinder() = default;


private:
	using NavNodeID = size_t;
	using NavMeshID = size_t;
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
		Box(const Box& other) : low(other.low), high(other.high) {}
		Box(GridCoord west, GridCoord north, GridCoord east, GridCoord south) : 
			low(GridCoord(west.x, north.y, 0)), high(GridCoord(east.x, south.y, 1)) { --(*this); }
		void operator--() { ++low; --high; }
		void operator++() { --low; ++high; }
		Box operator-(int n) { return Box(low + n, high - n); }
		Box operator+(int n) { return Box(low - n, high + n); }
		int Area() const
		{
			if (high == low) return 0;
			int length = high.x - low.x;
			int width = high.y - low.y;
			return ((length == 0) ? 1 : length) * ((width == 0) ? 1 : width)	;
		}
		bool operator<(const Box& other) const { return this->Area() < other.Area(); }
		bool Contains(const GridCoord pt) const { return low <= pt && pt <= high; }
		bool ContainsAny(const std::vector<GridCoord>& pts) const
		{
			bool contains = false;
			for (const GridCoord pt : pts)
				contains = contains || Contains(pt);
			return contains;
		}
		std::string str() { return low.str() + " " + high.str(); }
	};
	struct NavNode
	{
		Vector3	lowCorner;
		Vector3	hiCorner;
		NavMeshID  iMesh1;
		NavMeshID  iMesh2;
		NavNode(Vector3 low, Vector3 hi, NavMeshID one, NavMeshID two) :
			lowCorner(low), hiCorner(hi), iMesh1(one), iMesh2(two) {}
		NavNode(Vector3 pos, NavMeshID meshIdx) : lowCorner(pos), hiCorner(pos), iMesh1(meshIdx), iMesh2(meshIdx) {}

		// Methods
		Vector3 Midpoint();
	};
	struct NavMesh
	{
		// Extents
		Vector3 lowCorner;
		Vector3 hiCorner;
		// Exit zones
		std::vector<NavNodeID> navNodes;

		// Methods
		NavMesh(Vector3 low, Vector3 hi);
		void AddNavNode(NavNodeID nodeID);
		bool Contains(const Vector3 pos);
		float CostWalk(const Vector3 enter, const Vector3 exit);
		float CostFly(const Vector3 enter, const Vector3 exit);
	};

	// The graph representation of the map
	std::vector<NavMesh> m_navMeshes;
	std::vector<NavNode> m_navNodes;

	// Methods
	void GenerateNavMeshes(std::vector<std::string>& map, GridCoord origin, char& symbol);
	// newPos Walk(currentPos, goal, speed)
	// newPos Fly(currentPos, goal, speed)
	size_t FindNavMeshContaining(const Vector3 pos);
	std::vector<NavNode*> Astar(const Vector3 start, const Vector3 goal, float (*h)(Vector3, Vector3));
	float heuristicStraightLine(Vector3 start, Vector3 goal);
};
