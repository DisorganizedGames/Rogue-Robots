#include "PQ.h"

int PriorityQueue::Pop()
{
	if (!m_first)
	{
		return -1; //When all blocks have been handled.
	}
	else
	{
		//Go through the first few elements and see how many there are that share lowest entropy.
		uint32_t count = 1;
		float lowestVal = m_first->m_entropy;
		std::shared_ptr<QueueBlock> current = m_first->m_next;
		while (current && current->m_entropy == lowestVal)
		{
			++count;
			current = current->m_next;
		}

		//If only the first element have the lowest entropy.
		//Set 2nd element as first and return the removed element's index.
		if (count == 1)
		{
			current = m_first;
			m_first = m_first->m_next;

			uint32_t returnIndex = current->m_block->id;
			current->m_block = nullptr;
			current->m_next = nullptr;

			return returnIndex;
		}
		//Otherwise we randomize one of the elements with the lowest entropy and remove it from the PQ.
		else
		{
			std::default_random_engine gen;
			gen.seed(static_cast<unsigned int>(time(NULL)));
			std::uniform_int_distribution<uint32_t> dist(1, count);
			uint32_t val = dist(gen);

			//Go to the randomized element.
			count = 1;
			current = m_first;
			std::shared_ptr<QueueBlock> prev = nullptr;
			while (count != val)
			{
				prev = current;
				current = current->m_next;
				++count;
			}
			if (current)
			{
				//If it's the first element.
				if (!prev)
				{
					m_first = m_first->m_next;
				}
				else
				{
					//If it's the last element.
					if (!current->m_next)
					{
						prev->m_next = nullptr;
					}
					else
					{
						prev->m_next = current->m_next;
					}
				}
				uint32_t returnIndex = current->m_block->id;
				current->m_block = nullptr;
				current->m_next = nullptr;

				return returnIndex;
			}
			else
			{
				return -1; //Should never happen.
			}
		}
	}
}

bool PriorityQueue::Rearrange(uint32_t index, std::unordered_map<unsigned int, Block>& blockPossibilities)
{
	std::shared_ptr<QueueBlock> current = m_first;
	std::shared_ptr<QueueBlock> prev = nullptr;
	//Go to the element in question.
	while (current && current->m_block->id != index)
	{
		prev = current;
		current = current->m_next;
	}

	if (!current)
	{
		return true; //If the index has already been popped. (Could occur because of recursiveness)
	}
	else
	{
		if (current->m_block->possibilities.size() == 0)
		{
			return false; //This means the generation failed, because a block ended up not having any valid block.
		}

		//Recalculate the entropy of the element.
		current->m_entropy = CalculateEntropy(current->m_block->possibilities, blockPossibilities);

		//If it's not the first element it might have to be moved.
		if (prev)
		{
			//Remove it from the PQ.
			prev->m_next = current->m_next;
			current->m_next = nullptr;

			//Find the spot where it is supposed to go.
			std::shared_ptr<QueueBlock> stepper = m_first;
			prev = nullptr;
			while (stepper && stepper->m_entropy < current->m_entropy)
			{
				prev = stepper;
				stepper = stepper->m_next;
			}

			//If the spot where it is supposed to go is the front of the queue.
			if (!prev)
			{
				current->m_next = m_first;
				m_first = current;
			}
			else
			{
				//If it is not the last spot.
				if (stepper)
				{
					current->m_next = prev->m_next;
				}

				prev->m_next = current;
			}
		}
		return true;
	}
}

float PriorityQueue::CalculateEntropy(std::vector<unsigned int>& currentPossibilities, std::unordered_map<unsigned int, Block>& blockPossibilities)
{
	//Shannon entropy.

	float sumWeights = 0.0f;
	float sumWeightxLogWeight = 0.0f;

	//Go through each possibility of the block and sum up the frequencies.
	for (auto c : currentPossibilities)
	{
		float f = blockPossibilities[c].frequency * 100.0f; //Make the frequency into percentage, since log will be used.
		sumWeights += f;
		sumWeightxLogWeight += (f * log(f));
	}

	return log(sumWeights) - (sumWeightxLogWeight / sumWeights);

	//Normal entropy
	/*
	return currentPossibilities.size();
	*/
}
