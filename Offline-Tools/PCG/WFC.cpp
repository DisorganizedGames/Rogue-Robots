#include "WFC.h"

bool WFC::EdgeConstrain(uint32_t cellIndex, uint32_t dir, Room& room)
{
	bool removed = false;
	for (uint32_t j{ 0u }; j < m_entropy[cellIndex].possibilities.size(); ++j) //Go through all the current possibilities in the cell
	{
		std::string possibility = m_entropy[cellIndex].possibilities[j];
		if (possibility.find("Door") != std::string::npos)
		{
			continue;
		}
		//Check if the possibility cannot have a boundary in the direction.
		if (!std::count(m_blockPossibilities[possibility].dirPossibilities[dir].begin(), m_blockPossibilities[possibility].dirPossibilities[dir].end(), "Edge") &&
			!std::count(m_blockPossibilities[possibility].dirPossibilities[dir].begin(), m_blockPossibilities[possibility].dirPossibilities[dir].end(), "Void")) //This has to be tested. Might fuck everything
		{
			if (m_entropy[cellIndex].possibilities.size() == 1)
			{
				m_failed = true;
				break;
			}

			//If it can not we remove the possibility.
			m_entropy[cellIndex].possibilities.erase(m_entropy[cellIndex].possibilities.begin() + j);
			j--;
			removed = true;
			
			
		}
	}
	if (m_failed)
	{
		return false;
	}

	//If we removed a possibility the information has to be propagated.
	if (removed)
	{
		//Push the index onto the stack and then loop through the stack until it is empty.
		//(A call to Propogate can put more information on the stack.)
		m_recursiveStack.push(cellIndex);
		while (!m_recursiveStack.empty())
		{
			uint32_t data = m_recursiveStack.front();
			PropogateConstrain(data, room);
			m_recursiveStack.pop();
		}
	}

	return true;
}

bool WFC::IntroduceConstraints(Room& room)
{
	//First we set up the entropy.
	//A vector with all unique id's.
	std::vector<std::string> allBlocks;
	for (auto& possibility : m_blockPossibilities)
	{
		if (possibility.first.find("Door") == std::string::npos && possibility.first.find("Spawn") == std::string::npos) //Dont add special blocks.
		{
			allBlocks.push_back(possibility.first);
		}
	}

	m_entropy.clear();
	//Create an entropy block for each cell.
	//Set the id to be the index, and the possibilities as all blocks for now.
	for (uint32_t i{ 0u }; i < room.width * room.height * room.depth; ++i)
	{
		EntropyBlock temp;
		temp.id = i;
		temp.possibilities = allBlocks;
		m_entropy.push_back(temp);
	}

	//ADD A NICE WAY TO INTRODUCE MULTIPLE CONSTRAINTS HERE!
	{
		//Spawnblock
		{
			//Place a spawnblock if its the first block.
			if (m_generatedRooms.size() == 0)
			{
				std::default_random_engine gen;
				gen.seed(static_cast<unsigned int>(time(NULL)));
				std::uniform_int_distribution<uint32_t> distWidth(2u, room.width - 3u);
				std::uniform_int_distribution<uint32_t> distDepth(2u, room.depth - 3u);

				std::uniform_int_distribution<size_t> distSpawn(0u, m_spawnBlocks.size() - 1u);

				uint32_t x = distWidth(gen);
				uint32_t y = 1u;
				uint32_t z = distDepth(gen);

				m_spawnCoords[0] = x;
				m_spawnCoords[1] = y;
				m_spawnCoords[2] = z;

				uint32_t index = x + y * room.width + z * room.width * room.height;
				m_entropy[index].possibilities.clear();
				std::string spawnBlock = m_spawnBlocks[distSpawn(gen)];
				m_entropy[index].possibilities.push_back(spawnBlock);

				//Push the index onto the stack and then loop through the stack until it is empty.
				//(A call to Propogate can put more information on the stack.)
				m_recursiveStack.push(index);
				while (!m_recursiveStack.empty())
				{
					uint32_t data = m_recursiveStack.front();
					PropogateConstrain(data, room);
					m_recursiveStack.pop();
				}
			}
		}

		//Doors
		{

			uint32_t doors = 0; //Doors to generate.

			std::default_random_engine gen;
			gen.seed(static_cast<unsigned int>(time(NULL)));
			std::uniform_int_distribution<uint32_t> dist(0u, 3u);

			std::uniform_int_distribution<uint32_t> widthDist(1u, room.width - 2u);
			std::uniform_int_distribution<uint32_t> depthDist(1u, room.depth - 2u);

			//There will always be atleast 1 door.
			uint32_t chosenDoor = dist(gen);
			room.doors[chosenDoor].placed = true;

			uint32_t x = -1;
			uint32_t y = -1;
			uint32_t z = -1;
			switch (chosenDoor)
			{
			case 0:
			{
				x = room.width - 1;
				z = depthDist(gen);
				break;
			}
			case 1:
			{
				x = widthDist(gen);
				z = room.depth - 1;
				break;
			}
			case 2:
			{
				x = 0u;
				z = depthDist(gen);
				break;
			}
			case 3:
			{
				x = widthDist(gen);
				z = 0u;
				break;
			}
			default:
			{
				break;
			}
			}
			y = 1u;

			room.doors[chosenDoor].pos[0] = x;
			room.doors[chosenDoor].pos[1] = y;
			room.doors[chosenDoor].pos[2] = z;

			uint32_t index = x + y * room.width + z * room.width * room.height;
			m_entropy[index].possibilities.clear();
			//Randomize a door block to use.
			std::uniform_int_distribution<size_t> distDoorBlocks(0u, m_doorBlocks.size() - 1u);
			std::string doorBlock = m_doorBlocks[distDoorBlocks(gen)];
			std::string name = doorBlock.substr(0u, doorBlock.find("_"));
			size_t startFlip = doorBlock.find("_", doorBlock.find("_") + 1);
			std::string doorFlip = doorBlock.substr(startFlip + 1u, doorBlock.size() - startFlip);
			std::string doorCorrectRotation = name + "_r" + std::to_string(room.doors[chosenDoor].rot) + "_" + doorFlip;
			m_entropy[index].possibilities.push_back(doorCorrectRotation);
			
			//Push the index onto the stack and then loop through the stack until it is empty.
			//(A call to Propogate can put more information on the stack.)
			m_recursiveStack.push(index);
			while (!m_recursiveStack.empty())
			{
				uint32_t data = m_recursiveStack.front();
				PropogateConstrain(data, room);
				m_recursiveStack.pop();
			}

			
			//If its not the first room we might want more than 1 door.
			uint32_t placedCount = 1u;
			if (m_generatedRooms.size() != 0)
			{
				std::uniform_int_distribution<uint32_t> distDoorDecider(0u, 1u);
				for (uint32_t i{ 0u }; i < 3u; i++)
				{
					++chosenDoor;
					if (chosenDoor == 4u)
					{
						chosenDoor = 0;
					}

					//If we roll that the door should be generated, OR we get to the last direction and still only have 1 door.
					//This guarantees that we will have atleast 2 doors per room.
					if (distDoorDecider(gen) == 1u || (placedCount == 1u && i == 2))
					{
						++placedCount;
						room.doors[chosenDoor].placed = true;

						x = -1;
						y = -1;
						z = -1;
						switch (chosenDoor)
						{
						case 0:
						{
							x = room.width - 1;
							z = depthDist(gen);
							break;
						}
						case 1:
						{
							x = widthDist(gen);
							z = room.depth - 1;
							break;
						}
						case 2:
						{
							x = 0u;
							z = depthDist(gen);
							break;
						}
						case 3:
						{
							x = widthDist(gen);
							z = 0u;
							break;
						}
						default:
						{
							break;
						}
						}
						y = 1u;

						room.doors[chosenDoor].pos[0] = x;
						room.doors[chosenDoor].pos[1] = y;
						room.doors[chosenDoor].pos[2] = z;

						uint32_t index = x + y * room.width + z * room.width * room.height;
						m_entropy[index].possibilities.clear();
						//Randomize a door block to use.
						std::uniform_int_distribution<size_t> distDoorBlocks(0u, m_doorBlocks.size() - 1u);
						std::string doorBlock = m_doorBlocks[distDoorBlocks(gen)];
						std::string name = doorBlock.substr(0u, doorBlock.find("_"));
						size_t startFlip = doorBlock.find("_", doorBlock.find("_") + 1);
						std::string doorFlip = doorBlock.substr(startFlip + 1u, doorBlock.size() - startFlip);
						std::string doorCorrectRotation = name + "_r" + std::to_string(room.doors[chosenDoor].rot) + "_" + doorFlip;
						m_entropy[index].possibilities.push_back(doorCorrectRotation);

						//Push the index onto the stack and then loop through the stack until it is empty.
						//(A call to Propogate can put more information on the stack.)
						m_recursiveStack.push(index);
						while (!m_recursiveStack.empty())
						{
							uint32_t data = m_recursiveStack.front();
							PropogateConstrain(data, room);
							m_recursiveStack.pop();
						}
					}
				}
			}
		}

		//Boundary
		{
			//We now go through the entropy and introduce the boundary constraint.
			for (uint32_t i{ 0u }; i < room.width * room.height * room.depth; ++i)
			{
				if (i >= room.width * room.height * (room.depth - 1)) //If the index is on x = width
				{
					if (!EdgeConstrain(i, 0, room))
					{
						break;
					}
				}
				if (i < room.width * room.height) //If the index is on x = 0.
				{
					if (!EdgeConstrain(i, 1, room))
					{
						break;
					}
				}

				if (i % room.width == room.width - 1) //If the index is on z = depth
				{
					if (!EdgeConstrain(i, 2, room))
					{
						break;
					}
				}
				if (i % room.width == 0) //If the index is on z = 0
				{
					if (!EdgeConstrain(i, 3, room))
					{
						break;
					}
				}

				uint32_t remainder = i % (room.width * room.height);
				if (remainder < room.width) //If the index is on y = 0
				{
					if (!EdgeConstrain(i, 5, room))
					{
						break;
					}
				}
				if (remainder < room.width * room.height && remainder >= room.width * (room.height - 1)) //If the index is on y = height
				{
					if (!EdgeConstrain(i, 4, room))
					{
						break;
					}
				}
			}
		}
	}

	if (m_failed) //If we fail here it means that the contraints imposed can not generate any output.
	{
		m_failed = false;
		m_entropy.clear();
		return false;
	}

	return true;
}

