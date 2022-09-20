#pragma once
#include "GraphicsTypes.h"

namespace DOG
{
	struct ImportedMaterial
	{
		std::string materialName;
		std::string albedoPath;
		std::string normalMapPath;
		std::string metallicRoughnessPath;
		std::string emissivePath;
		float albedoFactor[4] = { 1, 1, 1, 1 };
		float metallicFactor = 0;
		float roughnessFactor = 1;
		float emissiveFactor[3] = { 0, 0, 0 };
	};

	struct ImportedTextureFileMip
	{
		std::vector<u8> data;
		u32 width{ 0 };
		u32 height{ 0 };
	};

	struct ImportedTextureFile
	{
		std::vector<ImportedTextureFileMip> dataPerMip;
	};


	struct ImportedMesh
	{
		std::unordered_map<VertexAttribute, std::vector<u8>> vertexData;
		std::vector<u32> indices;
	};

	struct ImportedModel
	{
		std::vector<ImportedMaterial> materials;
		std::vector<SubmeshMetadata> submeshes;
		ImportedMesh mesh;
	};
}