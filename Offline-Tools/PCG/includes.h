#pragma once
#include <string>
#include <unordered_map>
#include <iostream>
#include <fstream>
#include <vector>
#include <random>
#include <stack>

//Used to save data read from the input.
struct Block
{
	std::string id;
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
	EntropyBlock* m_block;
	float m_entropy;
	QueueBlock* m_next;
};

//Data that gets pushed onto the recursive stack. Information is used to call WFC::Propogate.
struct StackData
{
	std::string removed;
	uint32_t index = -1;
};