bool WFC::GenerateLevel(uint32_t nrOfRooms, uint32_t maxWidth, uint32_t maxHeight, uint32_t maxDepth)
{
	m_generatedLevel.assign(m_width * m_height * m_depth, "Void");

	//First we construct a virtual space containing blocks that represents the rooms.
	std::vector<uint32_t> min = {1u, 1u, 1u};
	std::vector<uint32_t> max = { m_width - 2, m_height - 2, m_depth - 2 };
	std::shared_ptr<Box> base = std::make_shared<Box>(min, max);

	std::default_random_engine gen;
	gen.seed(static_cast<unsigned int>(time(NULL)));

	std::vector<std::shared_ptr<Box>> viableOptions;
	if (base->Divide(maxWidth, maxHeight, maxDepth, gen))
	{
		viableOptions.push_back(base);
	}
	else
	{
		viableOptions = base->viable;
		base->viable.clear();
	}

	nrOfRooms = std::min(static_cast<uint32_t>(viableOptions.size()), nrOfRooms);
	if (nrOfRooms < 2)
	{
		std::cout << "Too few viable rooms." << std::endl;
		return false;
	}
	std::cout << "Possible options for room generation was " << viableOptions.size() << "." << std::endl;
	std::cout << "Will generate " << nrOfRooms << " rooms." << std::endl;

	//For each room to generate.
	for (uint32_t i{ 0u }; i < nrOfRooms; i++)
	{
		//Now we need to choose nrOfRooms from the viable rooms.
		std::uniform_int_distribution<size_t> roomID(0u, viableOptions.size() - 1u);
		uint32_t index = static_cast<uint32_t>(roomID(gen));
		std::shared_ptr<Box> chosenBox = viableOptions[index];
		//Remove the option from the vector.
		viableOptions.erase(viableOptions.begin() + index);

		Room newRoom;
		newRoom.globalPos[0] = chosenBox->min[0];
		newRoom.globalPos[1] = chosenBox->min[1];
		newRoom.globalPos[2] = chosenBox->min[2];
		newRoom.width = chosenBox->max[0] - chosenBox->min[0];
		newRoom.height = chosenBox->max[1] - chosenBox->min[1];
		newRoom.depth = chosenBox->max[2] - chosenBox->min[2];
		newRoom.generationSuccess = false;
		Door temp;
		temp.rot = 0u;
		newRoom.doors[0] = temp;
		temp.rot = 3u;
		newRoom.doors[1]= temp;
		temp.rot = 2u;
		newRoom.doors[2] = temp;
		temp.rot = 1u;
		newRoom.doors[3] = temp;

		std::cout << "Block count for this next room: " << newRoom.width * newRoom.height * newRoom.depth << std::endl;

		//Introduce the constraints.
		if (!IntroduceConstraints(newRoom))
		{
			std::cout << "Failed to introduce constraints." << std::endl;
			return false;
		}

		std::cout << "Done introducing constraints." << std::endl;

		uint32_t chances = 100;
		
		while((!GenerateRoom(newRoom) && chances != 0) || !newRoom.generationSuccess)
		{
			std::cout << "FAILED!" << std::endl;
			--chances;
		}
		
		//GenerateRoom(newRoom);
		if (chances == 0)
		{
			return false;
		}
		std::cout << "Done with 1 room, id: " << i << std::endl;
		std::cout << "Count of voids:" << std::count(newRoom.generatedRoom.begin(), newRoom.generatedRoom.end(), "Void") << std::endl;

		m_generatedRooms.push_back(newRoom);
	}

	for (Room& room : m_generatedRooms) //For each generated room.
	{
		//Go through and put the room in the final generated level.
		uint32_t idStart = (room.globalPos[0]) + (room.globalPos[1] * m_width) + (room.globalPos[2] * m_width * m_height);

		for (uint32_t z{ 0u }; z < room.depth; ++z)
		{
			for (uint32_t y{ 0u }; y < room.height; ++y)
			{
				for (uint32_t x{ 0u }; x < room.width; ++x)
				{
					m_generatedLevel[idStart + x + (y * m_width) + (z * m_width * m_height)] = room.generatedRoom[x + (y * room.width) + (z * room.width * room.height)];
				}
			}
		}
	}

	//Here post processing for the whole level starts.
	{
		//Go through all doors and see which is furthest away from the spawn point.
		{
			float longestDist = 0.0f;
			uint32_t roomIndex = 0;
			uint32_t doorIndex = 0;
			//SKIP FIRST ROOM! Do not want exit to be in the first room.
			for (uint32_t i{ 1u }; i < m_generatedRooms.size(); ++i) //Go through all rooms to go through all doors.
			{
				for (uint32_t j{ 0u }; j < 4; ++j) //Go through all doors in a room and calculate their distance to the spawn.
				{
					Room& r = m_generatedRooms[i];
					Door& d = r.doors[j];
					if (!d.placed)
					{
						continue;
					}

					uint32_t globalX = d.pos[0] + r.globalPos[0];
					uint32_t globalY = d.pos[1] + r.globalPos[1];
					uint32_t globalZ = d.pos[2] + r.globalPos[2];

					float dist = static_cast<float>(sqrt(pow(abs(static_cast<int>(m_spawnCoords[0]) - static_cast<int>(globalX)), 2) + pow(abs(static_cast<int>(m_spawnCoords[1]) - static_cast<int>(globalY)), 2) + pow(abs(static_cast<int>(m_spawnCoords[2]) - static_cast<int>(globalZ)), 2)));
					if (dist > longestDist)
					{
						longestDist = dist;
						roomIndex = i;
						doorIndex = j;
					}
				}
			}
			Room& roomToUse = m_generatedRooms[roomIndex];
			Door& doorToUse = roomToUse.doors[doorIndex];
			doorToUse.placed = false; //Mark as not placed so that it does not get connected later on.
			uint32_t idStart = (roomToUse.globalPos[0]) + (roomToUse.globalPos[1] * m_width) + (roomToUse.globalPos[2] * m_width * m_height);
			m_generatedLevel[idStart + doorToUse.pos[0] + (doorToUse.pos[1] * m_width) + (doorToUse.pos[2] * m_width * m_height)] = "Exit1_r0_f";
		}
		//Connect all the doors.
		{
			for (uint32_t i{ 0u }; i < m_generatedRooms.size(); ++i) //Go through all rooms to go through all doors.
			{
				for (uint32_t j{ 0u }; j < 4; ++j) //Go through all doors in a room and see if they are connected.
				{
					Door& doorToConnect = m_generatedRooms[i].doors[j];
					//If its not placed or is already connected we skip it.
					if (!doorToConnect.placed || doorToConnect.connected)
					{
						continue;
					}

					float closestDist = std::numeric_limits<float>::max();

					uint32_t roomIndex = 0;
					uint32_t doorIndex = 0;
					//We need to find the closest door that is not in this room.
					for (uint32_t k{ 0u }; k < m_generatedRooms.size(); ++k)
					{
						if (k == i) //If its the same room that the door to connect is in then we skip.
						{
							continue;
						}
						for (uint32_t l{ 0u }; l < 4; ++l)
						{
							Door& d = m_generatedRooms[k].doors[l];
							if (!d.placed)
							{
								continue;
							}

							uint32_t globalX = d.pos[0] + m_generatedRooms[k].globalPos[0];
							uint32_t globalY = d.pos[1] + m_generatedRooms[k].globalPos[1];
							uint32_t globalZ = d.pos[2] + m_generatedRooms[k].globalPos[2];

							float dist = static_cast<float>(sqrt(pow(abs(static_cast<int>(m_spawnCoords[0]) - static_cast<int>(globalX)), 2) + pow(abs(static_cast<int>(m_spawnCoords[1]) - static_cast<int>(globalY)), 2) + pow(abs(static_cast<int>(m_spawnCoords[2]) - static_cast<int>(globalZ)), 2)));
							if (dist < closestDist)
							{
								closestDist = dist;
								roomIndex = k;
								doorIndex = l;
							}
						}
					}

					//Now we know which door is closest that is not in the same room.
					//We run A* to find a path.
					uint32_t start[3] = { m_generatedRooms[i].globalPos[0] + doorToConnect.pos[0], m_generatedRooms[i].globalPos[1] + doorToConnect.pos[1], m_generatedRooms[i].globalPos[2] + doorToConnect.pos[2] };
					uint32_t goal[3] = { m_generatedRooms[roomIndex].globalPos[0] + m_generatedRooms[roomIndex].doors[doorIndex].pos[0], m_generatedRooms[roomIndex].globalPos[1] + m_generatedRooms[roomIndex].doors[doorIndex].pos[1], m_generatedRooms[roomIndex].globalPos[2] + m_generatedRooms[roomIndex].doors[doorIndex].pos[2] };
					std::vector<std::pair<uint32_t, int>> path = AStarLevel(m_width, m_height, m_depth, m_generatedLevel, start, goal);

					int prevDir = -1;
					int nextDir = -1;
					bool prevWasVoid = false;
					for (uint32_t i{ 0u }; i < path.size(); ++i)
					{
						std::string previous = "None";
						std::string current = m_generatedLevel[path[i].first];
						std::string next = "None";

						nextDir = path[i].second;

						//Directions go from earlier block in the path forward.
						//Meaning: prev -> current && current -> next.
						if (i != 0)
						{
							previous = m_generatedLevel[path[i - 1].first];
						}
						if (i != path.size() - 1)
						{
							next = m_generatedLevel[path[i + 1].first];
						}

						std::string replacer = ReplaceBlock(previous, current, next, prevDir, nextDir, prevWasVoid);
						if (m_generatedLevel[path[i].first] == "Void")
						{
							prevWasVoid = true;
						}
						else
						{
							prevWasVoid = false;
						}
						m_generatedLevel[path[i].first] = replacer;
						prevDir = nextDir;
					}

					doorToConnect.connected = true;
					m_generatedRooms[roomIndex].doors[doorIndex].connected = true;
				}
			}
		}
	}
	return true;
}

