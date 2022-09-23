#pragma once
#include "AssimpImporter.h"
#include "MaterialManager.h"
#include "ManagedAssets.h"
#include "../Audio/AudioFileReader.h"
#include "Types/GraphicsTypes.h"


namespace DOG
{
	

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

		[[nodiscard]] u32 LoadModelAsset(const std::string& path, AssetLoadFlag flag = AssetLoadFlag::VramOnly);
		[[nodiscard]] u32 LoadTexture(const std::string& path, AssetLoadFlag flag = AssetLoadFlag::None);
		[[nodiscard]] u32 LoadAudio(const std::string& path, AssetLoadFlag flag = AssetLoadFlag::None);

		MaterialManager& GetMaterialManager();

		void UnLoadAsset(u32 id, AssetUnLoadFlag flag);

		Asset* GetBaseAsset(u32 id) const;
		template<typename T>
		requires std::is_base_of_v<Asset, T>
		T* GetAsset(u32 id) const
		{
			if (m_assets.contains(id))
			{
				return static_cast<ManagedAsset<T>*>(m_assets.at(id))->Get();
			}
			else
			{
				std::cout << "Warning AssetManager::GetAsset called with invalid id as argument." << std::endl;
				return nullptr;
			}
		}
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

		[[nodiscard]] u32 AddMesh(const ImportedMesh& mesh);
		[[nodiscard]] std::vector<u32> LoadMaterials(const std::vector<ImportedMaterial>& importedMats);
		[[nodiscard]] u32 LoadTextureSTBI(const std::string& path, AssetLoadFlag flag);
		[[nodiscard]] u32 LoadTextureCommpresonator(const std::string& path, AssetLoadFlag flag);
		void MoveModelToGPU(u32 modelID, AssetLoadFlag flag);

		u32 NextKey();


	private:
		static std::unique_ptr<AssetManager> s_instance;
		static constexpr u64 MAX_AUDIO_SIZE_ASYNC = 65536;
		static std::mutex s_commandQueueMutex;
		static std::queue<std::function<void()>> s_commandQueue;
		std::unordered_map<u32, ManagedAssetBase*> m_assets;
		u32 m_lastKey = 0;


		MaterialManager m_materialManager;


		gfx::Renderer* m_renderer = nullptr;
	};
}