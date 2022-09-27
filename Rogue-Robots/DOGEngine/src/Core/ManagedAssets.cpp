#include "ManagedAssets.h"
#include "AssetManager.h"

namespace DOG
{
	bool ManagedAssetBase::CheckIfLoadingAsync()
	{
		if (loadFlag & AssetLoadFlag::Async)
		{
			if (m_isLoadingConcurrent == 1)
				return true;
			else
				loadFlag &= ~AssetLoadFlag::Async;
		}
		return false;
	}



	// ManagedAsset<ModelAsset>----------------------

	ManagedAsset<ModelAsset>::ManagedAsset(ModelAsset* asset) : m_asset(asset)
	{
	}

	ManagedAsset<ModelAsset>::~ManagedAsset()
	{
		if (m_asset)
		{
			while (CheckIfLoadingAsync())
			{
				std::cout << "blocking in wait of async function to complete" << std::endl;
			}

			delete m_asset;
			m_asset = nullptr;
		}
	}

	ModelAsset* ManagedAsset<ModelAsset>::Get()
	{
		if (CheckIfLoadingAsync())
			return nullptr;
		else
			return m_asset;
	}

	Asset* ManagedAsset<ModelAsset>::GetBase()
	{
		if (CheckIfLoadingAsync())
			return nullptr;
		else
			return static_cast<Asset*>(m_asset);
	}

	void ManagedAsset<ModelAsset>::UnloadAsset(AssetUnLoadFlag flag)
	{
		assert(m_asset && !CheckIfLoadingAsync());

		if (flag & AssetUnLoadFlag::MeshCPU)
		{
			m_asset->submeshes.clear();
			std::vector<SubmeshMetadata>().swap(m_asset->submeshes); // This will release the memory

			m_asset->meshAsset = MeshAsset();
		}
		if (flag & AssetUnLoadFlag::TextureCPU)
		{
			auto& matMan = AssetManager::Get().GetMaterialManager();
			for (auto& matIndex : m_asset->materialIndices)
			{
				const Material& mat = matMan.GetMaterial(matIndex);
				AssetManager::Get().UnLoadAsset(mat.albedo, flag);
				AssetManager::Get().UnLoadAsset(mat.emissive, flag);
				AssetManager::Get().UnLoadAsset(mat.normalMap, flag);
				AssetManager::Get().UnLoadAsset(mat.metallicRoughness, flag);
			}
		}
	}




	//TextureAsset--------------------------

	ManagedAsset<TextureAsset>::ManagedAsset(TextureAsset* asset) : m_asset(asset)
	{
	}

	ManagedAsset<TextureAsset>::~ManagedAsset()
	{
		if (m_asset)
		{
			while (CheckIfLoadingAsync())
			{
				std::cout << "blocking in wait of async function to complete" << std::endl;
			}

			delete m_asset;
			m_asset = nullptr;
		}
	}

	TextureAsset* ManagedAsset<TextureAsset>::Get()
	{
		if (CheckIfLoadingAsync())
			return nullptr;
		else
			return m_asset;
	}

	Asset* ManagedAsset<TextureAsset>::GetBase()
	{
		if (CheckIfLoadingAsync())
			return nullptr;
		else
			return static_cast<Asset*>(m_asset);
	}

	void ManagedAsset<TextureAsset>::UnloadAsset(AssetUnLoadFlag flag)
	{
		assert(m_asset && !CheckIfLoadingAsync());

		if (flag & AssetUnLoadFlag::TextureCPU && stateFlag & AssetStateFlag::ExistOnCPU)
		{
			m_asset->textureData.clear();
			std::vector<u8>().swap(m_asset->textureData); // This will release the memory
			stateFlag &= ~AssetStateFlag::ExistOnCPU;
		}

		if (flag & AssetUnLoadFlag::TextureGPU && stateFlag & AssetStateFlag::ExistOnGPU)
		{
			AssetManager::Get().GetGraphicsBuilder().FreeResource(m_asset->textureViewGPU);
			AssetManager::Get().GetGraphicsBuilder().FreeResource(m_asset->textureGPU);
			stateFlag &= ~AssetStateFlag::ExistOnGPU;
		}
	}
}