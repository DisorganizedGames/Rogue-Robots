#include "WFC.h"

bool WFC::EdgeConstrain(uint32_t i, uint32_t dir)
{
	bool removed = false;
	for (uint32_t j{ 0u }; j < m_entropy[i].possibilities.size(); ++j) //Go through all the current possibilities
	{
		std::string c = m_entropy[i].possibilities[j];
		//Check if the possibility cannot have a boundary in the negative z direction.
		if (!std::count(m_blockPossibilities[c].dirPossibilities[dir].begin(), m_blockPossibilities[c].dirPossibilities[dir].end(), "Edge"))
		{
			m_entropy[i].possibilities.erase(m_entropy[i].possibilities.begin() + j);
			j--;
			removed = true;
			
			if (m_entropy[i].possibilities.size() == 0)
			{
				m_failed = true;
				break;
			}
		}
	}
	if (m_failed)
	{
		return false;
	}

	if (removed)
	{
		StackData d;
		d.index = i;
		m_recursiveStack.push(d);
		while (!m_recursiveStack.empty())
		{
			StackData d = m_recursiveStack.top();
			Propogate(d.index);
			m_recursiveStack.pop();
		}
	}

	return true;
}

bool WFC::GenerateLevel()
{
	//First we set up the entropy.
	//A vector with all unique id's.
	std::vector<std::string> allBlocks;
	for (auto& c : m_blockPossibilities)
	{
		allBlocks.push_back(c.first);
	}

	//Create an entropy block for each cell.
	//Set the id to be the index, and the possibilities as all blocks for now.
	for (uint32_t i{ 0u }; i < m_width * m_height * m_depth; ++i)
	{
		EntropyBlock temp;
		temp.id = i;
		temp.possibilities = allBlocks;
		m_entropy.push_back(temp);
	}
	
	//ADD A NICE WAY TO INTRODUCE MULTIPLE CONSTRAINTS HERE!
	{
		//We now go through the entropy and introduce the boundary constraint.
		for (uint32_t i{ 0u }; i < m_width * m_height * m_depth; ++i)
		{
			if (i >= m_depth * m_height * (m_width - 1)) //If the index is on x = width
			{
				if (!EdgeConstrain(i, 0))
				{
					break;
				}
			}
			if (i < m_depth * m_height) //If the index is on x = 0.
			{
				if (!EdgeConstrain(i, 1))
				{
					break;
				}
			}

			if (i % m_depth == m_depth - 1) //If the index is on z = depth
			{
				if (!EdgeConstrain(i, 2))
				{
					break;
				}
			}
			if (i % m_depth == 0) //If the index is on z = 0
			{
				if (!EdgeConstrain(i, 3))
				{
					break;
				}
			}
			
			uint32_t remainder = i % (m_depth * m_height);
			if (remainder < m_depth) //If the index is on y = 0
			{
				if (!EdgeConstrain(i, 5))
				{
					break;
				}
			}
			if (remainder < m_depth * m_height && remainder >= m_depth * (m_height - 1)) //If the index is on y = height
			{
				if (!EdgeConstrain(i, 4))
				{
					break;
				}
			}
		}
	}
	if (m_failed)
	{
		std::cout << "FAILED!" << std::endl;
		m_failed = false;
		m_entropy.clear();
		return false;
	}

	//The priority queue is not needed for the constraints. As they do not use a priority.
	//All the entropy blocks should now be placed in a priority queue based on their Shannon entropy.
	m_priorityQueue = new PriorityQueue(m_entropy, m_blockPossibilities, m_width, m_height, m_depth);

	m_generatedLevel.assign(m_width * m_height * m_depth, "");

	//Here the WFC starts.
	//Pop the index with the lowest entropy.
	uint32_t index = m_priorityQueue->Pop();
	if (index == -1)
	{
		std::cout << "MEGA FAIL! SHOULD NEVER HAPPEN!!!" << std::endl;
		delete m_priorityQueue;
		m_priorityQueue = nullptr;
		m_entropy.clear();
		return false;
	}

	//Firstly we go through all the blocks with just 1 possibility.
	while (m_entropy[index].possibilities.size() == 1)
	{
		std::string c = m_entropy[index].possibilities[0];
		m_generatedLevel[index] = c;

		index = m_priorityQueue->Pop();

		//If we are done with the whole generation.
		if (index == -1)
		{
			delete m_priorityQueue;
			m_priorityQueue = nullptr;

			//If the generation failed.
			if (m_failed)
			{
				std::cout << "FAILED!" << std::endl;
				m_failed = false;
				m_entropy.clear();
				return false;
			}
			return true;
		}
	}

	//Here we only have blocks with a possibility count of 2 or higher left.
	std::default_random_engine gen;
	gen.seed(static_cast<unsigned int>(time(NULL)));

	//While blocks exist within the PQ and the count of possibilities is not 0.
	while (index != -1 && m_entropy[index].possibilities.size() != 0)
	{
		//calculate the total frequency of the possibilities of the current cell.
		float total = 0.0f;
		for (auto& c : m_entropy[index].possibilities)
		{
			total += m_blockPossibilities[c].frequency;
		}

		//We then randomize a value between 0 and that total value.
		std::uniform_real_distribution<float> dist(0.0f, total);
		float val = dist(gen);

		std::string c;
		float count = 0.0f;
		//Go through all possibilities and if the generated value is less than the frequency counter that possibility is chosen.
		for (auto& current : m_entropy[index].possibilities)
		{
			count += m_blockPossibilities[current].frequency;
			if (val < count)
			{
				c = current;
				break;
			}
		}

		//Now that a single possibility is chosen the rest of the possibilities have to be removed.
		//The information that each one have been removed is then propogated out to the neighboring cells.
		bool removed = false;
		for (uint32_t i{ 0u }; i < m_entropy[index].possibilities.size(); ++i)
		{
			std::string current = m_entropy[index].possibilities[i];
			if (current != c)
			{
				m_entropy[index].possibilities.erase(m_entropy[index].possibilities.begin() + i);
				i--;
				removed = true;

				if (m_entropy[index].possibilities.size() == 0)
				{
					m_failed = true;
					break;
				}

			}
		}
		if (m_failed)
		{
			break;
		}

		if (removed)
		{
			StackData d;
			d.index = index;
			m_recursiveStack.push(d);
			while (!m_recursiveStack.empty())
			{
				StackData d = m_recursiveStack.top();
				Propogate(d.index);
				m_recursiveStack.pop();
			}
		}
		//We then set the chosen value in the generated level.
		m_generatedLevel[index] = c;

		//Pop the next index.
		index = m_priorityQueue->Pop();
	}

	//We are now done and can delete our priorityqueue.
	delete m_priorityQueue;
	m_priorityQueue = nullptr;

	//If the generation failed we try again.
	if (m_failed)
	{
		std::cout << "FAILED!" << std::endl;
		m_failed = false;
		m_entropy.clear();
		return false;
	}

	return true;
}

