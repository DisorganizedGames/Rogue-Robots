#pragma once

struct GridCoord
{
	using Vector3 = DirectX::SimpleMath::Vector3;

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
