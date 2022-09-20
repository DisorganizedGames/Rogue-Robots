#pragma once
#include "AssimpImporter.h"
#include "MaterialManager.h"
#include "../Audio/AudioFileReader.h"
#include "Types/GraphicsTypes.h"

namespace DOG
{
	enum class AssetLoadFlag
	{
		None = 0,
		VramOnly = 1 << 1,
		Async = 1 << 2,
		GenMips = 1 << 3,
		Srgb = 1 << 4,
	};

	enum class AssetUnLoadFlag
	{
		RemoveFromVram = 1 << 1,
		RemoveFromRam = 1 << 2,
		RemoveFromAllMemoryTypes = RemoveFromVram | RemoveFromRam,
	};

	enum class AssetStateFlag
	{
		Unknown = 0,
		LoadingAsync = 1 << 1,
		ExistOnCPU = 1 << 2,
		ExistOnGPU = 1 << 3,
		Evicted = 1 << 4,
	};

	inline int operator &(AssetLoadFlag l, AssetLoadFlag r)
	{
		return (int)l & (int)r;
	}
	inline int operator |(AssetLoadFlag l, AssetLoadFlag r)
	{
		return (int)l | (int)r;
	}

	inline void operator |= (AssetLoadFlag& l, AssetLoadFlag r)
	{
		l = (AssetLoadFlag)(l | r);
	}

	inline void operator |= (AssetLoadFlag& l, int r)
	{
		l = (AssetLoadFlag)((int)l | r);
	}

	inline void operator &= (AssetLoadFlag& l, AssetLoadFlag r)
	{
		l = (AssetLoadFlag)(l & r);
	}

	inline void operator &= (AssetLoadFlag& l, int r)
	{
		l = (AssetLoadFlag)((int)l & r);
	}

	inline int operator~ (AssetLoadFlag e)
	{
		return ~(int)e;
	}

	inline int operator &(AssetUnLoadFlag l, AssetUnLoadFlag r)
	{
		return (int)l & (int)r;
	}
	inline int operator |(AssetUnLoadFlag l, AssetUnLoadFlag r)
	{
		return (int)l | (int)r;
	}

	inline void operator |= (AssetUnLoadFlag& l, AssetUnLoadFlag r)
	{
		l = (AssetUnLoadFlag)(l | r);
	}

	inline void operator |= (AssetUnLoadFlag& l, int r)
	{
		l = (AssetUnLoadFlag)((int)l | r);
	}

	inline void operator &= (AssetUnLoadFlag& l, AssetUnLoadFlag r)
	{
		l = (AssetUnLoadFlag)(l & r);
	}

	inline void operator &= (AssetUnLoadFlag& l, int r)
	{
		l = (AssetUnLoadFlag)((int)l & r);
	}

	inline int operator~ (AssetUnLoadFlag e)
	{
		return ~(int)e;
	}

	inline int operator &(AssetStateFlag l, AssetStateFlag r)
	{
		return (int)l & (int)r;
	}

	inline int operator |(AssetStateFlag l, AssetStateFlag r)
	{
		return (int)l | (int)r;
	}

	inline void operator |= (AssetStateFlag& l, AssetStateFlag r)
	{
		l = (AssetStateFlag)(l | r);
	}

	inline void operator |= (AssetStateFlag& l, int r)
	{
		l = (AssetStateFlag)((int)l | r);
	}

	inline void operator &= (AssetStateFlag& l, AssetStateFlag r)
	{
		l = (AssetStateFlag)(l & r);
	}

	inline void operator &= (AssetStateFlag& l, int r)
	{
		l = (AssetStateFlag)((int)l & r);
	}

	inline int operator~ (AssetStateFlag e)
	{
		return ~(int)e;
	}

	struct Asset
	{
		virtual ~Asset() = default;
	};

	class ManagedAsset
	{
	public:
		ManagedAsset() = default;
		ManagedAsset(AssetStateFlag flag, Asset* asset);
		~ManagedAsset();
		Asset* Get() const;
		bool CheckIfLoadingAsync() const;
		void ReleaseAsset();

		mutable AssetStateFlag stateFlag = AssetStateFlag::Unknown;

	private:
		Asset* m_asset = nullptr;
		std::atomic_signed_lock_free m_isLoadingConcurrent = 0;
	};

	

	struct TextureAsset : public Asset
	{
		uint32_t width{ 0 };
		uint32_t height{ 0 };
		uint32_t mipLevels{ 0 };
		std::vector<u8> textureData;
		bool srgb = true;
	};

	struct MeshAsset : public Asset
	{
		std::vector<u32> indices;
		std::unordered_map<VertexAttribute, std::vector<u8>> vertexData;		
	};

	struct ModelAsset : public Asset
	{
		// Note that a materialIndex is not a key to an asset
		std::vector<u64> materialIndices;
		std::vector<SubmeshMetadata> submeshes;
		u64 meshID{0};
		gfx::StaticModel gfxModel;

	};

	struct AudioAsset : public Asset
	{
		bool async = false;
		std::string filePath;
		WAVProperties properties;
		std::vector<u8> audioData;
	};

	namespace gfx
	{
		class Renderer;
	}
	class AssetManager
	{
	public:
		static void Initialize(gfx::Renderer* renderer);
		static void Destroy();
		static AssetManager& Get();

		~AssetManager();

		[[nodiscard]] u64 LoadModelAsset(const std::string& path, AssetLoadFlag flag = AssetLoadFlag::None);
		[[nodiscard]] u64 LoadTexture(const std::string& path, AssetLoadFlag flag = AssetLoadFlag::None);
		[[nodiscard]] u64 LoadAudio(const std::string& path, AssetLoadFlag flag = AssetLoadFlag::None);

		void UnLoadAsset(u64 id, AssetUnLoadFlag flag = AssetUnLoadFlag::RemoveFromAllMemoryTypes);

		Asset* GetAsset(u64 id) const;

	private:
		AssetManager();

		[[nodiscard]] u64 AddMesh(const ImportedMesh& mesh);
		[[nodiscard]] std::vector<u64> LoadMaterials(const std::vector<ImportedMaterial>& importedMats);
		[[nodiscard]] u64 LoadTextureSTBI(const std::string& path, AssetLoadFlag flag);
		[[nodiscard]] u64 LoadTextureCommpresonator(const std::string& path, AssetLoadFlag flag);
		void MoveModelToGPU(u64 modelID);


	private:
		static std::unique_ptr<AssetManager> s_instance;
		static constexpr u64 MAX_AUDIO_SIZE_ASYNC = 65536;

		std::unordered_map<u64, ManagedAsset*> m_assets;

		MaterialManager m_materialManager;

		gfx::Renderer* m_renderer = nullptr;
	};
}