void WFC::SetInput(std::string input)
{
	m_blockPossibilities.clear();
	ReadInput(input);
}

void WFC::SetDimensions(uint32_t width, uint32_t height, uint32_t depth)
{
	m_width = width;
	m_height = height;
	m_depth = depth;
}

void WFC::ReadInput(std::string input)
{
	std::string line;
	std::ifstream inputFile(input);

	if (inputFile.is_open())
	{
		//For each unique block.
		while (std::getline(inputFile, line))
		{
			size_t delim = line.find(' ');
			std::string id = line.substr(0, delim);

			m_blockPossibilities[id].id = id;
			//Put the count in the frequency and increment the totalcount.
			m_blockPossibilities[id].frequency = static_cast<float>(std::stoi(line.substr(delim + 1, line.size())));
			m_totalCount += static_cast<uint32_t>(m_blockPossibilities[id].frequency);

			//For each direction.
			for (uint32_t i{ 0u }; i < 6; ++i)
			{
				//Go through each possibility in that direction.
				std::getline(inputFile, line);
				while (line.find(',') != std::string::npos)
				{
					delim = line.find(',');
					m_blockPossibilities[id].dirPossibilities[i].push_back(line.substr(0, delim));
					line.erase(line.begin(), line.begin() + delim + 1);
				}
			}

			//Get empty line that is between blocks.
			std::getline(inputFile, line);
		}
	}
	inputFile.close();

	//Go through each block and change from count to frequency.
	for (auto& block : m_blockPossibilities)
	{
		block.second.frequency /= m_totalCount;

		if (block.first == "Void")
		{
			block.second.frequency *= 0.01f;
		}
		//Here we can tweak the frequencies.
	}

	//Or here
}

