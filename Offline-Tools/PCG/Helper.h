#pragma once
#include <string>
#include <unordered_map>
#include <iostream>
#include <fstream>
#include <vector>
#include <random>
#include <stack>
#include <queue>
#include <memory>
#include <cstdlib>
#include <limits>

//Used to save data read from the input.
struct Block
{
	std::string id = "";
	uint32_t count = 0u;
	float frequency = 0.0;
	std::vector<std::string> dirPossibilities[6];
};

//Id is the index in the 1D-array and possibilities is the possibilities that are left for this cell.
struct EntropyBlock
{
	uint32_t id = -1;
	std::vector<std::string> possibilities;
};

//Used in the PriorityQueue. Points to an entryblock and has a calculated Shannon Entropy.
struct QueueBlock
{
	EntropyBlock* m_block = nullptr;
	float m_entropy = 0.0f;
	std::shared_ptr<QueueBlock> m_next;
};

struct Box
{
	Box(std::vector<uint32_t>& minIn, std::vector<uint32_t>& maxIn)
	{
		min = minIn;
		max = maxIn;

		//area = (max[0] - min[0]) * (max[1] - min[1]) * (max[2] - min[2]);
	}

	//Returns true if the Box it was called on is to be put in viable. If false then the childs "viable" is to be taken instead.
	bool Divide(const uint32_t& maxWidth, const uint32_t& maxHeight, const uint32_t& maxDepth, std::default_random_engine& gen)
	{
		uint32_t diffX = max[0] - min[0];
		uint32_t diffToMaxX = diffX - maxWidth;
		uint32_t diffY = max[1] - min[1];
		uint32_t diffToMaxY = diffY - maxHeight;
		uint32_t diffZ = max[2] - min[2];
		uint32_t diffToMaxZ = diffZ - maxDepth;

		uint32_t highestDiff = 0;
		uint32_t maxDimension = 0;
		std::vector<uint32_t> newMax;
		std::vector<uint32_t> newMin;
		if (std::max(std::max(diffToMaxX, diffToMaxY), diffToMaxZ) == diffToMaxX) //If X is the highest value.
		{
			highestDiff = diffX;
			maxDimension = maxWidth;

			newMax.push_back(max[0] - static_cast<uint32_t>(std::floor(static_cast<float>(max[0] - min[0]) / 2.0f)) - 1);
			newMax.push_back(max[1] - 1);
			newMax.push_back(max[2] - 1);

			newMin.push_back(min[0] + static_cast<uint32_t>(std::ceil(static_cast<float>(max[0] - min[0]) / 2.0f)) + 1);
			newMin.push_back(min[1] + 1);
			newMin.push_back(min[2] + 1);
		}
		else if (std::max(std::max(diffToMaxX, diffToMaxY), diffToMaxZ) == diffToMaxY) //If Y is the highest value.
		{
			highestDiff = diffY;
			maxDimension = maxHeight;

			newMax.push_back(max[0] - 1);
			newMax.push_back(max[1] - static_cast<uint32_t>(std::floor(static_cast<float>(max[1] - min[1]) / 2.0f)) - 1);
			newMax.push_back(max[2] - 1);

			newMin.push_back(min[0] + 1);
			newMin.push_back(min[1] + static_cast<uint32_t>(std::ceil(static_cast<float>(max[1] - min[1]) / 2.0f)) + 1);
			newMin.push_back(min[2] + 1);
		}
		else //If Z is the highest value.
		{
			highestDiff = diffZ;
			maxDimension = maxDepth;

			newMax.push_back(max[0] - 1);
			newMax.push_back(max[1] - 1);
			newMax.push_back(max[2] - static_cast<uint32_t>(std::floor(static_cast<float>(max[2] - min[2]) / 2.0f)) - 1);

			newMin.push_back(min[0] + 1);
			newMin.push_back(min[1] + 1);
			newMin.push_back(min[2] + static_cast<uint32_t>(std::ceil(static_cast<float>(max[2] - min[2]) / 2.0f)) + 1);
		}

		//reduce min to get more bigger rooms
		//Increase max to get more smaller rooms
		std::uniform_real_distribution<float> dist(1.0f, 1.0f);
		float val = dist(gen);
		//Check if the highest difference in dimension is lower than that dimensions maxvalue.
		if (static_cast<float>(highestDiff) / 2.0f < (static_cast<float>(maxDimension) * val))
		{
			return true;
		}
		//Calculate a new max point for the smaller box that has the same min point.
		std::shared_ptr<Box> child1 = std::make_shared<Box>(min, newMax);
		if (child1->Divide(maxWidth, maxHeight, maxDepth, gen))
		{
			if (child1->viable.size() != 0)
			{
				viable.insert(viable.end(), child1->viable.begin(), child1->viable.end());
				child1->viable.clear();
			}
			else
			{
				viable.push_back(child1);
			}
		}
		else
		{
			viable.insert(viable.end(), child1->viable.begin(), child1->viable.end());
			child1->viable.clear();
		}

		//Calculate a new min point for the smaller box that has the same max point
		std::shared_ptr<Box> child2 = std::make_shared<Box>(newMin, max);
		if (child2->Divide(maxWidth, maxHeight, maxDepth, gen))
		{
			if (child2->viable.size() != 0)
			{
				viable.insert(viable.end(), child2->viable.begin(), child2->viable.end());
				child2->viable.clear();
			}
			else
			{
				viable.push_back(child2);
			}
		}
		else
		{
			viable.insert(viable.end(), child2->viable.begin(), child2->viable.end());
			child2->viable.clear();
		}

		return false;
	}

	//uint32_t area;
	std::vector<uint32_t> min;
	std::vector<uint32_t> max;

	std::vector<std::shared_ptr<Box>> viable;
};

struct Door
{
	bool placed = false;
	uint32_t pos[3] = { 0u, 0u, 0u };
	uint32_t rot = 0u;
	bool connected = false;
};

struct Room
{
	uint32_t globalPos[3] = {0u, 0u, 0u};
	uint32_t width = 0;
	uint32_t height = 0;
	uint32_t depth = 0;

	Door doors[4]; //+x, +z, -x, -z
	std::vector<std::string> generatedRoom;
	bool generationSuccess = false;
};

struct AStarData
{
	AStarData(uint32_t i, uint32_t f, uint32_t g) : index{ i }, fScore{ f }, gScore{ g } {
	}
	uint32_t index = 0;
	uint32_t fScore = 0;
	uint32_t gScore = 0;
};

class Compare
{
public:
	bool operator() (AStarData a, AStarData b)
	{
		return a.fScore > b.fScore;
	}
};

std::vector<uint32_t> ReconstructPath(std::unordered_map<uint32_t, uint32_t>& cameFrom, uint32_t current);
uint32_t Heuristic(uint32_t* start, uint32_t* goal);
std::vector<std::pair<uint32_t, int>> AStarRoom(Room& room, uint32_t* start, uint32_t* goal);
std::vector<std::pair<uint32_t, int>> AStarLevel(uint32_t& width, uint32_t& height, uint32_t& depth, std::vector<std::string>& level, uint32_t* start, uint32_t* goal);