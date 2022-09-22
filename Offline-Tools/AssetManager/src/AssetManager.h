#pragma once

using u8 = unsigned char;
using u16 = unsigned short;
using u32 = unsigned int;

void FatalError(const std::string& msg)
{
	std::cerr << "\x1b[4;31m" << "Error: " << msg << "\x1b[0m" << std::endl;
	exit(-1);
}

struct AssetMetaData
{
	struct TrivialData
	{
		u16 countTextures;
		u16 countMeshes;
	} trivial;

	std::map<std::string, u16> textureMap;
};

enum class AssetType
{
	Texture,
	Mesh,
	Audio
};

struct TextureHeader
{
	char id[3] = {'T', 'E', 'X'};
	u8 bytesPerPixel;
	u16 width;
	u16 height;
};

std::map<std::string, AssetType> extensionMap = {
	{".jpg", AssetType::Texture},
	{".png", AssetType::Texture},
};

AssetMetaData GenerateMetaData(const std::filesystem::path& path)
{
	if (!std::filesystem::is_directory(path))
	{
		FatalError("Provided path is not a directory");
	}

	AssetMetaData out = {};

	for (auto& filePath : std::filesystem::directory_iterator(path))
	{
		auto ext = filePath.path().extension();

		switch (extensionMap[ext.string()])
		{
		case AssetType::Texture:
			out.textureMap[filePath.path().filename().string()] = out.trivial.countTextures++;
			break;
		default:
			break;
		}
	}

	return out;
}

void WriteTexture(const std::filesystem::path& in, std::ofstream* out);

void WriteAssetFiles(const std::filesystem::path& assetPath, const std::filesystem::path& outPath, const std::string& ext = ".dog")
{
	AssetMetaData metaData = GenerateMetaData(assetPath);

	if (!std::filesystem::is_directory(outPath))
	{
		std::filesystem::create_directory(outPath);
	}

	auto& texMap = metaData.textureMap;

	for (auto& filePath : std::filesystem::directory_iterator(assetPath))
	{
		std::ofstream ofile(outPath.string() + std::string("/") + filePath.path().filename().replace_extension().string() + ext, std::ios::binary);

		auto extension = filePath.path().extension();
		switch (extensionMap[extension.string()])
		{
		case AssetType::Texture:
			WriteTexture(filePath, &ofile);
			break;
		default:
			FatalError(std::string("Extension not recognized: ") + extension.string());
			break;
		}
	}

	std::ofstream metaFile(outPath.string() + std::string("/") + "assets.meta", std::ios::binary);
	metaFile.write((char*)&metaData.trivial, sizeof(metaData.trivial));
	for (auto& item: metaData.textureMap)
	{
		metaFile.write((char*)(&item.second), sizeof(item.second));
		metaFile << item.first << '\0';
	}
}

void WriteTexture(const std::filesystem::path& in, std::ofstream* out)
{
	TextureHeader header = {};
	CMP_MipSet mipset = {};

	auto error = CMP_LoadTexture(in.string().c_str(), &mipset);

	if (error != CMP_OK)
	{
		FatalError(std::string("Failed to read texture: ") + in.string());
	}

	header.width = mipset.m_nWidth;
	header.height = mipset.m_nHeight;
	header.bytesPerPixel = mipset.m_nChannels;

	out->write((char*)&header, sizeof(TextureHeader));

	out->write((char*)mipset.pData, mipset.dwDataSize);
}

