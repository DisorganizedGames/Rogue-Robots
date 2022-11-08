#pragma once
#include <DOGEngine.h>
namespace pcgLevelNames
{
	constexpr const char* oldDefault = "..\\Offline-Tools\\PCG\\showOff_generatedLevel.txt";
	constexpr const char* tunnels = "..\\Offline-Tools\\PCG\\Tunnels_generatedLevel.txt";
	constexpr const char* testRooms = "..\\Offline-Tools\\PCG\\testRooms_generatedLevel.txt";
}
std::vector<DOG::entity> LoadLevel(std::string file); //Loads a PCG generated level.
