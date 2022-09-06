#pragma once

#include <stdexcept>
#include <filesystem>
#include "../common/Utils.h"
#include <memory>

namespace DOG
{
	constexpr const u64 WAV_HEADER_LENGTH = 36;

	enum class WAVFormat : u32
	{
		PCM = 1,
	};

	struct WAVProperties
	{
		WAVFormat format;
		u16 channels;
		u32 sampleRate;
		u16 bps;
	};

	static WAVProperties ReadWAVProperties(const u8* data, u64 dataSize)
	{
		if (dataSize < WAV_HEADER_LENGTH)
		{
			throw std::runtime_error("WAV File is too small to contain a header");
		}
		if (memcmp(data, "RIFF", 4) != 0 || memcmp(&data[8], "WAVE", 4) != 0)
		{
			throw std::runtime_error("Provided file is not a WAV file");
		}
		
		
		WAVProperties out = {
			.format = *reinterpret_cast<const WAVFormat*>(&data[8]),
			.channels = *reinterpret_cast<const u16*>(&data[22]),
			.sampleRate = *reinterpret_cast<const u32*>(&data[24]),
			.bps = *reinterpret_cast<const u16*>(&data[34]),
		};
		return out;
	}
	
	// Reads the properties of the specified WAV file.
	// Does NOT read the whole file, only enough to gleam the header information needed.
	static WAVProperties ReadWAVProperties(const char* filePath)
	{
		std::ifstream file(filePath, std::ios::binary);
		if (!file)
		{
			// TODO: Make a proper error (FileNotFoundError?)
			throw std::runtime_error(std::string("Failed to open file: ") + filePath);
		}

		u64 fileSize = std::filesystem::file_size(filePath);
		std::unique_ptr<u8> data(new u8[fileSize]);
		file.read(reinterpret_cast<char*>(data.get()), WAV_HEADER_LENGTH);
		return ReadWAVProperties(data.get(), WAV_HEADER_LENGTH);
	}

	static std::tuple<WAVProperties, std::unique_ptr<u8>, u64> ReadWAV(const char* filePath)
	{
		std::ifstream file(filePath, std::ios::binary);
		if (!file)
		{
			// TODO: Make a proper error (FileNotFoundError?)
			throw std::runtime_error(std::string("Failed to open file: ") + filePath);
		}

		u64 fileSize = std::filesystem::file_size(filePath);
		std::unique_ptr<u8> data(new u8[fileSize]);
		file.read(reinterpret_cast<char*>(data.get()), fileSize);

		WAVProperties properties = ReadWAVProperties(data.get(), fileSize);

		std::unique_ptr<u8> outData(new u8[fileSize-44]);
		std::copy_n(data.get()+44, fileSize-44, outData.get());

		return {properties, std::move(outData), fileSize-44};
	}
}

