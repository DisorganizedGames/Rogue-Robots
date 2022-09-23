#pragma once

using u8 = unsigned char;
using u16 = unsigned short;
using u32 = unsigned int;

using i32 = int;

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

AssetMetaData GenerateMetaData(const std::filesystem::path& path, const std::string& outExt)
{
	if (!std::filesystem::is_directory(path))
	{
		FatalError("Provided path is not a directory");
	}

	AssetMetaData out = {};

	// Map each texture to its unique id
	for (auto& filePath : std::filesystem::directory_iterator(path))
	{
		auto ext = filePath.path().extension();

		switch (extensionMap[ext.string()])
		{
		case AssetType::Texture:
			out.textureMap[filePath.path().filename().replace_extension().string() + outExt] = out.trivial.countTextures++;
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
	AssetMetaData metaData = GenerateMetaData(assetPath, ext);

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
	CMP_MipSet inMipset = {};

	auto error = CMP_LoadTexture(in.string().c_str(), &inMipset);

	if (error != CMP_OK)
	{
		FatalError(std::string("Failed to read texture: ") + in.string());
	}

	KernelOptions opts = {
		.fquality = 0.f,
		.format = CMP_FORMAT_BC7,
		.threads = 8,
	};

	CMP_MipSet outMipset = {};

	error = CMP_ProcessTexture(&inMipset, &outMipset, opts, nullptr);

	if (error != CMP_OK)
	{
		FatalError(std::string("Failed to compress texture: ") + in.string());
	}

	header.width = outMipset.m_nWidth;
	header.height = outMipset.m_nHeight;

	out->write((char*)&header, sizeof(TextureHeader));
	out->write((char*)outMipset.pData, outMipset.dwDataSize);

	CMP_FreeMipSet(&inMipset);
	CMP_FreeMipSet(&outMipset);
}

