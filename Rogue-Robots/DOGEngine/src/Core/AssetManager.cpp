#include "AssetManager.h"

#pragma warning(push, 0)
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#pragma warning(pop)

namespace DOG
{
	AssetManager::AssetManager()
	{

	}

	AssetManager::~AssetManager()
	{

	}

	u64 AssetManager::LoadModelAsset(const std::string& path, AssetLoadFlag)
	{
		assert(std::filesystem::exists(path));
		DOG::AssimpImporter assimpImporter = DOG::AssimpImporter(path);
		auto asset = assimpImporter.get_result();

		ModelAsset newModel;
		newModel.filePath = path;
		newModel.meshID = AddMesh(asset->mesh, path);
		newModel.submeshes = std::move(asset->submeshes);
		newModel.materialIDs = LoadMaterials(asset->materials);

		GetAsset(newModel.meshID)->stateFlag = AssetStateFlag::ExistOnCPU;
		newModel.stateFlag = AssetStateFlag::ExistOnCPU;

		u64 id = GenerateRandomID();
		m_assets[id] = std::make_unique<ModelAsset>(std::move(newModel));
		return id;
	}

	u64 AssetManager::LoadTexture(const std::string& path, AssetLoadFlag)
	{
		assert(std::filesystem::exists(path));
		int width;
		int height;
		int numChannels; // Number of channels the image contained, we will force it to load with rgba
		u8* imageData = stbi_load(path.c_str(), &width, &height, &numChannels, STBI_rgb_alpha);
		numChannels = STBI_rgb_alpha; // we will have rgba
		assert(imageData);

		TextureAsset newTexture;
		newTexture.filePath = path;
		newTexture.mipLevels = 1; // Mip maps will be handled later on when the assetTool is implemented.
		newTexture.width = width;
		newTexture.height = height;
		newTexture.textureData.resize(width * height * numChannels);

		memcpy(newTexture.textureData.data(), imageData, newTexture.textureData.size());
		STBI_FREE(imageData);
		
		newTexture.stateFlag = AssetStateFlag::ExistOnCPU;
		u64 id = GenerateRandomID();
		m_assets[id] = std::make_unique<TextureAsset>(std::move(newTexture));
		return id;
	}

	u64 AssetManager::LoadAudio(const std::string&, AssetLoadFlag)
	{
		return 0;
	}

	Asset* AssetManager::GetAsset(u64 id) const
	{
		if (m_assets.contains(id))
		{
			return m_assets.at(id).get();
		}
		else
		{
			return nullptr;
		}
	}

	u64 AssetManager::AddMesh(const MeshAsset& mesh)
	{
		u64 id = GenerateRandomID();
		m_assets[id] = std::make_unique<MeshAsset>(mesh);
		return id;
	}

	u64 AssetManager::AddMesh(MeshAsset&& mesh)
	{
		u64 id = GenerateRandomID();
		m_assets[id] = std::make_unique<MeshAsset>(std::move(mesh));
		return id;
	}

	u64 AssetManager::AddMesh(const ImportedMesh& mesh, const std::string& pathImportedFrom)
	{
		MeshAsset newMesh;
		newMesh.filePath = pathImportedFrom;
		newMesh.indices = mesh.indices;
		newMesh.vertexData = mesh.vertexData;
		return AddMesh(std::move(newMesh));
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
}
