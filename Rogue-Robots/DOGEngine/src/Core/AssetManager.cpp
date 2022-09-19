#include "AssetManager.h"

#pragma warning(push, 0)

// Nad: Compressonator library already defines this 
//#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#pragma warning(pop)

namespace DOG
{
	std::unique_ptr<AssetManager> AssetManager::s_instance = nullptr;

	void AssetManager::Initialize()
	{
		assert(!s_instance);
		s_instance = std::unique_ptr<AssetManager>(new AssetManager());
	}

	void AssetManager::Destroy()
	{
		assert(s_instance);
		s_instance.reset();
		s_instance = nullptr;
	}

	AssetManager& AssetManager::Get()
	{
		assert(s_instance);
		return *s_instance;
	}

	AssetManager::AssetManager()
	{

	}

	AssetManager::~AssetManager()
	{
		for (auto& [id, asset] : m_assets)
		{
			assert(asset);
			delete asset;
			asset = nullptr;
		}
	}

	u64 AssetManager::LoadModelAsset(const std::string& path, AssetLoadFlag)
	{
		if (!std::filesystem::exists(path))
		{
			// assert wont catch if we have wrong path only in release mode
			std::cout << "AssetManager::LoadModelAsset throw. " + path + " does not exist" << std::endl;
			throw std::runtime_error(path + " does not exist");
		}

		DOG::AssimpImporter assimpImporter = DOG::AssimpImporter(path);
		auto asset = assimpImporter.GetResult();

		ModelAsset* newModel = new ModelAsset;
		newModel->meshID = AddMesh(asset->mesh);
		newModel->submeshes = std::move(asset->submeshes);
		newModel->materialIDs = LoadMaterials(asset->materials);

		u64 id = GenerateRandomID();
		m_assets.insert({ id, new ManagedAsset(AssetStateFlag::ExistOnCPU, newModel) });
		return id;
	}

	u64 AssetManager::LoadTexture(const std::string& path, AssetLoadFlag)
	{
		if (!std::filesystem::exists(path))
		{
			// assert wont catch if we have wrong path only in release mode
			std::cout << "AssetManager::LoadTexture throw. " + path + " does not exist" << std::endl;
			throw std::runtime_error(path + " does not exist");
		}

		int width;
		int height;
		int numChannels; // Number of channels the image contained, we will force it to load with rgba
		u8* imageData = stbi_load(path.c_str(), &width, &height, &numChannels, STBI_rgb_alpha);
		numChannels = STBI_rgb_alpha; // we will have rgba
		assert(imageData);

		TextureAsset* newTexture = new TextureAsset;
		newTexture->mipLevels = 1; // Mip maps will be handled later on when the assetTool is implemented.
		newTexture->width = width;
		newTexture->height = height;
		newTexture->textureData.resize(static_cast<size_t>(width) * height * numChannels);

		memcpy(newTexture->textureData.data(), imageData, newTexture->textureData.size());
		stbi_image_free(imageData);
		//STBI_FREE(imageData);
		
		u64 id = GenerateRandomID();
		m_assets.insert({ id, new ManagedAsset(AssetStateFlag::ExistOnCPU, newTexture) });
		return id;
	}

	u64 AssetManager::LoadAudio(const std::string& path, AssetLoadFlag)
	{
		if (!std::filesystem::exists(path))
		{
			throw FileNotFoundError(path);
		}
		u64 fileSize = std::filesystem::file_size(path);
		AudioAsset* newAudio = new AudioAsset;
		newAudio->filePath = path;

		if (fileSize > MAX_AUDIO_SIZE_ASYNC)
		{
			newAudio->async = true;
			newAudio->properties = ReadWAVProperties(path);
		}
		else
		{
			auto [properties, data] = ReadWAV(path);
			newAudio->properties = properties;
			newAudio->audioData = data;
		}

		u64 id = GenerateRandomID();
		m_assets.insert({ id, new ManagedAsset(AssetStateFlag::ExistOnCPU, newAudio) });
		return id;
	}

	Asset* AssetManager::GetAsset(u64 id) const
	{
		if (m_assets.contains(id))
		{
			return m_assets.at(id)->Get();
		}
		else
		{
			std::cout << "Warning AssetManager::GetAsset called with invalid id as argument." << std::endl;
			return nullptr;
		}
	}

	u64 AssetManager::AddMesh(const ImportedMesh& mesh)
	{
		MeshAsset* newMesh = new MeshAsset;
		newMesh->indices = mesh.indices;
		newMesh->vertexData = mesh.vertexData;

		u64 id = GenerateRandomID();
		m_assets.insert({ id, new ManagedAsset(AssetStateFlag::ExistOnCPU, newMesh) });
		return id;
	}

	void AssetManager::UnLoadAsset(u64 id, AssetUnLoadFlag flag)
	{
		if (m_assets.contains(id) && !m_assets[id]->CheckIfLoadingAsync())
		{
			assert(m_assets[id]->stateFlag != AssetStateFlag::Unknown);

			if (m_assets[id]->stateFlag & AssetStateFlag::ExistOnGPU && flag & AssetUnLoadFlag::RemoveFromVram)
			{
				// TODO
				// This can't be handled internaly in the ManagedAsset, it does not know about gpu land.
			}

			if (m_assets[id]->stateFlag & AssetStateFlag::ExistOnCPU && flag & AssetUnLoadFlag::RemoveFromRam)
			{
				m_assets[id]->ReleaseAsset();
			}
		}
	}

	std::vector<u64> AssetManager::LoadMaterials(const std::vector<ImportedMaterial>& importedMats)
	{
		std::vector<u64> newMats;
		newMats.reserve(importedMats.size());
		for (auto& m : importedMats)
		{
			Material material;

			material.albedo = m.albedoPath.empty() ? 0 : LoadTexture(m.albedoPath);
			material.metallicRoughness = m.metallicRoughnessPath.empty() ? 0 : LoadTexture(m.metallicRoughnessPath);
			material.normalMap = m.normalMapPath.empty() ? 0 : LoadTexture(m.normalMapPath);
			material.emissive = m.emissivePath.empty() ? 0 : LoadTexture(m.emissivePath);

			material.albedoFactor = DirectX::XMFLOAT4(m.albedoFactor);
			material.emissiveFactor = DirectX::XMFLOAT3(m.emissiveFactor);
			material.metallicFactor = m.metallicFactor;
			material.roughnessFactor = m.roughnessFactor;

			newMats.push_back(m_materialManager.AddMaterial(material));
		}

		return newMats;
	}

	ManagedAsset::ManagedAsset(AssetStateFlag flag, Asset* asset) : m_asset(asset), stateFlag(flag)
	{
		if (stateFlag & AssetStateFlag::LoadingAsync)
			m_isLoadingConcurrent = 1;
	}

	ManagedAsset::~ManagedAsset()
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

	Asset* ManagedAsset::Get() const
	{
		if (CheckIfLoadingAsync())
			return nullptr;
		else
			return m_asset;
	}
	bool ManagedAsset::CheckIfLoadingAsync() const
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
	void ManagedAsset::ReleaseAsset()
	{
		assert(m_asset && !CheckIfLoadingAsync());
		delete m_asset;
		m_asset = nullptr;
		stateFlag &= ~AssetStateFlag::ExistOnCPU;
	}
}
