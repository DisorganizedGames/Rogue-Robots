#pragma once
#include "GridCoord.h"


struct Box
{
	using Vector3 = DirectX::SimpleMath::Vector3;

	GridCoord low, high;
	Box() = default;
	Box(const GridCoord& lo, const GridCoord& hi) : low(lo), high(hi) {}
	Box(const Vector3& lo, const Vector3& hi) : low(lo), high(hi) {}
	Box(const Box& other) : low(other.low), high(other.high) {}
	Box(GridCoord west, GridCoord north, GridCoord east, GridCoord south) :
		low(GridCoord(west.x, north.y, 0)), high(GridCoord(east.x, south.y, 1)) {
		--(*this);
	}
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