bool WFC::GenerateRoom(Room& room)
{
	m_currentEntropy.clear();
	m_currentEntropy = m_entropy;

	while (!m_recursiveStack.empty())
	{
		m_recursiveStack.pop();
	}

	//The priority queue is not needed for the constraints. As they do not use a priority.
	//All the entropy blocks should now be placed in a priority queue based on their Shannon entropy.
	m_priorityQueue = new PriorityQueue(m_currentEntropy, m_blockPossibilities, room.width, room.height, room.depth);

	room.generatedRoom.assign(room.width * room.height * room.depth, "Void");
	room.generationSuccess = false;
	//Here the WFC starts.
	//Pop the index with the lowest entropy.
	uint32_t index = m_priorityQueue->Pop();

#ifdef _DEBUG
	if (index == -1)
	{
		std::cout << "MEGA FAIL! SHOULD NEVER HAPPEN!!!" << std::endl;
		delete m_priorityQueue;
		m_priorityQueue = nullptr;
		m_currentEntropy.clear();
		return false;
	}
#endif

	//Firstly we go through all the blocks with just 1 possibility.
	while (m_currentEntropy[index].possibilities.size() == 1)
	{
		std::string possibility = m_currentEntropy[index].possibilities[0]; //It only has 1 possibility.
		room.generatedRoom[index] = possibility; //Put the block in the generated room.
		room.generationSuccess = true;

		index = m_priorityQueue->Pop();

		//If we are done with the whole generation.
		if (index == -1)
		{
			delete m_priorityQueue;
			m_priorityQueue = nullptr;

			//If the generation failed.
			if (m_failed)
			{
				m_failed = false;
				m_currentEntropy.clear();
				return false;
			}
			return true;
		}
	}

	//Here we only have blocks with a possibility count of 2 or higher left.
	std::default_random_engine gen;
	gen.seed(static_cast<unsigned int>(time(NULL)));

	//While blocks exist within the PQ and the count of possibilities is not 0.
	while (index != -1 && m_currentEntropy[index].possibilities.size() != 0)
	{
		//calculate the total frequency of the possibilities of the current cell.
		float total = 0.0f;
		for (auto& c : m_currentEntropy[index].possibilities)
		{
			total += m_blockPossibilities[c].frequency;
		}

		//We then randomize a value between 0 and that total value.
		std::uniform_real_distribution<float> dist(0.0f, total);
		float val = dist(gen);

		std::string chosenBlock = "";
		float count = 0.0f;
		//Go through all possibilities and if the generated value is less than the frequency counter that possibility is chosen.
		for (auto& current : m_currentEntropy[index].possibilities)
		{
			count += m_blockPossibilities[current].frequency;
			if (val < count)
			{
				chosenBlock = current;
				break;
			}
		}

		//Now that a single possibility is chosen the rest of the possibilities have to be removed.
		bool removed = false;
		for (uint32_t i{ 0u }; i < m_currentEntropy[index].possibilities.size(); ++i)
		{
			std::string current = m_currentEntropy[index].possibilities[i];
			if (current != chosenBlock)
			{
				if (m_currentEntropy[index].possibilities.size() == 1)
				{
 					m_failed = true;
					break;
				}

				m_currentEntropy[index].possibilities.erase(m_currentEntropy[index].possibilities.begin() + i);
				i--;
				removed = true;
			}
		}
		if (m_failed)
		{
			break;
		}

		//The information that possibilities have been removed is then propogated out to the neighboring cells.
		if (removed)
		{
			//We now have to rearrange the PQ since possibilities were removed.
			if (!m_priorityQueue->Rearrange(index, m_blockPossibilities))
			{
				m_failed = false;
				m_currentEntropy.clear();
				delete m_priorityQueue;
				m_priorityQueue = nullptr;
				return false;
			}

			m_recursiveStack.push(index);
			while (!m_recursiveStack.empty())
			{
				uint32_t data = m_recursiveStack.front();
				Propogate(data, room);
				m_recursiveStack.pop();
				
				if (m_failed)
				{
					m_failed = false;
					m_currentEntropy.clear();
					return false;
				}
			}
		}
		//We then set the chosen value in the generated level.
		room.generatedRoom[index] = chosenBlock;
		room.generationSuccess = true;

		index = m_priorityQueue->Pop();
	}

	//We are now done and can delete our priorityqueue.
	delete m_priorityQueue;
	m_priorityQueue = nullptr;

	//If the generation failed we return false.
	if (m_failed)
	{
		m_failed = false;
		m_currentEntropy.clear();
		return false;
	}

	//Here we know that the generation of the room was successful.
	//POST-PROCESSING OF THE ROOM!
	{
		//Connect corners that are next to eachother.
		{
			for (uint32_t i{ 0u }; i < room.generatedRoom.size(); ++i)
			{
				if (room.generatedRoom[i] == "Void" || room.generatedRoom[i] == "Empty")
				{
					continue;
				}
				else if (room.generatedRoom[i].find("CornerFloor") != std::string::npos)
				{
					uint32_t rot = std::stoi(room.generatedRoom[i].substr(room.generatedRoom[i].find("_") + 2, 1));
					//Check if a corner is adjacent to this corner.
					std::string replacement1 = "";
					std::string replacement2 = "";
					//If the x-index is not = width.
					if ((i % room.width) != room.width - 1 && i != room.generatedRoom.size() - 1 && room.generatedRoom[i + 1u].find("CornerFloor") != std::string::npos && room.generatedRoom[i + 1u].find("Replacer") == std::string::npos)
					{
						switch (rot)
						{
						case 0:
						{
							if (room.generatedRoom[i].find("Replacer") == std::string::npos)
							{
								replacement1 = "CornerFloor1Replacer1_r0_f";
							}
							else
							{
								replacement1 = "CornerFloor1Replacer6_r0_f";
							}
							if (room.generatedRoom[i + 1u].find("Replacer") == std::string::npos)
							{
							replacement2 = "CornerFloor1Replacer2_r1_f";
							}
							else
							{
							replacement2 = "CornerFloor1Replacer6_r1_f";
							}
							room.generatedRoom[i] = replacement1;
							room.generatedRoom[i + 1u] = replacement2;
							break;
						}
						case 3:
						{
							if (room.generatedRoom[i].find("Replacer") == std::string::npos)
							{
								replacement1 = "CornerFloor1Replacer2_r3_f";
							}
							else
							{
								replacement1 = "CornerFloor1Replacer6_r3_f";
							}
							if (room.generatedRoom[i + 1u].find("Replacer") == std::string::npos)
							{
								replacement2 = "CornerFloor1Replacer1_r2_f";
							}
							else
							{
								replacement2 = "CornerFloor1Replacer6_r2_f";
							}
							room.generatedRoom[i] = replacement1;
							room.generatedRoom[i + 1u] = replacement2;
							break;
						}

						default:
						{
							break;
						}
						}
					}
					//If the z-index is not = depth
					if ((i < room.width * room.height * (room.depth - 1)) && room.generatedRoom[i + (room.width * room.height)].find("CornerFloor") != std::string::npos && room.generatedRoom[i + (room.width * room.height)].find("Replacer") == std::string::npos)
					{
						switch (rot)
						{
						case 3:
						{
							if (room.generatedRoom[i].find("Replacer") == std::string::npos)
							{
								replacement1 = "CornerFloor1Replacer1_r3_f";
							}
							else
							{
								replacement1 = "CornerFloor1Replacer6_r3_f";
							}
							if (room.generatedRoom[i + (room.width * room.height)].find("Replacer") == std::string::npos)
							{
								replacement2 = "CornerFloor1Replacer2_r0_f";
							}
							else
							{
								replacement2 = "CornerFloor1Replacer6_r0_f";
							}
							room.generatedRoom[i] = replacement1;
							room.generatedRoom[i + (room.width * room.height)] = replacement2;
							break;
						}
						case 2:
						{
							if (room.generatedRoom[i].find("Replacer") == std::string::npos)
							{
								replacement1 = "CornerFloor1Replacer2_r2_f";
							}
							else
							{
								replacement1 = "CornerFloor1Replacer6_r2_f";
							}
							if (room.generatedRoom[i + (room.width * room.height)].find("Replacer") == std::string::npos)
							{
								replacement2 = "CornerFloor1Replacer1_r1_f";
							}
							else
							{
								replacement2 = "CornerFloor1Replacer6_r1_f";
							}
							room.generatedRoom[i] = replacement1;
							room.generatedRoom[i + (room.width * room.height)] = replacement2;
							break;
						}
						default:
						{
							break;
						}
						}
					}
				}
			}
		}

		//A* to connect collections of blocks to eachother. Void is considered "walls".
		{
			//First we find the start block. First block that appears that is not a void.
			uint32_t cellIndex = 0u;
			for (cellIndex; cellIndex < room.generatedRoom.size(); cellIndex++)
			{
				if (room.generatedRoom[cellIndex] != "Void")
				{
					break;
				}
			}
			if (cellIndex >= room.generatedRoom.size() - 1u)
			{
				return false;
			}
			//Get the coordiantes for the start index.
			uint32_t startZ = static_cast<uint32_t>(std::floor(cellIndex / (room.width * room.height)));
			cellIndex = cellIndex - (startZ * room.width * room.height);
			uint32_t startY = static_cast<uint32_t>(std::floor(cellIndex / room.width));
			cellIndex = cellIndex - (startY * room.width);
			uint32_t startX = cellIndex;
			uint32_t start[3] = { startX, startY, startZ };
			//Go through every cell in the room and A* to that location.
			for (uint32_t z{ 0u }; z < room.depth; ++z)
			{
				for (uint32_t y{ 0u }; y < room.height; ++y)
				{
					for (uint32_t x{ 0u }; x < room.width; ++x)
					{
						if (room.generatedRoom[x + y * room.width + z * room.width * room.height] != "Void")
						{
							uint32_t goal[3] = { x, y, z };
							std::vector<std::pair<uint32_t, int>> path = AStarRoom(room, start, goal);
							bool goOn = false;
							for (auto& p : path)
							{
								if (room.generatedRoom[p.first] == "Void")
								{
									goOn = true;
									break;
								}
							}

							if (!goOn) //If goOn is false nothing needs to be replaced in this iteration.
							{
								continue;
							}

							//Go through the path and see if something has to be replaced.
							int prevDir = -1;
							int nextDir = -1;
							bool prevWasVoid = false;
							for (uint32_t i{ 0u }; i < path.size(); ++i)
							{
								std::string previous = "None";
								std::string current = room.generatedRoom[path[i].first];
								std::string next = "None";

								nextDir = path[i].second;

								//Directions go from earlier block in the path forward.
								//Meaning: prev -> current && current -> next.
								if (i != 0)
								{
									previous = room.generatedRoom[path[i - 1].first];
								}
								if (i != path.size() - 1)
								{
									next = room.generatedRoom[path[i + 1].first];
								}

								std::string replacer = ReplaceBlock(previous, current, next, prevDir, nextDir, prevWasVoid);
								if (room.generatedRoom[path[i].first] == "Void")
								{
									prevWasVoid = true;
								}
								else
								{
									prevWasVoid = false;
								}
								room.generatedRoom[path[i].first] = replacer;
								prevDir = nextDir;
							}
						}
					}
				}
			}
		}
	}

	return true;
}

