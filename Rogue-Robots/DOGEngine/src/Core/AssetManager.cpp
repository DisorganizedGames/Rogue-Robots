#include "AssetManager.h"
#include "../Graphics/Rendering/Renderer.h"
#include "../Graphics/Rendering/TextureManager.h"
#include "../Graphics/Rendering/GraphicsBuilder.h"
#include "../Core/TextureFileImporter.h"

#pragma warning(push, 0)

// Nad: Compressonator library already defines this 
//#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#pragma warning(pop)


namespace DOG
{
	// Helpers
	std::optional<gfx::TextureView> TextureAssetToGfxTexture(u64 assetID, gfx::GraphicsBuilder& builder, const AssetManager& am);

	std::unique_ptr<AssetManager> AssetManager::s_instance = nullptr;
	std::mutex AssetManager::s_commandQueueMutex;
	std::queue<std::function<void()>> AssetManager::s_commandQueue;

	void AssetManager::Initialize(gfx::Renderer* renderer)
	{
		assert(!s_instance);
		s_instance = std::unique_ptr<AssetManager>(new AssetManager());
		s_instance->m_renderer = renderer;
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

	MaterialManager& AssetManager::GetMaterialManager()
	{
		return m_materialManager;
	}

	u64 AssetManager::LoadModelAsset(const std::string& path, AssetLoadFlag flag)
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
		newModel->materialIndices = LoadMaterials(asset->materials);

		u64 id = GenerateRandomID();
		m_assets.insert({ id, new ManagedAsset<ModelAsset>(AssetStateFlag::ExistOnCPU, newModel) });


		AssetManager::AddCommand([](u64 idToMove, AssetLoadFlag f) { AssetManager::Get().MoveModelToGPU(idToMove, f); }, id, flag);
		return id;
	}

	u64 AssetManager::LoadTexture(const std::string& path, AssetLoadFlag flag)
	{
		if (!std::filesystem::exists(path))
		{
			// assert wont catch if we have wrong path only in release mode
			std::cout << "AssetManager::LoadTexture throw. " + path + " does not exist" << std::endl;
			throw std::runtime_error(path + " does not exist");
		}
		//u64 id = LoadTextureSTBI(path, flag);
		u64 id = LoadTextureCommpresonator(path, flag);
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
		m_assets.insert({ id, new ManagedAsset<AudioAsset>(AssetStateFlag::ExistOnCPU, newAudio) });
		return id;
	}

	Asset* AssetManager::GetBaseAsset(u64 id) const
	{
		if (m_assets.contains(id))
		{
			return m_assets.at(id)->GetBase();
		}
		else
		{
			std::cout << "Warning AssetManager::GetAsset called with invalid id as argument." << std::endl;
			return nullptr;
		}
	}

	void AssetManager::Update()
	{
		ExecuteCommands();
	}

	void AssetManager::ExecuteCommands()
	{
		while (!s_commandQueue.empty())
		{
			s_commandQueueMutex.lock();
			auto command = std::move(s_commandQueue.front());
			s_commandQueue.pop();
			s_commandQueueMutex.unlock();
			command();
		}
	}

	u64 AssetManager::AddMesh(const ImportedMesh& mesh)
	{
		MeshAsset* newMesh = new MeshAsset;
		newMesh->indices = mesh.indices;
		newMesh->vertexData = mesh.vertexData;

		u64 id = GenerateRandomID();
		m_assets.insert({ id, new ManagedAsset<MeshAsset>(AssetStateFlag::ExistOnCPU, newMesh) });
		return id;
	}

	void AssetManager::UnLoadAsset(u64 id, AssetUnLoadFlag flag)
	{
		if (m_assets.contains(id) && !m_assets[id]->CheckIfLoadingAsync())
		{
			assert(m_assets[id]->stateFlag != AssetStateFlag::Unknown);

			m_assets[id]->UnloadAsset(flag);
		}
	}

	std::vector<u64> AssetManager::LoadMaterials(const std::vector<ImportedMaterial>& importedMats)
	{
		std::vector<u64> newMats;
		newMats.reserve(importedMats.size());
		for (auto& m : importedMats)
		{
			Material material;

			material.albedo = m.albedoPath.empty() ? 0 : LoadTexture(m.albedoPath, AssetLoadFlag::Srgb);
			material.metallicRoughness = m.metallicRoughnessPath.empty() ? 0 : LoadTexture(m.metallicRoughnessPath, AssetLoadFlag::Srgb);
			material.normalMap = m.normalMapPath.empty() ? 0 : LoadTexture(m.normalMapPath);
			material.emissive = m.emissivePath.empty() ? 0 : LoadTexture(m.emissivePath, AssetLoadFlag::Srgb);

			material.albedoFactor = DirectX::XMFLOAT4(m.albedoFactor);
			material.emissiveFactor = DirectX::XMFLOAT3(m.emissiveFactor);
			material.metallicFactor = m.metallicFactor;
			material.roughnessFactor = m.roughnessFactor;

			newMats.push_back(m_materialManager.AddMaterial(material));
		}

		return newMats;
	}

