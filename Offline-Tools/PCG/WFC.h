#pragma once
#include "PQ.h"

class WFC
{
public:
	WFC() noexcept = delete;

	//Reads the input and saves the connection data.
	WFC(std::string input, uint32_t width, uint32_t height, uint32_t depth) : m_width{ width }, m_height{ height }, m_depth{ depth }
	{
		ReadInput(input);
	}

	~WFC() noexcept = default;

	//Generates a level from the read input in the constructor or SetInput.
	//Can be called multiple times for different results each time.
	bool GenerateLevel();
	const std::vector<std::string>& GetGeneratedLevel() const
	{
		return m_generatedLevel;
	}

	//Changes the input so that the algorithm uses a different level to generate levels from.
	void SetInput(std::string input);

	//Changes the dimensions of the output.
	void SetDimensions(uint32_t width, uint32_t height, uint32_t depth);

private:
	//Reads input from a file and adds it to the block possibilities.
	void ReadInput(std::string input);

	//Propogates information to neighboring cells after a possibility is removed.
	bool Propogate(std::string& removedId, uint32_t index);
	//Helper function
	void CheckForPropogation(uint32_t currentIndex, uint32_t neighborIndex, unsigned dir, std::string& removed);

private:
	uint32_t m_totalCount = 0u; //Total number of blocks read during input.
	std::unordered_map<std::string, Block> m_blockPossibilities; //The possibilities for each block-id.

	bool m_failed = false; //If the generation fails.

	//Dimensions of the output level.
	uint32_t m_width = 0;
	uint32_t m_height = 0;
	uint32_t m_depth = 0;

	std::vector<std::string> m_generatedLevel; //The final level that is being generated.
	std::vector<EntropyBlock> m_entropy; //The current entropy of the current generation.

	PriorityQueue* m_priorityQueue = nullptr; //Used for prioritizing entropy.

	std::stack<StackData> m_recursiveStack; //Used to circumvent recursiveness. Saves us from stack overflows.
};