//Here be hardcoded dragons and madness
std::string WFC::ReplaceBlock(std::string& prevBlock, std::string& currentBlock, std::string& nextBlock, int prevDir, int nextDir, bool prevWasVoid)
{
	std::string replacer = currentBlock;
	/*
	if (currentBlock != "Void" && (prevWasVoid || prevBlock.find("Replacer") != std::string::npos) && nextBlock == "Void")
	{
		std::string none = "None";
		ReplaceBlock(prevBlock, currentBlock, none, prevDir, -1, false);
	}
	*/
	if (currentBlock == "Void" || nextBlock == "Void" || prevWasVoid || nextBlock.find("Door") != std::string::npos)
	{
		if (currentBlock == "Void")
		{
			//Straight tunnels.
			if (prevDir == nextDir)
			{
				if (prevDir == 0 || prevDir == 1) //Across, side to side
				{
					replacer = "SideTwoConnector_r0_f";
				}
				else if (prevDir == 2 || prevDir == 3) //Up to down
				{
					replacer = "DownUpConnector_r0_f";
				}
				else if (prevDir == 4 || prevDir == 5) //Across, side to side.
				{
					replacer = "SideTwoConnector_r1_f";
				}
			}
			//L-tunnels
			else
			{
				//Horizontal
				if ((prevDir == 1 && nextDir == 5) || (prevDir == 4 && nextDir == 0))
				{
					replacer = "LHorizontalConnector_r3_f";
				}
				else if ((prevDir == 0 && nextDir == 4) || (prevDir == 5 && nextDir == 1))
				{
					replacer = "LHorizontalConnector_r1_f";
				}
				else if ((prevDir == 1 && nextDir == 4) || (prevDir == 5 && nextDir == 0))
				{
					replacer = "LHorizontalConnector_r2_f";
				}
				else if ((prevDir == 4 && nextDir == 1) || (prevDir == 0 && nextDir == 5))
				{
					replacer = "LHorizontalConnector_r0_f";
				}
				//Vertical
				else if (prevDir == 3) //Upwards L
				{
					switch (nextDir)
					{
					case 0:
					{
						replacer = "LVertical1Connector_r0_f";
						break;
					}
					case 1:
					{
						replacer = "LVertical1Connector_r2_f";
						break;
					}
					case 4:
					{
						replacer = "LVertical1Connector_r3_f";
						break;
					}
					case 5:
					{
						replacer = "LVertical1Connector_r1_f";
						break;
					}
					}
				}
				else if (nextDir == 2) //Upwards L
				{
					switch (prevDir)
					{
					case 0:
					{
						replacer = "LVertical1Connector_r2_f";
						break;
					}
					case 1:
					{
						replacer = "LVertical1Connector_r0_f";
						break;
					}
					case 4:
					{
						replacer = "LVertical1Connector_r1_f";
						break;
					}
					case 5:
					{
						replacer = "LVertical1Connector_r3_f";
						break;
					}
					}
				}
				else if (prevDir == 2) //Downwards L
				{
					switch (nextDir)
					{
					case 0:
					{
						replacer = "LVertical2Connector_r2_f";
						break;
					}
					case 1:
					{
						replacer = "LVertical2Connector_r0_f";
						break;
					}
					case 4:
					{
						replacer = "LVertical2Connector_r1_f";
						break;
					}
					case 5:
					{
						replacer = "LVertical2Connector_r3_f";
						break;
					}
					}
				}
				else if (nextDir == 3) //Downwards L
				{
					switch (prevDir)
					{
					case 0:
					{
						replacer = "LVertical2Connector_r0_f";
						break;
					}
					case 1:
					{
						replacer = "LVertical2Connector_r2_f";
						break;
					}
					case 4:
					{
						replacer = "LVertical2Connector_r3_f";
						break;
					}
					case 5:
					{
						replacer = "LVertical2Connector_r1_f";
						break;
					}
					}
				}
			}
		}
		else //This is the block before or after the void.
		{
			int dirToUse;
			if (prevWasVoid)
			{
				dirToUse = prevDir;
				
				if (dirToUse % 2 == 0)
				{
					++dirToUse;
				}
				else
				{
					--dirToUse;
				}
			}
			else
			{
				dirToUse = nextDir;
			}
			size_t firstUnderscore = currentBlock.find('_');
			size_t secondUnderscore = currentBlock.find('_', firstUnderscore + 1);
			std::string name = currentBlock.substr(0, firstUnderscore);
			std::string rotation = currentBlock.substr(firstUnderscore + 1, 2);
			std::string flip = currentBlock.substr(secondUnderscore + 1, currentBlock.size() - secondUnderscore);
			//Replacement of blocks that make up the room, before it goes out to void.
			if (name == "Wall1")
			{
				replacer = "WallTunnelEntrance1";
				replacer += "_" + rotation + "_" + flip;
			}
			else if (name == "WallFloor1")
			{
				if (dirToUse == 3) //If the next block is downwards.
				{
					replacer = "WallFloorTunnelEntrance2";
				}
				else
				{
					replacer = "WallFloorTunnelEntrance1";
				}
				replacer += "_" + rotation + "_" + flip;
			}
			else if (name == "WallFloorTunnelEntrance1" || name == "WallFloorTunnelEntrance2")
			{
				name = "WallFloorTunnelEntrance3";
				replacer += "_" + rotation + "_" + flip;
			}
			else if (name == "WallRoof1")
			{
				if (dirToUse == 2) //If the next block is upwards.
				{
					replacer = "WallRoof1Replacer1";
				}
				else
				{
					replacer = "WallRoof1Replacer2";
				}
				replacer += "_" + rotation + "_" + flip;
			}
			else if (name == "WallRoof1Replacer1" || name == "WallRoof1Replacer2")
			{
				replacer = "WallRoof1Replacer3";
				replacer += "_" + rotation + "_" + flip;
			}
			else if (name == "CornerWall1")
			{
				int rot = std::stoi(rotation.substr(1, 1));
				if (rot == 0 && dirToUse == 0)
				{
					replacer = "CornerWall1Replacer1";
				}
				else if (rot == 0 && dirToUse == 5)
				{
					replacer = "CornerWall1Replacer2";
				}
				else if (rot == 1 && dirToUse == 5)
				{
					replacer = "CornerWall1Replacer1";
				}
				else if (rot == 1 && dirToUse == 1)
				{
					replacer = "CornerWall1Replacer2";
				}
				else if (rot == 2 && dirToUse == 1)
				{
					replacer = "CornerWall1Replacer1";
				}
				else if (rot == 2 && dirToUse == 4)
				{
					replacer = "CornerWall1Replacer2";
				}
				else if (rot == 3 && dirToUse == 4)
				{
					replacer = "CornerWall1Replacer1";
				}
				else if (rot == 3 && dirToUse == 0)
				{
					replacer = "CornerWall1Replacer2";
				}
				replacer += "_" + rotation + "_" + flip;
			}
			else if (name == "CornerWall1Replacer1" || name == "CornerWall1Replacer2")
			{
				replacer = "CornerWall1Replacer3";
				replacer += "_" + rotation + "_" + flip;
			}
			else if (name.find("CornerFloor1") != std::string::npos)
			{
				if (name == "CornerFloor1")
				{
					int rot = std::stoi(rotation.substr(1, 1));
					if (dirToUse == 3)
					{
						replacer = "CornerFloor1Replacer3"; //Only tunnel down
					}
					else if (rot == 0 && dirToUse == 0)
					{
						replacer = "CornerFloor1Replacer1";
					}
					else if (rot == 0 && dirToUse == 5)
					{
						replacer = "CornerFloor1Replacer2";
					}
					else if (rot == 1 && dirToUse == 5)
					{
						replacer = "CornerFloor1Replacer1";
					}
					else if (rot == 1 && dirToUse == 1)
					{
						replacer = "CornerFloor1Replacer2";
					}
					else if (rot == 2 && dirToUse == 1)
					{
						replacer = "CornerFloor1Replacer1";
					}
					else if (rot == 2 && dirToUse == 4)
					{
						replacer = "CornerFloor1Replacer2";
					}
					else if (rot == 3 && dirToUse == 4)
					{
						replacer = "CornerFloor1Replacer1";
					}
					else if (rot == 3 && dirToUse == 0)
					{
						replacer = "CornerFloor1Replacer2";
					}
				}
				else if (name == "CornerFloor1Replacer1") //If only left or right, replace with connection to both or connection to down.
				{
					if (dirToUse == 3)
					{
						replacer = "CornerFloor1Replacer4"; //Only tunnel left/right and down
					}
					else
					{
						replacer = "CornerFloor1Replacer6"; //Only tunnel to the sides

					}
				}
				else if (name == "CornerFloor1Replacer2") //If only left or right, replace with connection to both or connection to down.
				{
					if (dirToUse == 3)
					{
						replacer = "CornerFloor1Replacer5"; //Only tunnel left/right and down.
					}
					else
					{
						replacer = "CornerFloor1Replacer6"; //Only tunnel to the sides.
					}
				}
				else if (name == "CornerFloor1Replacer3") // If only tunnel down, replace with tunnel down and left or right.
				{
					int rot = static_cast<int>(rotation[1]);

					if (rot == 0 && dirToUse == 0)
					{
						replacer = "CornerFloor1Replacer4";
					}
					else if (rot == 0 && dirToUse == 5)
					{
						replacer = "CornerFloor1Replacer5";
					}
					else if (rot == 1 && dirToUse == 5)
					{
						replacer = "CornerFloor1Replacer4";
					}
					else if (rot == 1 && dirToUse == 1)
					{
						replacer = "CornerFloor1Replacer5";
					}
					else if (rot == 2 && dirToUse == 1)
					{
						replacer = "CornerFloor1Replacer4";
					}
					else if (rot == 2 && dirToUse == 4)
					{
						replacer = "CornerFloor1Replacer5";
					}
					else if (rot == 3 && dirToUse == 4)
					{
						replacer = "CornerFloor1Replacer4";
					}
					else if (rot == 3 && dirToUse == 0)
					{
						replacer = "CornerFloor1Replacer5";
					}
				}
				else if (name == "CornerFloor1Replacer4" || name == "CornerFloor1Replacer5" || name == "CornerFloor1Replacer6") //If tunnels left and right or left and down or right and down, change to all directions.
				{
					replacer = "CornerFloor1Replacer7"; //All directions.
				}

				replacer += "_" + rotation + "_" + flip;
			}
			else if (name.find("CornerRoof1") != std::string::npos)
			{
				if (name == "CornerRoof1")
				{
					int rot = std::stoi(rotation.substr(1, 1));
					if (dirToUse == 2)
					{
						replacer = "CornerRoof1Replacer3"; //Only tunnel up
					}
					else if (rot == 0 && dirToUse == 0)
					{
						replacer = "CornerRoof1Replacer1";
					}
					else if (rot == 0 && dirToUse == 5)
					{
						replacer = "CornerRoof1Replacer2";
					}
					else if (rot == 1 && dirToUse == 5)
					{
						replacer = "CornerRoof1Replacer1";
					}
					else if (rot == 1 && dirToUse == 1)
					{
						replacer = "CornerRoof1Replacer2";
					}
					else if (rot == 2 && dirToUse == 1)
					{
						replacer = "CornerRoof1Replacer1";
					}
					else if (rot == 2 && dirToUse == 4)
					{
						replacer = "CornerRoof1Replacer2";
					}
					else if (rot == 3 && dirToUse == 4)
					{
						replacer = "CornerRoof1Replacer1";
					}
					else if (rot == 3 && dirToUse == 0)
					{
						replacer = "CornerRoof1Replacer2";
					}
				}
				else if (name == "CornerRoof1Replacer1") //If only left or right, replace with connection to both or connection to up.
				{
					if (dirToUse == 2)
					{
						replacer = "CornerRoof1Replacer4"; //Only tunnel left/right and up
					}
					else
					{
						replacer = "CornerRoof1Replacer6"; //Only tunnel to the sides

					}
				}
				else if (name == "CornerRoof1Replacer2") //If only left or right, replace with connection to both or connection to up.
				{
					if (dirToUse == 2)
					{
						replacer = "CornerRoof1Replacer5"; //Only tunnel left/right and up
					}
					else
					{
						replacer = "CornerRoof1Replacer6"; //Only tunnel to the sides.
					}
				}
				else if (name == "CornerRoof1Replacer3") // If only tunnel up, replace with tunnel down and left or right.
				{
					int rot = static_cast<int>(rotation[1]);

					if (rot == 0 && dirToUse == 0)
					{
						replacer = "CornerRoof1Replacer4";
					}
					else if (rot == 0 && dirToUse == 5)
					{
						replacer = "CornerRoof1Replacer5";
					}
					else if (rot == 1 && dirToUse == 5)
					{
						replacer = "CornerRoof1Replacer4";
					}
					else if (rot == 1 && dirToUse == 1)
					{
						replacer = "CornerRoof1Replacer5";
					}
					else if (rot == 2 && dirToUse == 1)
					{
						replacer = "CornerRoof1Replacer4";
					}
					else if (rot == 2 && dirToUse == 4)
					{
						replacer = "CornerRoof1Replacer5";
					}
					else if (rot == 3 && dirToUse == 4)
					{
						replacer = "CornerRoof1Replacer4";
					}
					else if (rot == 3 && dirToUse == 0)
					{
						replacer = "CornerRoof1Replacer5";
					}
				}
				else if (name == "CornerRoof1Replacer4" || name == "CornerRoof1Replacer5" || name == "CornerRoof1Replacer6") //If tunnels left and right or left and down or right and down, change to all directions.
				{
					replacer = "CornerRoof1Replacer7"; //All directions.
				}

				replacer += "_" + rotation + "_" + flip;
			}
			//Replacement of blocks that used to be voids but have been replaced with connectors.
			//add T-tunnel, if straight tunnel
			else if (name == "TunnelStraight1" || name == "SideTwoConnector")
			{
				int rot = std::stoi(rotation.substr(1, 1));
				if (dirToUse == 3) //Tunnel downwards
				{
					replacer = "TVertical2Connector_" + rotation + "_f";
				}
				else if (dirToUse == 2) //Tunnel upwards
				{
					replacer = "TVertical1Connector_" + rotation + "_f";
				}
				//Horizontal connections.
				else if (dirToUse == 5 && (rot == 0 || rot == 2))
				{
					replacer = "THorizontal1Connector_r0_f";
				}
				else if (dirToUse == 4 && (rot == 0 || rot == 2))
				{
					replacer = "THorizontal1Connector_r2_f";
				}
				else if (dirToUse == 1 && (rot == 1 || rot == 3))
				{
					replacer = "THorizontal1Connector_r1_f";
				}
				else if (dirToUse == 0 && (rot == 1 || rot == 3))
				{
					replacer = "THorizontal1Connector_r3_f";
				}
			}
			//Add T-tunnel, if horizontal L tunnel
			else if (name == "LHorizontalConnector")
			{
				int rot = std::stoi(rotation.substr(1, 1));
				if (dirToUse == 2)
				{
					replacer = "LUpConnector_" + rotation + "_f";
				}
				else if (dirToUse == 3)
				{
					replacer = "LDownConnector_" + rotation + "_f";
				}
				else
				{
					switch (rot)
					{
					case 0:
					{
						if (dirToUse == 0)
						{
							replacer = "THorizontal1Connector_r0_f";
						}
						else if (dirToUse == 4)
						{
							replacer = "THorizontal1Connector_r1_f";
						}
						break;
					}
					case 1:
					{
						if (dirToUse == 0)
						{
							replacer = "THorizontal1Connector_r2_f";
						}
						else if (dirToUse == 5)
						{
							replacer = "THorizontal1Connector_r1_f";
						}
						break;
					}
					case 2:
					{
						if (dirToUse == 1)
						{
							replacer = "THorizontal1Connector_r2_f";
						}
						else if (dirToUse == 5)
						{
							replacer = "THorizontal1Connector_r3_f";
						}
						break;
					}
					case 3:
					{
						if (dirToUse == 1)
						{
							replacer = "THorizontal1Connector_r0_f";
						}
						else if (dirToUse == 4)
						{
							replacer = "THorizontal1Connector_r3_f";
						}
						break;
					}
					}
				}
				
			}
			//Add T-tunnel, if vertical L tunnel
			else if (name == "LVertical1Connector") //Up
			{
				int rot = std::stoi(rotation.substr(1, 1));
				if (dirToUse == 3)
				{
					replacer = "TVertical3Connector_" + rotation + "_f";
				}
				else
				{
					switch (rot)
					{
					case 0:
					{
						if (dirToUse == 1)
						{
							replacer = "TVertical1Connector_r0_f";
						}
						else if (dirToUse == 4)
						{
							replacer = "LUpConnector_r2_f";
						}
						else if (dirToUse == 5)
						{
							replacer = "LUpConnector_r3_f";
						}
						break;
					}
					case 1:
					{
						if (dirToUse == 0)
						{
							replacer = "LUpConnector_r3_f";
						}
						else if (dirToUse == 1)
						{
							replacer = "LUpConnector_r0_f";
						}
						else if (dirToUse == 4)
						{
							replacer = "TVertical1Connector_r1_f";
						}
						break;
					}
					case 2:
					{
						if (dirToUse == 0)
						{
							replacer = "TVertical1Connector_r2_f";
						}
						else if (dirToUse == 4)
						{
							replacer = "LUpConnector_r1_f";
						}
						else if (dirToUse == 5)
						{
							replacer = "LUpConnector_r0_f";
						}
						break;
					}
					case 3:
					{
						if (dirToUse == 5)
						{
							replacer = "TVertical1Connector_r3_f";
						}
						else if (dirToUse == 0)
						{
							replacer = "LUpConnector_r2_f";
						}
						else if (dirToUse == 1)
						{
							replacer = "LUpConnector_r1_f";
						}
						break;
					}
					}
				}
			}
			else if (name == "LVertical2Connector") //Down
			{
				int rot = std::stoi(rotation.substr(1, 1));
				if (dirToUse == 2)
				{
					rot = (rot + 2) % 4;
					
					replacer = "TVertical3Connector_r" + std::to_string(rot) + "_f";
				}
				else
				{
					switch (rot)
					{
					case 0:
					{
						if (dirToUse == 0)
						{
							replacer = "TVertical2Connector_r0_f";
						}
						else if (dirToUse == 5)
						{
							replacer = "LDownConnector_r0_f";
						}
						else if (dirToUse == 4)
						{
							replacer = "LDownConnector_r1_f";
						}
						break;
					}
					case 1:
					{
						if (dirToUse == 5)
						{
							replacer = "TVertical2Connector_r1_f";
						}
						else if (dirToUse == 0)
						{
							replacer = "LDownConnector_r2_f";
						}
						else if (dirToUse == 1)
						{
							replacer = "LDownConnector_r1_f";
						}
						break;
					}
					case 2:
					{
						if (dirToUse == 1)
						{
							replacer = "TVertical2Connector_r2_f";
						}
						else if (dirToUse == 4)
						{
							replacer = "LDownConnector_r2_f";
						}
						else if (dirToUse == 5)
						{
							replacer = "LDownConnector_r3_f";
						}
						break;
					}
					case 3:
					{
						if (dirToUse == 4)
						{
							replacer = "TVertical2Connector_r3_f";
						}
						else if (dirToUse == 0)
						{
							replacer = "LDownConnector_r3_f";
						}
						else if (dirToUse == 1)
						{
							replacer = "LDownConnector_r0_f";
						}
						break;
					}
					}
				}
			}
			//Add T-tunnel, if vertical tunnel
			else if (name == "DownUpConnector")
			{
				switch (dirToUse)
				{
				case 0:
				{
					replacer = "TVertical3Connector_r0_f";
					break;
				}
				case 1:
				{
					replacer = "TVertical3Connector_r2_f";
					break;
				}
				case 4:
				{
					replacer = "TVertical3Connector_r3_f";
					break;
				}
				case 5:
				{
					replacer = "TVertical3Connector_r1_f";
					break;
				}
				}
			}
			//add 4-connection-tunnel, if T-tunnel
			else if (name == "TVertical1Connector")
			{
				if (dirToUse == 3)
				{
					replacer = "PlusVerticalConnector_" + rotation + "_f";
				}
				else
				{
					int rot = std::stoi(rotation.substr(1, 1));
					if (rot == 0 || rot == 2)
					{
						if (dirToUse == 4)
						{
							replacer = "4UpConnector_r0_f";
						}
						else if (dirToUse == 5)
						{
							replacer = "4UpConnector_r2_f";
						}
					}
					else if (rot == 1 || rot == 3)
					{
						if (dirToUse == 0)
						{
							replacer = "4UpConnector_r1_f";
						}
						else if (dirToUse == 1)
						{
							replacer = "4UpConnector_r3_f";
						}
					}
				}
			}
			else if (name == "TVertical2Connector")
			{
				if (dirToUse == 2)
				{
					replacer = "PlusVerticalConnector_" + rotation + "_f";
				}
				else
				{
					int rot = std::stoi(rotation.substr(1, 1));
					if (rot == 0 || rot == 2)
					{
						if (dirToUse == 4)
						{
							replacer = "4DownConnector_r2_f";
						}
						else if (dirToUse == 5)
						{
							replacer = "4DownConnector_r0_f";
						}
					}
					else if (rot == 1 || rot == 3)
					{
						if (dirToUse == 0)
						{
							replacer = "4DownConnector_r3_f";
						}
						else if (dirToUse == 1)
						{
							replacer = "4DownConnector_r1_f";
						}
					}
				}
			}
			else if (name == "TVertical3Connector")
			{
				int rot = std::stoi(rotation.substr(1, 1));
				switch (rot)
				{
				case 0:
				{
					if (dirToUse == 1)
					{
						replacer = "PlusVerticalConnector_r0_f";
					}
					else if (dirToUse == 4)
					{
						replacer = "4UpDownConnector_r3_f";
					}
					else if (dirToUse == 5)
					{
						replacer = "4UpDownConnector_r0_f";
					}
					break;
				}
				case 1:
				{
					if (dirToUse == 4)
					{
						replacer = "PlusVerticalConnector_r1_f";
					}
					else if (dirToUse == 0)
					{
						replacer = "4UpDownConnector_r0_f";
					}
					else if (dirToUse == 1)
					{
						replacer = "4UpDownConnector_r1_f";
					}
					break;
				}
				case 2:
				{
					if (dirToUse == 0)
					{
						replacer = "PlusVerticalConnector_r2_f";
					}
					else if (dirToUse == 4)
					{
						replacer = "4UpDownConnector_r2_f";
					}
					else if (dirToUse == 5)
					{
						replacer = "4UpDownConnector_r1_f";
					}
					break;
				}
				case 3:
				{
					if (dirToUse == 5)
					{
						replacer = "PlusVerticalConnector_r3_f";
					}
					else if (dirToUse == 0)
					{
						replacer = "4UpDownConnector_r3_f";
					}
					else if (dirToUse == 1)
					{
						replacer = "4UpDownConnector_r2_f";
					}
					break;
				}
				}
			}
			else if (name == "THorizontal1Connector" || name == "TunnelT1")
			{
				if (dirToUse != 2 && dirToUse != 3)
				{
					replacer = "PlusHorizontalConnector_" + rotation + "_f";
				}
				else if (dirToUse == 2)
				{
					int rot = std::stoi(rotation.substr(1, 1));
					rot = (rot + 2) % 4;

					replacer = "4UpConnector_r" + std::to_string(rot) + "_f";
				}
				else if (dirToUse == 3)
				{
					replacer = "4DownConnector_" + rotation + "_f";
				}
			}
			else if (name == "TunnelT2")
			{
				int rot = std::stoi(rotation.substr(1, 1));
				rot = (rot + 2) % 4;

				if (dirToUse != 2 && dirToUse != 3)
				{
					replacer = "PlusHorizontalConnector_r" + std::to_string(rot) + "_f";
				}
				else if (dirToUse == 2)
				{
					rot = (rot + 2) % 4;
					replacer = "4UpConnector_r" + std::to_string(rot) + "_f";
				}
				else if (dirToUse == 3)
				{
					replacer = "4DownConnector_" + rotation + "_f";
				}
			}
			else if (name == "LUpConnector")
			{
				int rot = std::stoi(rotation.substr(1, 1));
				if (dirToUse == 3)
				{
					rot = rot + 1;
					replacer = "4UpDownConnector_r" + std::to_string(rot) + "_f";
				}
				else
				{
					switch (rot)
					{
					case 0:
					{
						if (dirToUse == 0)
						{
							replacer = "4UpConnector_r2_f";
						}
						else if (dirToUse == 4)
						{
							replacer = "4UpConnector_r3_f";
						}
						break;
					}
					case 1:
					{
						if (dirToUse == 0)
						{
							replacer = "4UpConnector_r0_f";
						}
						else if (dirToUse == 5)
						{
							replacer = "4UpConnector_r3_f";
						}
						break;
					}
					case 2:
					{
						if (dirToUse == 1)
						{
							replacer = "4UpConnector_r0_f";
						}
						else if (dirToUse == 5)
						{
							replacer = "4UpConnector_r1_f";
						}
						break;
					}
					case 3:
					{
						if (dirToUse == 1)
						{
							replacer = "4UpConnector_r2_f";
						}
						else if (dirToUse == 4)
						{
							replacer = "4UpConnector_r1_f";
						}
						break;
					}
					}
				}
			}
			else if (name == "LDownConnector")
			{
				int rot = std::stoi(rotation.substr(1, 1));
				if (dirToUse == 2)
				{
					rot = rot + 1;
					replacer = "4UpDownConnector_r" + std::to_string(rot) + "_f";
				}
				else
				{
					switch (rot)
					{
					case 0:
					{
						if (dirToUse == 0)
						{
							replacer = "4DownConnector_r0_f";
						}
						else if (dirToUse == 4)
						{
							replacer = "4DownConnector_r1_f";
						}
						break;
					}
					case 1:
					{
						if (dirToUse == 0)
						{
							replacer = "4DownConnector_r2_f";
						}
						else if (dirToUse == 5)
						{
							replacer = "4DownConnector_r1_f";
						}
						break;
					}
					case 2:
					{
						if (dirToUse == 1)
						{
							replacer = "4DownConnector_r2_f";
						}
						else if (dirToUse == 5)
						{
							replacer = "4DownConnector_r3_f";
						}
						break;
					}
					case 3:
					{
						if (dirToUse == 1)
						{
							replacer = "4DownConnector_r0_f";
						}
						else if (dirToUse == 4)
						{
							replacer = "4DownConnector_r3_f";
						}
						break;
					}
					}
				}
			}
			//add 5-connection tunnel, if 4-connection tunnel.
			else if (name == "PlusVerticalConnector")
			{
				int rot = std::stoi(rotation.substr(1, 1));
				if (rot == 0 || rot == 2)
				{
					if (dirToUse == 4)
					{
						replacer = "5NoSideConnector_r2_f";
					}
					else if (dirToUse == 5)
					{
						replacer = "5NoSideConnector_r0_f";
					}
				}
				else if (rot == 1 || rot == 3)
				{
					if (dirToUse == 0)
					{
						replacer = "5NoSideConnector_r3_f";
					}
					else if (dirToUse == 1)
					{
						replacer = "5NoSideConnector_r1_f";
					}
				}
			}
			else if (name == "4UpConnector")
			{
				if (dirToUse != 3)
				{
					replacer = "5NoDownConnector_r0_f";
				}
				else if (dirToUse == 3)
				{
					int rot = std::stoi(rotation.substr(1, 1));
					rot = (rot + 2) % 4;
					replacer = "5NoSideConnector_r" + std::to_string(rot) + "_f";
				}
			}
			else if (name == "4DownConnector")
			{
				if (dirToUse != 2)
				{
					replacer = "5NoUpConnector_r0_f";
				}
				else if (dirToUse == 2)
				{
					int rot = std::stoi(rotation.substr(1, 1));
					replacer = "5NoSideConnector_r" + std::to_string(rot) + "_f";
				}
			}
			else if (name == "4UpDownConnector")
			{
				int rot = std::stoi(rotation.substr(1, 1));
				switch (rot)
				{
				case 0:
				{
					if (dirToUse == 1)
					{
						replacer = "5NoSideConnector_r0_f";
					}
					else if (dirToUse == 4)
					{
						replacer = "5NoSideConnector_r3_f";
					}
					break;
				}
				case 1:
				{
					if (dirToUse == 0)
					{
						replacer = "5NoSideConnector_r0_f";
					}
					else if (dirToUse == 4)
					{
						replacer = "5NoSideConnector_r1_f";
					}
					break;
				}
				case 2:
				{
					if (dirToUse == 0)
					{
						replacer = "5NoSideConnector_r2_f";
					}
					else if (dirToUse == 5)
					{
						replacer = "5NoSideConnector_r1_f";
					}
					break;
				}
				case 3:
				{
					if (dirToUse == 1)
					{
						replacer = "5NoSideConnector_r2_f";
					}
					else if (dirToUse == 5)
					{
						replacer = "5NoSideConnector_r3_f";
					}
					break;
				}
				}
			}
			else if (name == "PlusHorizontalConnector" || name == "TunnelCross1")
			{
				if (dirToUse == 2)
				{
					replacer = "5NoUpConnector_" + rotation + "_f";
				}
				else if (dirToUse == 3)
				{
					replacer = "5NoDownConnector_" + rotation + "_f";
				}
			}
			//add 6-connection tunnel, if 5-connection tunnel.
			else if (name == "5NoSideConnector" || name == "5NoDownConnector" || name == "5NoUpConnector")
			{
				replacer = "3DCrossConnector_" + rotation + "_f";
			}
		}

		if (replacer == currentBlock && nextBlock.find("Door") == std::string::npos && currentBlock.find("Door") == std::string::npos)
		{
			std::cout << "Should not happen: " << replacer << std::endl;;
		}
	}
	if (replacer == "Void") //Temporary
	{
		replacer = "Cube_r0_f";
	}

	if (prevWasVoid && nextBlock != "None")
	{
		replacer = ReplaceBlock(prevBlock, replacer, nextBlock, prevDir, nextDir, false);
	}

	return replacer;
}