	u64 AssetManager::LoadTextureSTBI(const std::string& path, AssetLoadFlag flag)
	{
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
		newTexture->srgb = flag & AssetLoadFlag::Srgb;
		u64 id = GenerateRandomID();
		m_assets.insert({ id, new ManagedAsset<TextureAsset>(AssetStateFlag::ExistOnCPU, newTexture) });
		return id;
	}

	u64 AssetManager::LoadTextureCommpresonator(const std::string& path, AssetLoadFlag flag)
	{
		auto importedTex = TextureFileImporter(path, false).GetResult();
		TextureAsset* newTexture = new TextureAsset;
		newTexture->mipLevels = 1; // Mip maps will be handled later on when the assetTool is implemented.
		newTexture->width = importedTex->dataPerMip.front().width;
		newTexture->height = importedTex->dataPerMip.front().height;
		assert(static_cast<size_t>(newTexture->width) * newTexture->height * 4 == importedTex->dataPerMip.front().data.size());
		newTexture->textureData.resize(importedTex->dataPerMip.front().data.size());
		memcpy(newTexture->textureData.data(), importedTex->dataPerMip.front().data.data(), importedTex->dataPerMip.front().data.size());
		newTexture->srgb = flag & AssetLoadFlag::Srgb;
		u64 id = GenerateRandomID();
		m_assets.insert({ id, new ManagedAsset<TextureAsset>(AssetStateFlag::ExistOnCPU, newTexture) });
		return id;
	}

	void AssetManager::MoveModelToGPU(u64 modelID, AssetLoadFlag flag)
	{
		gfx::GraphicsBuilder* builder = m_renderer->GetBuilder();

		// Convert ModelAsset and its MeshAsset to MeshSpecification
		ModelAsset* modelAsset = GetAsset<ModelAsset>(modelID);
		MeshAsset* meshAsset = GetAsset<MeshAsset>(modelAsset->meshID);
		gfx::MeshTable::MeshSpecification loadSpec{};

		loadSpec.submeshData = modelAsset->submeshes;
		loadSpec.indices = meshAsset->indices;
		for (auto& [attr, data] : meshAsset->vertexData)
			loadSpec.vertexDataPerAttribute[attr] = data;


		// Convert Materials and its TextureAssets to MaterialSpecification
		std::vector<gfx::MaterialTable::MaterialSpecification> matSpecs;
		matSpecs.reserve(modelAsset->materialIndices.size());
		for (auto& matIndex : modelAsset->materialIndices)
		{
			const Material& mat = m_materialManager.GetMaterial(matIndex);
			auto& matSpec = matSpecs.emplace_back();

			matSpec.albedo = TextureAssetToGfxTexture(mat.albedo, *builder, *this);
			matSpec.metallicRoughness = TextureAssetToGfxTexture(mat.metallicRoughness, *builder, *this);
			matSpec.emissive = TextureAssetToGfxTexture(mat.emissive, *builder, *this);
			matSpec.normal = TextureAssetToGfxTexture(mat.normalMap, *builder, *this);

			matSpec.albedoFactor = mat.albedoFactor;
			matSpec.emissiveFactor = DirectX::SimpleMath::Vector4(mat.emissiveFactor.x, mat.emissiveFactor.y, mat.emissiveFactor.z, 1.f);
			matSpec.roughnessFactor = mat.roughnessFactor;
			matSpec.metallicFactor = mat.metallicFactor;
		}

		modelAsset->gfxModel = builder->LoadCustomModel(loadSpec, matSpecs);

		if (flag & AssetLoadFlag::VramOnly)
		{
			AssetUnLoadFlag unloadFlag = AssetUnLoadFlag::MeshCPU;
			unloadFlag |= AssetUnLoadFlag::TextureCPU;
			AssetManager::Get().UnLoadAsset(modelID, unloadFlag);
		}
	}

	std::optional<gfx::TextureView> TextureAssetToGfxTexture(u64 assetID, gfx::GraphicsBuilder& builder, const AssetManager& am)
	{
		if (!assetID) return std::nullopt;
		TextureAsset* asset = am.GetAsset<TextureAsset>(assetID);
		if (!asset) return std::nullopt;

		gfx::GraphicsBuilder::TextureSubresource subres{};
		subres.width = asset->width;
		subres.height = asset->height;
		subres.data = asset->textureData;

		// Only one mip level for now
		gfx::GraphicsBuilder::MippedTexture2DSpecification textureSpec{};
		textureSpec.srgb = asset->srgb;
		textureSpec.dataPerMip.push_back(subres);
		gfx::Texture texture = builder.LoadTexture(textureSpec);

		gfx::TextureViewDesc desc = gfx::TextureViewDesc(gfx::ViewType::ShaderResource, gfx::TextureViewDimension::Texture2D,
			textureSpec.srgb ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM);

		return builder.CreateTextureView(texture, desc);
	}
}