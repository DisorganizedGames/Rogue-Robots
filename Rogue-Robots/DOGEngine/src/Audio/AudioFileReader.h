#pragma once

namespace DOG
{

	class WAVFileReader
	{
	public:
		WAVFileReader() = default;
		WAVFileReader(const std::filesystem::path& path);
		WAVEFORMATEX ReadWFXProperties();
		std::vector<u8> ReadDataChunk(u32 chunkSize);
		std::vector<u8> ReadFull();

	private:
		enum class ChunkType
		{
			RIFF,
			Format,
			Data,
			List,
			Junk,
			Info,
			ResU,
			Bext,
			EndOfFile,
		};

	private:
		std::ifstream m_file;
		u64 m_fileSize = 0;
		u32 m_dataChunkBytesLeft = 0;

	private:
		void SkipChunk(ChunkType type);
		WAVEFORMATEX ReadFormat();
		ChunkType ReadNextChunkType();
	};

};
