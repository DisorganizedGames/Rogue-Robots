#include "Box.h"

Box Box::Intersection(const Box other)
{
	// find necessary translation to move both boxes to positive space
	// note: GridCoords are as of now always positive
	//GridCoord translate;
	//translate.x = std::min(0, std::min(low.x, other.low.x)) * (-1);
	//translate.y = std::min(0, std::min(low.y, other.low.y)) * (-1);
	//translate.z = std::min(0, std::min(low.z, other.low.z)) * (-1);
	Box result = other;
	// should work as long as both boxes are in positive space
	result.low.x = std::max(result.low.x, std::min(result.high.x, low.x));
	result.low.y = std::max(result.low.y, std::min(result.high.y, low.y));
	result.low.z = std::max(result.low.z, std::min(result.high.z, low.z));
	result.high.x = std::min(result.high.x, std::max(result.low.x, high.x));
	result.high.y = std::min(result.high.y, std::max(result.low.y, high.y));
	result.high.z = std::min(result.high.z, std::max(result.low.z, high.z));
	//Box out = (Contains(result.low) && Contains(result.high) ? result : Box());
	//std::cout << Box(low, high).str() << " / " << other.str() << " = " << result.str() << " [" << result.Area() << "]" << std::endl;
	//std::cout << Box(low, high).str() << " / " << other.str() << " = " << (Contains(result.low) && Contains(result.high) ? result.str() : Box().str()) << std::endl;
	if (Contains(result.low) && Contains(result.high))
		return result;
	else
		return Box();  // basically a point
}

bool Box::ContainsAny(const std::vector<GridCoord>& pts) const
{
	bool contained = false;
	for (GridCoord pt : pts)
		contained = contained || Contains(pt);
	return contained;
}

int Box::Area() const
{
	if (high == low) return 0;
	int length = high.x - low.x;
	int width = high.y - low.y;
	return ((length == 0) ? 1 : length) * ((width == 0) ? 1 : width);
}
int Box::RealArea() const
{
	if (high == low) return 0;
	int length = high.x - low.x;
	int width = high.y - low.y;
	return length * width;
}

GridCoord Box::Midpoint()
{
	return GridCoord(low.x + (high.x - low.x) / 2, low.y + (high.y - low.y) / 2, 0);
}