bool WFC::SetInput(std::string input)
{
	//Make sure everything is reset.
	m_entropy.clear();
	m_currentEntropy.clear();
	m_blockPossibilities.clear();

	//If the read fails it means the constraints can not generate a level.
	if (!ReadInput(input))
	{
		return false;
	}
	return true;
}

void WFC::SetDimensions(uint32_t width, uint32_t height, uint32_t depth)
{
	m_width = width;
	m_height = height;
	m_depth = depth;
}

bool WFC::ReadInput(std::string input)
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
			
			if (id.find("Door") != std::string::npos)
			{
				m_doorBlocks.push_back(id);
			}
			else if (id.find("Spawn") != std::string::npos)
			{
				m_spawnBlocks.push_back(id);
			}

			m_blockPossibilities[id].id = id;
			//Put the count in the frequency and increment the totalcount.
			m_blockPossibilities[id].count = std::stoi(line.substr(delim + 1, line.size()));
			m_totalCount += m_blockPossibilities[id].count;

			//For each direction.
			for (uint32_t i{ 0u }; i < 6; ++i)
			{
				//Go through each possibility in that direction.
				std::getline(inputFile, line);
				while (line.find(',') != std::string::npos)
				{
					delim = line.find(',');
					std::string possibility = line.substr(0, delim);
					m_blockPossibilities[id].dirPossibilities[i].push_back(possibility);
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
		block.second.count = static_cast<uint32_t>(std::ceil(block.second.count * 0.25f)); //Since all blocks are added 4 times each because of rotations.
		
		block.second.frequency = 1.0f;
	}

	return true;
}