bool WFC::Propogate(uint32_t index)
{
	if (index == 67)
	{
		std::cout << "t";
	}
	//We now have to rearrange the PQ since possibilities were removed. (Is not done during contraints.)
	if (m_priorityQueue)
	{
		if (!m_priorityQueue->Rearrange(index, m_blockPossibilities))
		{
			m_failed = true; //Mark generation as failed if it fails to rearrange.
		}
	}
	
	//If the index is not z = 0 we can propogate in the negative z direction.
	if (index % m_depth != 0)
	{
		CheckForPropogation(index, index - 1, 2);
	}

	//If the index is not z = width we can propogate in the positive z direction.
	if (index % m_depth != m_depth - 1)
	{
		CheckForPropogation(index, index + 1, 3);
	}

	//If the index is not y = 0 we can propogate in the negative y direction.
	uint32_t remainder = index % (m_depth * m_height);
	if (remainder >= m_depth)
	{
		CheckForPropogation(index, index - m_depth, 4);
	}

	//If the index is not y = height we can propogate in the positive y direction.
	if (remainder >= m_depth * m_height || remainder < m_depth * (m_height - 1))
	{
		CheckForPropogation(index, index + m_depth, 5);
	}

	//If the index is not x = 0 we can propogate in the negative x direction.
	if (index >= m_depth * m_height)
	{
		CheckForPropogation(index, index - (m_depth * m_height), 0);
	}

	//If the index is not x = depth we can propogate in the positive x direction.
	if (index < m_depth * m_height * (m_width - 1))
	{
		CheckForPropogation(index, index + (m_depth * m_height), 1);
	}

	return true;
}

//Dir is the direction from the neighborIndex TO the currentIndex.
void WFC::CheckForPropogation(uint32_t currentIndex, uint32_t neighborIndex, unsigned dir)
{
	if (neighborIndex == 67)
	{
		std::cout << "t";
	}
	bool removed = false;
	for (uint32_t i{ 0u }; i < m_entropy[neighborIndex].possibilities.size(); ++i) //For every possibility in the neighbor cell
	{
		std::string& r = m_entropy[neighborIndex].possibilities[i];
		bool matched = false;
		
		for (auto& t : m_entropy[currentIndex].possibilities) //Check every possibility still left in the current cell and make sure "r" matches with atleast one of them.
		{
			if (std::count(m_blockPossibilities[r].dirPossibilities[dir].begin(), m_blockPossibilities[r].dirPossibilities[dir].end(), t))
			{
				matched = true;
				break;
			}
		}
		if (!matched)
		{
			m_entropy[neighborIndex].possibilities.erase(m_entropy[neighborIndex].possibilities.begin() + i);
			i--;
			removed = true;

			if (m_entropy[neighborIndex].possibilities.size() == 0)
			{
				m_failed = true;
				break;
			}
		}
	}
	if (removed)
	{
		StackData d;
		d.index = neighborIndex;
		m_recursiveStack.push(d);
	}
}