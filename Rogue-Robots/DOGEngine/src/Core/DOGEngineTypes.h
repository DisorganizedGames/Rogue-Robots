#pragma once
namespace DOG
{
	enum class MaterialTextureType
	{
		Albedo = 0,
		NormalMap,
		MetallicRoughness,
		Emissive
	};

	enum class VertexAttribute
	{
		Position,
		Normal,
		UV,
		Tangent
	};

	struct SubmeshMetadata
	{
		u32 vertexStart{ 0 };
		u32 vertexCount{ 0 };
		u32 indexStart{ 0 };
		u32 indexCount{ 0 };
	};

	struct Mesh
	{
		u64 handle{ 0 };
	};

	struct MeshContainer
	{
		Mesh mesh;
		u32 numSubmeshes{ 0 };

		u32 managerID{ UINT_MAX };
	};

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