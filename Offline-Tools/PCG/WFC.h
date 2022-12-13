#pragma once
#include "PQ.h"

class WFC
{
public:
	WFC() noexcept = delete;

	//Reads the input and saves the connection data.
	WFC(uint32_t width, uint32_t height, uint32_t depth) : m_width{ width }, m_height{ height }, m_depth{ depth }
	{
	}

	~WFC() noexcept = default;

	//Generates a level from the read input in the constructor or SetInput.
	//Can be called multiple times for different results each time.
	bool GenerateLevel(uint32_t nrOfRooms, uint32_t maxWidth, uint32_t maxHeight, uint32_t maxDepth);
	const std::vector<std::string>& GetGeneratedLevel() const
	{
		return m_generatedLevel;
	}

	const std::vector<Room>& GetGeneratedRoomsData() const
	{
		return m_generatedRooms;
	}

	//Changes the input so that the algorithm uses a different level to generate levels from.
	bool SetInput(std::string input);

	//Changes the dimensions of the output.
	void SetDimensions(uint32_t width, uint32_t height, uint32_t depth);

private:

	//PRINT FOR DEBUGGING.
	void PrintLevel();

	//CONSTRAINTS-FUNCTIONS
	bool EdgeConstrain(uint32_t i, uint32_t dir, Room& room);
	bool IntroduceConstraints(Room& room);

	bool GenerateRoom(Room& room);

	//Reads input from a file and adds it to the block possibilities.
	bool ReadInput(std::string input);

	//The constrain functions are only used on startup for constraints, same code but uses m_entropy or m_currentEntropy.
	//Propogates information to neighboring cells after a possibility is removed.
	void Propogate(uint32_t index, Room& room);
	void PropogateConstrain(uint32_t index, Room& room);

	//Helper function
	void CheckForPropogation(uint32_t currentIndex, uint32_t neighborIndex, unsigned dir, unsigned int roomi);
	void CheckForPropogationConstrain(uint32_t currentIndex, uint32_t neighborIndex, unsigned dir, unsigned int roomi);

	//Post processing functions.
	std::string ReplaceBlock(std::string& prevBlock, std::string& currentBlock, std::string& nextBlock, int prevDir, int nextDir, bool prevWasVoid, bool doorConnected);
private:
	void t_GenerateRoom(unsigned int i, std::shared_ptr<Box> chosenBox);

	uint32_t m_totalCount = 0u; //Total number of blocks read during input.
	std::unordered_map<unsigned int, Block> m_blockPossibilities; //The possibilities for each block-id.
	std::vector<std::string> m_spawnBlocks;
	unsigned int m_spawnBlocksSize = 0u;
	std::vector<std::string> m_doorBlocks;
	std::vector<std::string> m_connectorBlocks;

	std::vector<bool> m_failed; //If the generation fails.

	//Dimensions of the output level.
	uint32_t m_width = 0;
	uint32_t m_height = 0;
	uint32_t m_depth = 0;

	uint32_t m_spawnCoords[3] = {0u, 0u, 0u};
	std::vector<Room> m_generatedRooms; //The generated rooms. Rooms are placed here before the level is generated 
	std::vector<std::string> m_generatedLevel; //The final level that is being generated.
	std::vector<std::vector<EntropyBlock>> m_entropy; //The initial entropy. After the constraints.
	std::vector<std::vector<EntropyBlock>> m_currentEntropy; //The current entropy of the generation.

	std::vector<PriorityQueue*> m_priorityQueue; //Used for prioritizing entropy.

	std::vector<std::queue<uint32_t>> m_recursiveStack; //Used to circumvent recursiveness. Saves us from stack overflows.

	unsigned int m_uniqueIdCounter = 0u;
	std::unordered_map<unsigned int, std::string> m_idToStringMap;
	std::unordered_map<std::string, unsigned int> m_stringToIdMap;
};