void WFC::Propogate(uint32_t index, Room& room)
{	
	//If the index is not z = 0 we can propogate in the negative z direction.
	if (index % room.width != 0)
	{
		CheckForPropogation(index, index - 1, 2);
	}

	//If the index is not z = width we can propogate in the positive z direction.
	if (index % room.width != room.width - 1)
	{
		CheckForPropogation(index, index + 1, 3);
	}

	//If the index is not y = 0 we can propogate in the negative y direction.
	uint32_t remainder = index % (room.width * room.height);
	if (remainder >= room.width)
	{
		CheckForPropogation(index, index - room.width, 4);
	}

	//If the index is not y = height we can propogate in the positive y direction.
	if (remainder < room.width * (room.height - 1))
	{
		CheckForPropogation(index, index + room.width, 5);
	}

	//If the index is not x = 0 we can propogate in the negative x direction.
	if (index >= room.width * room.height)
	{
		CheckForPropogation(index, index - (room.width * room.height), 0);
	}

	//If the index is not x = depth we can propogate in the positive x direction.
	if (index < room.width * room.height * (room.depth - 1))
	{
		CheckForPropogation(index, index + (room.width * room.height), 1);
	}
}

//Dir is the direction from the neighborIndex TO the currentIndex.
void WFC::CheckForPropogation(uint32_t currentIndex, uint32_t neighborIndex, unsigned dir)
{
	bool removed = false;
	for (uint32_t i{ 0u }; i < m_currentEntropy[neighborIndex].possibilities.size(); ++i) //For every possibility in the neighbor cell
	{
		std::string& neighborPossibility = m_currentEntropy[neighborIndex].possibilities[i];
		bool matched = false;
		
		for (auto& possibility : m_currentEntropy[currentIndex].possibilities) //Check every possibility still left in the current cell and make sure "r" matches with atleast one of them.
		{
			if (std::count(m_blockPossibilities[neighborPossibility].dirPossibilities[dir].begin(), m_blockPossibilities[neighborPossibility].dirPossibilities[dir].end(), possibility))
			{
				matched = true;
				break;
			}
		}
		if (!matched) //If it didnt match with any possibility in the current cell we remove the possibility from the neighbor cell.
		{
			if (m_currentEntropy[neighborIndex].possibilities.size() == 1)
			{
				m_failed = true;
				break;
			}

			m_currentEntropy[neighborIndex].possibilities.erase(m_currentEntropy[neighborIndex].possibilities.begin() + i);
			i--;
			removed = true;
		}
	}
	//If something got removed we have to propagate that information.
	if (removed)
	{
		//We now have to rearrange the PQ since possibilities were removed. (Is not done during contraints.)
		if (m_priorityQueue)
		{
			if (!m_priorityQueue->Rearrange(neighborIndex, m_blockPossibilities))
			{
				m_failed = true; //Mark generation as failed if it fails to rearrange.
			}
		}

		m_recursiveStack.push(neighborIndex);
	}
}

