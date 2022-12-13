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
		"Generate.txt",
		"Cave.txt",
		"Bridge.txt",
		"Tunnels.txt",
		"Bamboozle.txt",
		"Crazy.txt",
		"Omega.txt",
		"Infinity.txt",
		"Abyss.txt",
		"Impossible.txt"
	};
}
std::vector<DOG::entity> LoadLevel(std::string file); //Loads a PCG generated level.
