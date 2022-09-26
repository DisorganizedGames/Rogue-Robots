#pragma once

namespace DOG
{
	struct TextureHeader
	{
		char id[3] = { 'T', 'E', 'X' };
		u8 mips;
		u16 width;
		u16 height;
		// mip0, mip1, mip2, ... offsets u32
	};

	struct Metadata
	{
		u32 textureCount;
		u32 meshCount;
		std::vector<std::string> textureIndices;
	};
	
	// Reads and returns metadata from a .meta file
	Metadata ReadMetadata(const std::filesystem::path& metaDataFilePath)
	{
		Metadata meta = {};
		std::ifstream file(metaDataFilePath, std::ios::binary);
		assert(file);
		
		u64 dataSize = std::filesystem::file_size(metaDataFilePath);
		char* data = new char[dataSize];
		file.read(data, dataSize);
		file.close();

		auto ptr = data;

		meta.textureCount = *(u32*)ptr;
		meta.meshCount = *(u32*)(ptr += sizeof(u32));
		ptr += sizeof(u32);
		
		meta.textureIndices.resize(meta.textureCount);
		
		while (ptr < data+dataSize)
		{
			u16 index;
			index = *(u16*)ptr;
			ptr += sizeof(u16);

			std::string fileName(ptr);
			ptr += fileName.length() + 1;
			meta.textureIndices[index] = std::move(fileName);
		}

		delete[] data;

		return meta;
	}
}

