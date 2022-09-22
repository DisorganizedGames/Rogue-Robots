#include "ManagedAssets.h"
#include "AssetManager.h"

namespace DOG
{
	ManagedAssetBase::ManagedAssetBase(AssetStateFlag flag) : stateFlag(flag)
	{
		if (stateFlag & AssetStateFlag::LoadingAsync)
			m_isLoadingConcurrent = 1;
	}

	bool ManagedAssetBase::CheckIfLoadingAsync()
	{
		if (stateFlag & AssetStateFlag::LoadingAsync)
		{
			if (m_isLoadingConcurrent == 1)
				return true;
			else
				stateFlag &= ~AssetStateFlag::LoadingAsync;
		}
		return false;
	}





	void ManagedAsset<ModelAsset>::UnloadAsset(AssetUnLoadFlag flag)
	{
		assert(m_asset && !CheckIfLoadingAsync());

		if (flag & AssetUnLoadFlag::MeshCPU)
		{
			m_asset->submeshes.clear();
			std::vector<SubmeshMetadata>().swap(m_asset->submeshes); // This will release the memory

			AssetManager::Get().UnLoadAsset(m_asset->meshID, flag);
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
}