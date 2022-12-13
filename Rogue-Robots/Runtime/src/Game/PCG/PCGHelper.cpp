#include "PCGHelper.h"

std::vector<std::pair<uint32_t, int>> ReconstructPath(std::unordered_map<uint32_t, std::pair<uint32_t, int>>& cameFrom, uint32_t current)
{
	std::vector<std::pair<uint32_t, int>> totalPath = { std::make_pair(current, -1) };
	std::pair<uint32_t, int> currentPair;
	while (cameFrom.find(current) != cameFrom.end())
	{
		currentPair = cameFrom[current];
		current = currentPair.first;
		totalPath.insert(totalPath.begin(), currentPair);
	}

	return totalPath;
}

uint32_t Heuristic(uint32_t* start, uint32_t* goal)
{
	return std::abs(static_cast<int>(goal[0]) - static_cast<int>(start[0])) + abs(static_cast<int>(goal[0]) - static_cast<int>(start[0])) + abs(static_cast<int>(goal[0]) - static_cast<int>(start[0]));
}

std::vector<std::pair<uint32_t, int>> AStarRoom(Room& room, uint32_t* start, uint32_t* goal)
{
	uint32_t startIndex = start[0] + start[1] * room.width + start[2] * room.width * room.height;
	uint32_t goalIndex = goal[0] + goal[1] * room.width + goal[2] * room.width * room.height;

	std::unordered_map<uint32_t, uint32_t> fScore;
	fScore[startIndex] = Heuristic(start, start);
	std::unordered_map<uint32_t, uint32_t> gScore;
	gScore[startIndex] = 0u;

	std::unordered_map<uint32_t, bool> added;

	AStarData startData(startIndex, fScore[startIndex], gScore[startIndex]);

	std::priority_queue<AStarData, std::vector<AStarData>, Compare> openSet;
	openSet.push(startData);

	added[startData.index] = true;

	std::unordered_map<uint32_t, std::pair<uint32_t, int>> cameFrom;

	while (!openSet.empty())
	{
		AStarData current = openSet.top();
		if (current.index == goalIndex)
		{
			return ReconstructPath(cameFrom, current.index);
		}

		openSet.pop();
		added[current.index] = false;

		for (uint32_t i{ 0u }; i < 6; ++i)
		{
			uint32_t weight = 1;
			bool condition = false;
			uint32_t neighborIndex = (uint32_t)-1;
			switch (i)
			{
			case 0://+x
			{
				condition = current.index % room.width == room.width - 1;
				neighborIndex = current.index + 1;
				break;
			}
			case 1://-x
			{
				condition = current.index % room.width == 0;
				neighborIndex = current.index - 1;
				break;
			}
			case 2://+y
			{
				condition = current.index % (room.width * room.height) >= (room.width * room.height) - room.width;
				neighborIndex = current.index + room.width;
				break;
			}
			case 3://-y
			{
				condition = current.index % (room.width * room.height) < room.width;
				neighborIndex = current.index - room.width;
				break;
			}
			case 4://+z
			{
				condition = current.index >= room.width * room.height * (room.depth - 1);
				neighborIndex = current.index + room.width * room.height;
				break;
			}
			case 5://-z
			{
				condition = current.index < room.width* room.height;
				neighborIndex = current.index - room.width * room.height;
				break;
			}
			default:
			{
				break;
			}
			}

			//Change the weight or continue if the block is not passable.
			if (condition)
			{
				continue;
			}
			else if (room.generatedRoom[neighborIndex] == "Void")
			{
				weight = 10'000;
			}
			else if (room.generatedRoom[current.index].find("Door") != std::string::npos && room.generatedRoom[neighborIndex] == "Void")
			{
				continue;
			}
			else if (//All blocks it shouldnt be able to create a door through.
				room.generatedRoom[neighborIndex].find("Roof") != std::string::npos ||
				room.generatedRoom[neighborIndex].find("Wall1") != std::string::npos ||
				room.generatedRoom[neighborIndex].find("Shelf") != std::string::npos ||
				room.generatedRoom[neighborIndex].find("WallToRoofCorner") != std::string::npos ||
				room.generatedRoom[neighborIndex].find("InnerCorner") != std::string::npos ||
				room.generatedRoom[neighborIndex].find("Riverbed") != std::string::npos ||
				room.generatedRoom[neighborIndex].find("Spawn") != std::string::npos ||
				room.generatedRoom[neighborIndex].find("Exit") != std::string::npos ||
				room.generatedRoom[neighborIndex].find("Cliff") != std::string::npos ||
				(room.generatedRoom[neighborIndex].find("Floor1") != std::string::npos && room.generatedRoom[neighborIndex].find("WallFloor1") == std::string::npos && room.generatedRoom[neighborIndex].find("CornerFloor1") == std::string::npos)
				)
			{
				continue;
			}

			if (gScore.find(current.index) == gScore.end())
			{
				gScore[current.index] = (uint32_t)-1;
			}
			if (gScore.find(neighborIndex) == gScore.end())
			{
				gScore[neighborIndex] = (uint32_t)-1;
			}
			uint32_t tentativeScore = gScore[current.index] + weight;
			if (tentativeScore < gScore[neighborIndex])
			{
				cameFrom[neighborIndex] = std::make_pair(current.index, static_cast<int>(i));
				gScore[neighborIndex] = tentativeScore;

				uint32_t tempIndex = neighborIndex;
				uint32_t neighborZ = static_cast<uint32_t>(std::floor(tempIndex / (room.width * room.height)));
				tempIndex = tempIndex - (neighborZ * room.width * room.height);
				uint32_t neighborY = static_cast<uint32_t>(std::floor(tempIndex / room.width));
				tempIndex = tempIndex - (neighborY * room.width);
				uint32_t neighborX = tempIndex;
				uint32_t neighborPos[3] = { neighborX, neighborY, neighborZ };
				fScore[neighborIndex] = tentativeScore + Heuristic(neighborPos, goal);
				if (added.find(neighborIndex) == added.end() || added[neighborIndex] == false)
				{
					AStarData newData(neighborIndex, fScore[neighborIndex], gScore[neighborIndex]);
					openSet.push(newData);
				}
			}
		}
	}

	std::vector<std::pair<uint32_t, int>> v;
	return v;
}

