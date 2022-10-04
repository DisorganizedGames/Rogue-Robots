#pragma once
#include "../Audio/AudioFileReader.h"
#include "Types/GraphicsTypes.h"
#include "../Graphics/Rendering/GraphicsBuilder.h"
namespace DOG
{
	enum class AssetLoadFlag
	{
		None = 0,
		CPUMemory = 1 << 1,
		GPUMemory = 1 << 2,
		Async = 1 << 3,
		Srgb = 1 << 4,
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

	enum class AssetUnLoadFlag
	{
		None = 0,
		TextureGPU = 1 << 1,
		TextureCPU = 1 << 2,
		MeshGPU = 1 << 3,
		MeshCPU = 1 << 4,
		AllGPU = TextureGPU | MeshGPU,
		AllCPU = TextureCPU | MeshCPU,
	};

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

	enum class AssetStateFlag
	{
		None = 0,
		ExistOnCPU = 1 << 1,
		ExistOnGPU = 1 << 2,
	};

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

	struct TextureAsset : public Asset
	{
		uint32_t width{ 0 };
		uint32_t height{ 0 };
		uint32_t mipLevels{ 0 };
		std::vector<u8> textureData;
		bool srgb = true;
		gfx::Texture textureGPU;
		gfx::TextureView textureViewGPU;
	};

	struct MeshAsset : public Asset
	{
		std::vector<u32> indices;
		std::unordered_map<VertexAttribute, std::vector<u8>> vertexData;
	};

	struct ModelAsset : public Asset
	{
		// Note that a materialIndex is not a key to an asset
		std::vector<u32> materialIndices;
		std::vector<SubmeshMetadata> submeshes;
		MeshAsset meshAsset;
		std::optional<gfx::StaticModel> gfxModel = std::nullopt;
		ImportedRig animation;
	};

	struct AudioAsset : public Asset
	{
		bool async = false;
		std::string filePath;
		WAVProperties properties = {};
		std::vector<u8> audioData;
	};

	class AssetManager;
	class ManagedAssetBase
	{
		friend AssetManager;
	public:
		virtual ~ManagedAssetBase() = default;
		virtual Asset* GetBase() = 0;
		bool CheckIfLoadingAsync();
		virtual void UnloadAsset(AssetUnLoadFlag flag) = 0;
		AssetStateFlag stateFlag = AssetStateFlag::None;
		AssetLoadFlag loadFlag = AssetLoadFlag::None;
		AssetUnLoadFlag unLoadFlag = AssetUnLoadFlag::None;
	private:
		std::atomic_signed_lock_free m_isLoadingConcurrent = 0;
	};

	template<typename T>
	requires std::is_base_of_v<Asset, T>
	class ManagedAsset : public ManagedAssetBase
	{
	public:
		ManagedAsset() = default;
		ManagedAsset(T* asset) : m_asset(asset)
		{
		}
		~ManagedAsset()
		{
			if (m_asset)
			{
				if (CheckIfLoadingAsync())
				{
					assert(false); // Does this ever happen, if so create a task to fix this.
				}

				delete m_asset;
				m_asset = nullptr;
			}
		}
		T* Get()
		{
			return m_asset;
		}
		Asset* GetBase() override
		{
			return static_cast<Asset*>(m_asset);
		}
		void UnloadAsset(AssetUnLoadFlag) override
		{
			assert(m_asset && !CheckIfLoadingAsync());
			delete m_asset;
			m_asset = nullptr;
			stateFlag = AssetStateFlag::None;
		}

	private:
		T* m_asset = nullptr;
	};


	// ManagedAsset template specialization on ModelAsset as it is the asset holding other assets and is needed for access to gpu resources.

	template<>
	class ManagedAsset<ModelAsset> : public ManagedAssetBase
	{
	public:
		ManagedAsset() = default;
		ManagedAsset(ModelAsset* asset);
		~ManagedAsset();
		ModelAsset* Get();

		Asset* GetBase() override;
		void UnloadAsset(AssetUnLoadFlag flag) override;

	private:
		ModelAsset* m_asset = nullptr;
	};



	//TextureAsset
	template<>
	class ManagedAsset<TextureAsset> : public ManagedAssetBase
	{
	public:
		ManagedAsset() = default;
		ManagedAsset(TextureAsset* asset);
		~ManagedAsset();

		TextureAsset* Get();
		Asset* GetBase() override;
		void UnloadAsset(AssetUnLoadFlag flag) override;

	private:
		TextureAsset* m_asset = nullptr;
	};
}
