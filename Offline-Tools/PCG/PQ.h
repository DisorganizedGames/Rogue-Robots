#pragma once
#include "Helper.h"

class PriorityQueue
{
public:
	PriorityQueue() noexcept = delete;

	PriorityQueue(std::vector<EntropyBlock>& blockList, std::unordered_map<unsigned int, Block>& blockPossibilities, uint32_t width, uint32_t height, uint32_t depth) noexcept
	{
		m_first = std::make_shared<QueueBlock>();
		//m_first = new QueueBlock();
		m_first->m_block = &blockList[0];
		m_first->m_entropy = CalculateEntropy(m_first->m_block->possibilities, blockPossibilities);
		m_first->m_next = nullptr;

		//For all entropyblocks except first calculate the entropy and place it in the correct location in the PQ.
		for (uint32_t i{ 1u }; i < width * height * depth; ++i)
		{
			std::shared_ptr<QueueBlock> newBlock = std::make_shared<QueueBlock>();
			newBlock->m_block = &blockList[i];
			newBlock->m_entropy = CalculateEntropy(newBlock->m_block->possibilities, blockPossibilities);
			newBlock->m_next = nullptr;

			//If the entropy of the new block is lower or the same as the first block.
			if (newBlock->m_entropy <= m_first->m_entropy)
			{
				newBlock->m_next = m_first;
				m_first = newBlock;
			}
			else
			{
				std::shared_ptr<QueueBlock> prevBlock = m_first;
				std::shared_ptr<QueueBlock> nextBlock = m_first->m_next;
				bool placed = false;
				//Find the spot where the new block is supposed to go.
				while (nextBlock && !placed)
				{
					//If the new blocks entropy is lower or the same as the next block to check we put the new block into the queue here.
					if (newBlock->m_entropy <= nextBlock->m_entropy)
					{
						newBlock->m_next = nextBlock;
						prevBlock->m_next = newBlock;
						placed = true;
					}
					else //Otherwise we go to the next block.
					{
						prevBlock = nextBlock;
						nextBlock = nextBlock->m_next;
					}
				}
				//If it did not get placed during the loop it gets placed at the end of the queue.
				if (!placed)
				{
					prevBlock->m_next = newBlock;
				}
			}
		}
	}

	~PriorityQueue() noexcept
	{
		//This prevents a stack overflow when the generation is done.
		QueueBlock* temp = m_first.get();
		while(temp)
		{
			m_first = temp->m_next;
			temp->m_next = nullptr;
			temp = m_first.get();
		}
	};

	//Returns the index of one of the first blocks in the PQ (decided randomly)
	int Pop();

	//Is called everytime the entropy for a block is reduced, goes to the item in question and rearrages it in the PQ.
	bool Rearrange(uint32_t index, std::unordered_map<unsigned int, Block>& blockPossibilities);

private:
	//Calculates the Shannon entropy of the block. Used as prio.
	float CalculateEntropy(std::vector<unsigned int>& currentPossibilities, std::unordered_map<unsigned int, Block>& blockPossibilities);

	std::shared_ptr<QueueBlock> m_first = nullptr;
};