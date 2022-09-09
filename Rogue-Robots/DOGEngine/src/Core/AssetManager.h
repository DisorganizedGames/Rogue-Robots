#pragma once
#include "AssimpImporter.h"
#include "MaterialManager.h"

namespace DOG
{
	enum class AssetLoadFlag
	{
		None = 0,
		VramOnly = 1 << 1,
		Async = 1 << 2,
	};

	enum class AssetStateFlag
	{
		Unknown = 0,
		LoadingAsync = 1 << 1,
		ExistOnCPU = 1 << 2,
		ExistOnGPU = 1 << 3,
		Evicted = 1 << 4,
	};

	inline AssetLoadFlag operator &(AssetLoadFlag l, AssetLoadFlag r)
	{
		return (AssetLoadFlag)((int)l & (int)r);
	}
	inline AssetLoadFlag operator |(AssetLoadFlag l, AssetLoadFlag r)
	{
		return (AssetLoadFlag)((int)l | (int)r);
	}

	struct Asset
	{
		virtual ~Asset() = default;
		std::string filePath;
		AssetStateFlag stateFlag = AssetStateFlag::Unknown;
	};

	struct TextureAsset : public Asset
	{
		uint32_t width;
		uint32_t height;
		uint32_t mipLevels;
		std::vector<u8> textureData;
	};

	struct MeshAsset : public Asset
	{
		std::vector<u32> indices;
		std::unordered_map<VertexAttribute, std::vector<u8>> vertexData;		
	};

	struct ModelAsset : public Asset
	{
		std::vector<u64> materialIDs;
		std::vector<SubmeshMetadata> submeshes;
		u64 meshID{0};
	};

	struct AudioAsset : public Asset
	{
		std::vector<u8> audioData;
	};

	class AssetManager
	{
	public:
		AssetManager();
		~AssetManager();

		[[nodiscard]] u64 LoadModelAsset(const std::string& path, AssetLoadFlag flag = AssetLoadFlag::None);
		[[nodiscard]] u64 LoadTexture(const std::string& path, AssetLoadFlag flag = AssetLoadFlag::None);
		[[nodiscard]] u64 LoadAudio(const std::string& path, AssetLoadFlag flag = AssetLoadFlag::None);
		[[nodiscard]] u64 AddMesh(const MeshAsset& mesh);
		[[nodiscard]] u64 AddMesh(MeshAsset&& mesh);
		[[nodiscard]] u64 AddMesh(const ImportedMesh& mesh, const std::string& pathImportedFrom);
		Asset* GetAsset(u64 id) const;

	private:
		[[nodiscard]] std::vector<u64> LoadMaterials(const std::vector<ImportedMaterial>& importedMats);

	private:
		std::unordered_map<uint64_t, std::unique_ptr<Asset>> m_assets;

		MaterialManager m_materialManager;
	};
}