void WFC::PropogateConstrain(uint32_t index, Room& room)
{
	//If the index is not z = 0 we can propogate in the negative z direction.
	if (index % room.width != 0)
	{
		CheckForPropogationConstrain(index, index - 1, 2);
	}

	//If the index is not z = width we can propogate in the positive z direction.
	if (index % room.width != room.width - 1)
	{
		CheckForPropogationConstrain(index, index + 1, 3);
	}

	//If the index is not y = 0 we can propogate in the negative y direction.
	uint32_t remainder = index % (room.width * room.height);
	if (remainder >= room.width)
	{
		CheckForPropogationConstrain(index, index - room.width, 4);
	}

	//If the index is not y = height we can propogate in the positive y direction.
	if (remainder < room.width * (room.height - 1))
	{
		CheckForPropogationConstrain(index, index + room.width, 5);
	}

	//If the index is not x = 0 we can propogate in the negative x direction.
	if (index >= room.width * room.height)
	{
		CheckForPropogationConstrain(index, index - (room.width * room.height), 0);
	}

	//If the index is not x = depth we can propogate in the positive x direction.
	if (index < room.width * room.height * (room.depth - 1))
	{
		CheckForPropogationConstrain(index, index + (room.width * room.height), 1);
	}
}

//Dir is the direction from the neighborIndex TO the currentIndex.
void WFC::CheckForPropogationConstrain(uint32_t currentIndex, uint32_t neighborIndex, unsigned dir)
{
	bool removed = false;
	for (uint32_t i{ 0u }; i < m_entropy[neighborIndex].possibilities.size(); ++i) //For every possibility in the neighbor cell
	{
		std::string& neighborPossibility = m_entropy[neighborIndex].possibilities[i];
		bool matched = false;

		for (auto& possibility : m_entropy[currentIndex].possibilities) //Check every possibility still left in the current cell and make sure "r" matches with atleast one of them.
		{
			if (std::count(m_blockPossibilities[neighborPossibility].dirPossibilities[dir].begin(), m_blockPossibilities[neighborPossibility].dirPossibilities[dir].end(), possibility))
			{
				matched = true;
				break;
			}
		}
		if (!matched) //If it didnt match with any possibility in the current cell we remove the possibility from the neighbor cell.
		{
			if (m_entropy[neighborIndex].possibilities.size() == 1)
			{
				m_failed = true;
				break;
			}

			m_entropy[neighborIndex].possibilities.erase(m_entropy[neighborIndex].possibilities.begin() + i);
			i--;
			removed = true;
		}
	}
	//If something got removed we have to propagate that information.
	if (removed)
	{
		m_recursiveStack.push(neighborIndex);
	}
}


void WFC::PrintLevel()
{
#ifndef _DEBUG
	return;
#endif
	std::cout << std::endl;
	std::cout << "|||||||||||||||NEWPRINT|||||||||||||||||||" << std::endl;
	unsigned int count = 1;
	for (uint32_t i{ m_width * m_height * m_depth - 1 }; i > 0; --i)
	{
		if (i % (m_width * m_height) == 0)
		{
			std::cout << std::endl;
			std::cout << "-----------------------------------------------------------" << std::endl;
		}
		else if (i % m_width == 0)
		{
			std::cout << std::endl;
		}
		if (m_generatedLevel[i] == "")
		{
			std::cout << i << ": x\t\t";
		}
		else
		{
			std::cout << i << ": " << m_generatedLevel[i] << "\t\t";
		}
	}
}
