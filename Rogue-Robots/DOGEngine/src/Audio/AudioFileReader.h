#pragma once

#include <stdexcept>
#include <filesystem>
#include "../common/Utils.h"
#include <memory>

namespace DOG
{
	constexpr const u64 WAV_HEADER_LENGTH = 44;

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

	//Reads the provided WAV file and returns a tuple of it's properties, a pointer to the sound data, and
	// the size of said data.
	static std::tuple<WAVProperties, std::unique_ptr<u8>, u64> ReadWAV(const char* filePath)
	{
		std::ifstream file(filePath, std::ios::binary);
		if (!file)
		{
			// TODO: Make a proper error (FileNotFoundError?)
			throw std::runtime_error(std::string("Failed to open file: ") + filePath);
		}

		const u64 fileSize = std::filesystem::file_size(filePath);
		const size_t dataSize = fileSize - WAV_HEADER_LENGTH;

		std::unique_ptr<u8> wavHeader(new u8[WAV_HEADER_LENGTH]);
		std::unique_ptr<u8> data(new u8[dataSize]);

		file.read(reinterpret_cast<char*>(wavHeader.get()), WAV_HEADER_LENGTH);
		file.read(reinterpret_cast<char*>(data.get()), dataSize);

		WAVProperties properties = ReadWAVProperties(wavHeader.get(), WAV_HEADER_LENGTH);

		return {properties, std::move(data), dataSize};
	}
}

