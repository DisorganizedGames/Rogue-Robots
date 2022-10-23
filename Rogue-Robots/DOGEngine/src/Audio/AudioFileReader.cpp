#include "AudioFileReader.h"

using namespace DOG;

WAVFileReader::WAVFileReader(const std::filesystem::path& path) : m_file(path, std::ios::binary)
{
	m_fileSize = std::filesystem::file_size(path);
}

WAVEFORMATEX WAVFileReader::ReadWFXProperties()
{
	i64 curPos = m_file.tellg();
	m_file.seekg(0, std::ios_base::beg);
	ChunkType chunkType = ChunkType::RIFF;

	while ((chunkType = ReadNextChunkType()) != ChunkType::Format)
	{
		//std::cout << "Skipping chunk type: " << (i32)chunkType << std::endl;
		SkipChunk(chunkType);
	}
	ReadFormat();
	m_file.seekg(curPos, std::ios_base::beg);
	return m_wfx;
}

std::vector<u8> WAVFileReader::ReadDataChunk(u32 chunkSize)
{
	if (m_dataChunkBytesLeft == 0)
	{
		ChunkType chunkType = ChunkType::RIFF;
		while (((chunkType = ReadNextChunkType()) != ChunkType::Data) && (chunkType != ChunkType::EndOfFile))
		{
			SkipChunk(chunkType);
		}

		if (chunkType == ChunkType::EndOfFile) return {};

		m_file.read((char*)&m_dataChunkBytesLeft, sizeof(u32));
		m_dataSize = m_dataChunkBytesLeft;

		if (m_dataStart == 0)
		{
			m_dataStart = m_file.tellg();
			m_dataStart -= 4;
		}
	}
	chunkSize = std::min(m_dataChunkBytesLeft, chunkSize);

	std::vector<u8> outData(chunkSize);
	m_file.read((char*)outData.data(), chunkSize);

	m_dataChunkBytesLeft -= chunkSize;
	return outData;
}

std::vector<u8> WAVFileReader::ReadFull()
{
	std::vector<u8> outData;
	ChunkType chunkType = ChunkType::RIFF;
	while ((chunkType != ChunkType::EndOfFile))
	{
		while (((chunkType = ReadNextChunkType()) != ChunkType::Data) && (chunkType != ChunkType::EndOfFile))
		{
			SkipChunk(chunkType);
		}

		if (chunkType == ChunkType::EndOfFile) break;

		u32 chunkSize;
		m_file.read((char*)&chunkSize, sizeof(u32));

		std::vector<u8> chunkData(chunkSize);
		m_file.read((char*)chunkData.data(), chunkSize);

		std::copy(chunkData.begin(), chunkData.end(), std::back_inserter(outData));
	}

	return outData;
}

u32 WAVFileReader::DataSize()
{
	if (m_dataSize)
	{
		return m_dataSize;
	}

	return 0;
}

void WAVFileReader::SeekToSample(u32 sample)
{
	if (m_wfx.wFormatTag == 0)
	{
		ReadWFXProperties();
	}
	if (m_dataStart == 0)
	{
		ReadDataChunk(0); // Dummy read to find data chunk
	}

	auto sampleSize = m_wfx.nBlockAlign;
	m_file.seekg(m_dataStart);
	m_file.read((char*)&m_dataChunkBytesLeft, sizeof(u32));

	m_file.seekg(sample*sampleSize, std::ios_base::cur);

	m_dataChunkBytesLeft -= sample;
}

void WAVFileReader::SkipChunk(ChunkType type)
{
	constexpr u32 RIFF_CHUNK_SIZE = 12;

	u32 skip = 0;
	if (type == ChunkType::RIFF)
	{
		skip = RIFF_CHUNK_SIZE;
		skip -= 4;
	}
	else if (type == ChunkType{ -1 })
	{
		m_file.read((char*)&skip, sizeof(u32));
		//std::cout << "Skipping unknown chunk: " << skip << " bytes" << std::endl;
	}
	else
	{
		m_file.read((char*)&skip, sizeof(u32));
	}
	skip += (skip % 2);
	m_file.seekg((i32)skip, std::ios_base::cur);
	auto tellg = m_file.tellg();
	//std::cout << "Skipping " << skip << " bytes" << std::endl;
}

void WAVFileReader::ReadFormat()
{
	if (m_wfx.wFormatTag)
		return;

	constexpr u64 FORMAT_CHUNK_SIZE = 24;
	u32 chunkSize = 0;
	m_file.read((char*)&chunkSize, sizeof(u32));

	m_file.read((char*)&m_wfx, FORMAT_CHUNK_SIZE - 8);
}

WAVFileReader::ChunkType WAVFileReader::ReadNextChunkType()
{
	auto tellg = m_file.tellg();
	if (m_file.tellg() == static_cast<std::streampos>(m_fileSize))
	{
		return ChunkType::EndOfFile;
	}

	i32 type = 0x0;
	m_file.read((char*)&type, sizeof(i32));

	// Compare to little-endian int representation of chunk types
	if (type == 0x46464952)
	{
		//std::cout << "Chunk type is RIFF" << std::endl;
		return ChunkType::RIFF;
	}
	if (type == 0x20746D66)
	{
		//std::cout << "Chunk type is Format" << std::endl;
		return ChunkType::Format;
	}
	if (type == 0x4B4E554A)
	{
		//std::cout << "Chunk type is JUNK" << std::endl;
		return ChunkType::Junk;
	}
	if (type == 0x5453494C)
	{
		//std::cout << "Chunk type is LIST" << std::endl;
		return ChunkType::List;
	}
	if (type == 0x61746164)
	{
		//std::cout << "Chunk type is data" << std::endl;
		return ChunkType::Data;
	}
	if (type == 0x494E4679)
	{
		//std::cout << "Chunk type is INFO" << std::endl;
		return ChunkType::Info;
	}
	if (type == 0x55736552)
	{
		//std::cout << "Chunk type is ResU" << std::endl;
		return ChunkType::ResU;
	}
	if (type == 0x74786562)
	{
		//std::cout << "Chunk type is bext" << std::endl;
		return ChunkType::Bext;
	}
	else
	{
		//char charRep[5] = { 0 };
		//memcpy(charRep, &type, sizeof(type));
		//std::cout << "Chunk type is " << charRep << std::endl;
		//std::cout << "Skipping" << std::endl;
		SkipChunk(ChunkType{ -1 });
		return ReadNextChunkType();
	}
}
