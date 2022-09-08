#pragma once

namespace DOG
{
	constexpr const u32 WAV_RIFF_HEADER_SIZE = 12;
	constexpr const u32 WAV_FORMAT_CHUNK_SIZE = 24;
	constexpr const u32 LIST_CHUNK_SIZE = 4;
	constexpr const u32 INFO_CHUNK_SIZE = 8;
	constexpr const u32 DATA_CHUNK_SIZE = 4;
	constexpr const u32 DATA_CHUNK_DATA = 8;

	enum class WAVFormat : u16
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

	static WAVProperties ReadWAVProperties(const std::vector<u8>& data)
	{
		if (data.size() < WAV_RIFF_HEADER_SIZE + WAV_FORMAT_CHUNK_SIZE)
		{
			throw std::runtime_error("WAV File is too small to contain a header");
		}
		if (memcmp(data.data(), "RIFF", 4) != 0 || memcmp(&data[8], "WAVE", 4) != 0)
		{
			throw std::runtime_error("Provided file is not a WAV file");
		}
		if (memcmp(&data[12], "fmt", 3) != 0)
		{
			throw std::runtime_error("Second chunk of WAV file is not a format chunk");
		}

		WAVProperties out = {
			.format = *(WAVFormat*)(&data[20]),
			.channels = *(u16*)(&data[22]),
			.sampleRate = *(u32*)(&data[24]),
			.bps = *(u16*)(&data[34]),
		};
		if (out.format != WAVFormat::PCM)
		{
			throw std::runtime_error("WAV file is not using the PCM format");
		}

		return out;
	}

	static std::vector<u8> ReadLISTChunk(const u8* data)
	{
		data += 4; // Skip "LIST"
		u32 bytesLeft = *(u32*)data;
		data += 4; // Move past chunk size

		while (bytesLeft)
		{
			if (memcmp(data, "INFO", 4) == 0)
			{
				u32 chunkSize = *(u32*)&data[INFO_CHUNK_SIZE] + INFO_CHUNK_SIZE + sizeof(u32);

				bytesLeft -= chunkSize;
				data += chunkSize;
			}
			else
			{
				assert(false);
			}
		}

		return std::vector<u8>();
	}

	static std::pair<std::vector<u8>, u64> ReadWAVChunk(const u8* data)
	{
		std::vector<u8> soundData;
		u64 chunkSize = 0;

		// Handle the current chunk type (LIST or data)
		if (memcmp(data, "LIST", 4) == 0)
		{
			chunkSize = (u64)8 + *(u32*)&data[LIST_CHUNK_SIZE];

			soundData = ReadLISTChunk(data);
		}
		else if (memcmp(data, "data", 4) == 0)
		{
			chunkSize = (u64)8 + *(u32*)&data[DATA_CHUNK_SIZE];

			soundData.resize(chunkSize - 8);

			memcpy(soundData.data(), &data[DATA_CHUNK_DATA], soundData.size());
		}
		else
		{
			assert(false);
		}

		return {soundData, chunkSize};
	}

	static std::vector<u8> ReadWAVData(const std::vector<u8>& data)
	{
		u64 bytesLeft = data.size();
		u64 index = 0;
		std::vector<u8> outData;
		while (bytesLeft)
		{
			auto [chunkData, chunkSize] = ReadWAVChunk(&data[index]);
			outData.insert(outData.end(), chunkData.begin(), chunkData.end());
			bytesLeft -= chunkSize;
			index += chunkSize;
		}
		return outData;
	}

	// Reads the provided WAV file and returns a tuple of it's properties, a pointer to the sound data, and
	// the size of said data.
	static std::pair<WAVProperties, std::vector<u8>> ReadWAV(const std::string& filePath)
	{
		std::ifstream file(filePath, std::ios::binary);
		if (!file)
		{
			// TODO: Make a proper error (FileNotFoundError?)
			throw std::runtime_error(std::string("Failed to open file: ") + filePath);
		}

		const u64 fileSize = std::filesystem::file_size(filePath);

		std::vector<u8> wavHeader(WAV_RIFF_HEADER_SIZE + WAV_FORMAT_CHUNK_SIZE);
		file.read((char*)(wavHeader.data()), wavHeader.size());

		WAVProperties properties = ReadWAVProperties(wavHeader);

		std::vector<u8> wavData(fileSize - wavHeader.size());
		file.read((char*)(wavData.data()), wavData.size());

		return {properties, ReadWAVData(wavData)};
	}
}

