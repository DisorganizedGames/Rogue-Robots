#pragma once
#include <DOGEngine.h>
namespace pcgBlock
{
	constexpr float DIMENSION = 5.0f;
}
namespace pcgLevelNames
{
	constexpr const char* oldDefault = "..\\Offline-Tools\\PCG\\showOff_generatedLevel.txt";
	constexpr const char* tunnels = "..\\Offline-Tools\\PCG\\Tunnels_generatedLevel.txt";
	constexpr const uint32_t nrLevels = 10;
	constexpr const char pcgLevels[nrLevels][128] = {
		"WorkingLevel1.txt",
		"WorkingLevel2.txt",
		"WorkingLevel3.txt",
		"WorkingLevel4.txt",
		"WorkingLevel5.txt",
		"WorkingLevel6.txt",
		"WorkingLevel7.txt",
		"WorkingLevel8.txt",
		"WorkingLevel9.txt",
		"WorkingLevel10.txt"
	};
}
std::vector<DOG::entity> LoadLevel(std::string file); //Loads a PCG generated level.