std::vector<std::pair<uint32_t, int>> AStarLevel(uint32_t& width, uint32_t& height, uint32_t& depth, std::vector<std::string>& level, uint32_t* start, uint32_t* goal)
{
	uint32_t startIndex = start[0] + start[1] * width + start[2] * width * height;
	uint32_t goalIndex = goal[0] + goal[1] * width + goal[2] * width * height;

	std::unordered_map<uint32_t, uint32_t> fScore;
	fScore[startIndex] = Heuristic(start, start);
	std::unordered_map<uint32_t, uint32_t> gScore;
	gScore[startIndex] = 0u;

	std::unordered_map<uint32_t, bool> added;

	AStarData startData(startIndex, fScore[startIndex], gScore[startIndex]);

	std::priority_queue<AStarData, std::vector<AStarData>, Compare> openSet;
	openSet.push(startData);

	added[startData.index] = true;

	std::unordered_map<uint32_t, std::pair<uint32_t, int>> cameFrom;

	while (!openSet.empty())
	{
		AStarData current = openSet.top();
		if (current.index == goalIndex)
		{
			return ReconstructPath(cameFrom, current.index);
		}

		openSet.pop();
		added[current.index] = false;

		for (uint32_t i{ 0u }; i < 6; ++i)
		{
			uint32_t weight = 5;
			bool condition = false;
			uint32_t neighborIndex = (uint32_t)-1;
			switch (i)
			{
			case 0://+x
			{
				condition = current.index % width == width - 1;
				neighborIndex = current.index + 1;
				break;
			}
			case 1://-x
			{
				condition = current.index % width == 0;
				neighborIndex = current.index - 1;
				break;
			}
			case 2://+y
			{
				condition = current.index % (width * height) >= (width * height) - width;
				neighborIndex = current.index + width;
				break;
			}
			case 3://-y
			{
				condition = current.index % (width * height) < width;
				neighborIndex = current.index - width;
				break;
			}
			case 4://+z
			{
				condition = current.index >= width * height * (depth - 1);
				neighborIndex = current.index + width * height;
				break;
			}
			case 5://-z
			{
				condition = current.index < width* height;
				neighborIndex = current.index - width * height;
				break;
			}
			default:
			{
				break;
			}
			}

			//Change the weights or continue if the block is not passable.
			if (condition)
			{
				continue;
			}
			else if ((level[neighborIndex].find("Door") != std::string::npos || level[current.index].find("Door") != std::string::npos) && (i == 2 || i == 3))
			{
				continue; //Do not allow the A* to go into a door from a vertical angle.
			}
			else if (level[neighborIndex].find("Connector") != std::string::npos)
			{
				weight = 1;
			}
			else if (level[neighborIndex].find("Door") != std::string::npos)
			{
				weight = 1;
			}
			else if (level[neighborIndex] != "Void")
			{
				continue; //Do not allow the A* to go inside rooms.
			}

			if (gScore.find(current.index) == gScore.end())
			{
				gScore[current.index] = (uint32_t)-1;
			}
			if (gScore.find(neighborIndex) == gScore.end())
			{
				gScore[neighborIndex] = (uint32_t)-1;
			}
			uint32_t tentativeScore = gScore[current.index] + weight;
			if (tentativeScore < gScore[neighborIndex])
			{
				cameFrom[neighborIndex] = std::make_pair(current.index, static_cast<int>(i));
				gScore[neighborIndex] = tentativeScore;

				uint32_t tempIndex = neighborIndex;
				uint32_t neighborZ = static_cast<uint32_t>(std::floor(tempIndex / (width * height)));
				tempIndex = tempIndex - (neighborZ * width * height);
				uint32_t neighborY = static_cast<uint32_t>(std::floor(tempIndex / width));
				tempIndex = tempIndex - (neighborY * width);
				uint32_t neighborX = tempIndex;
				uint32_t neighborPos[3] = { neighborX, neighborY, neighborZ };
				fScore[neighborIndex] = tentativeScore + Heuristic(neighborPos, goal);
				if (added.find(neighborIndex) == added.end() || added[neighborIndex] == false)
				{
					AStarData newData(neighborIndex, fScore[neighborIndex], gScore[neighborIndex]);
					openSet.push(newData);
				}
			}
		}
	}

	std::vector<std::pair<uint32_t, int>> v;
	return v;
}