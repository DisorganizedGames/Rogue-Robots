#pragma once
#include "ShapeCreator.h"
#include "AssimpImporter.h"
#include "MaterialManager.h"
#include "ManagedAssets.h"
#include "../Audio/AudioFileReader.h"
#include "Types/GraphicsTypes.h"


namespace DOG
{
	
	struct AssetFlags
	{
		AssetLoadFlag loadFlag;
		AssetUnLoadFlag unloadFlag;
		AssetStateFlag stateFlag;
	};

	namespace gfx
	{
		class Renderer;
	}
	class AssetManager
	{
		friend ManagedAsset<TextureAsset>;
	public:
		static void Initialize(gfx::Renderer* renderer);
		static void Destroy();
		static AssetManager& Get();

		~AssetManager();

		[[nodiscard]] u32 LoadModelAsset(const std::string& path, AssetLoadFlag flag = (AssetLoadFlag)(AssetLoadFlag::GPUMemory | AssetLoadFlag::Async));
		[[nodiscard]] u32 LoadShapeAsset(const Shape shape, const u32 tessFactor1, const u32 tessFactor2 = 3, AssetLoadFlag flag = (AssetLoadFlag)(AssetLoadFlag::GPUMemory | AssetLoadFlag::Async));
		[[nodiscard]] u32 LoadTexture(const std::string& path, AssetLoadFlag flag = AssetLoadFlag::None);
		[[nodiscard]] u32 LoadAudio(const std::string& path, AssetLoadFlag flag = AssetLoadFlag::CPUMemory);

		MaterialManager& GetMaterialManager();

		void UnLoadAsset(u32 id, AssetUnLoadFlag flag);

		Asset* GetBaseAsset(u32 id) const;
		template<typename T>
		requires std::is_base_of_v<Asset, T>
		T* GetAsset(u32 id) const
		{
			if (!m_assets.contains(id))
			{
				std::cout << "Warning AssetManager::GetAsset called with invalid id as argument." << std::endl;
				return nullptr;
			}
			else if (m_assets.at(id)->CheckIfLoadingAsync())
			{
				return nullptr;
			}
			else if (m_assets.at(id)->stateFlag == AssetStateFlag::None)
			{
				return nullptr;
			}
			else
			{
				return static_cast<ManagedAsset<T>*>(m_assets.at(id))->Get();
			}
		}

		AssetFlags GetAssetFlags(u32 id) const;

		void Update();

		template<typename T, typename ...Args>
		static void AddCommand(T&& command, Args&&... args)
		{
			s_commandQueueMutex.lock();
			s_commandQueue.emplace(
				[command = std::forward<T>(command), args = std::make_tuple(std::forward<Args>(args)...)]() mutable
				{
					std::apply(command, std::move(args));
				});
			s_commandQueueMutex.unlock();
		}

		void ExecuteCommands();
	private:
		AssetManager();

		void LoadModelAssetInternal(const std::string& path, u32 id, ModelAsset* assetOut);
		void LoadTextureAssetInternal(const std::string& path, u32 id, TextureAsset* assetOut);
		void TextureMoveToGpuCallHelper(u32 id);
		void LoadShapeAssetInternal(const std::string& path, const Shape shape, const u32 tessFactor1, const u32 tessFactor2, u32 id, ModelAsset* assetOut);

		[[nodiscard]] u32 AddMesh(const ImportedMesh& mesh);
		[[nodiscard]] std::vector<u32> LoadMaterials(const std::vector<ImportedMaterial>& importedMats, AssetLoadFlag flag);
		static void LoadTextureSTBI(const std::string& path, TextureAsset* assetOut);
		static void LoadTextureCommpresonator(const std::string& path, TextureAsset* assetOut, bool srgb = false);
		void MoveModelToGPU(u32 modelID);

		void MoveTextureToGPU(u32 textureID);

		u32 NextKey();

		// if assetIDOut == 0 then the component needs needs to be added to m_assets
		bool ShapeAssetNeedsToBeLoaded(const std::string& path, AssetLoadFlag loadFlag, u32& assetIDOut);
		bool AssetNeedsToBeLoaded(const std::string& path, AssetLoadFlag flag, u32& assetIDOut);

		gfx::GraphicsBuilder& GetGraphicsBuilder();

		void ImguiLoadModel(bool& open);
		static void AssetManagerGUI(bool& open);

		template<typename T>
		requires std::is_base_of_v<Asset, T>
		T* GetAssetUnsafe(u32 id) const
		{
			if (!m_assets.contains(id))
			{
				std::cout << "Warning AssetManager::GetAsset called with invalid id as argument." << std::endl;
				return nullptr;
			}
			else
			{
				return static_cast<ManagedAsset<T>*>(m_assets.at(id))->Get();
			}
		}

	private:
		static std::unique_ptr<AssetManager> s_instance;
		static constexpr u64 MAX_AUDIO_SIZE_ASYNC = 65536;
		static std::mutex s_commandQueueMutex;
		static std::queue<std::function<void()>> s_commandQueue;
		std::unordered_map<u32, ManagedAssetBase*> m_assets;
		u32 m_lastKey = 0;

		std::unordered_map<std::string, u32> m_pathTOAssetID;

		MaterialManager m_materialManager;


		gfx::Renderer* m_renderer = nullptr;
	};
}