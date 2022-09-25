#include "AssetManager.h"
#include "../Graphics/Rendering/Renderer.h"
#include "../Core/TextureFileImporter.h"

#pragma warning(push, 0)

// Nad: Compressonator library already defines this 
//#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#pragma warning(pop)

using namespace DOG::gfx;

namespace DOG
{
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

	void AssetManager::LoadModelAssetInternal(const std::string& path, u32 id, AssetLoadFlag flag, ModelAsset* assetOut)
	{
		if (flag & AssetLoadFlag::Async)
		{
			m_assets[id]->stateFlag |= AssetStateFlag::ExistOnCPU;
			m_assets[id]->stateFlag |= AssetStateFlag::LoadingAsync;
			m_assets[id]->m_isLoadingConcurrent = 1;

			CallAsync([=, lock = &m_assets[id]->m_isLoadingConcurrent]()
				{
					DOG::AssimpImporter assimpImporter = DOG::AssimpImporter(path);
					std::shared_ptr<ImportedModel> asset = assimpImporter.GetResult();

					assetOut->meshAsset.indices = std::move(asset->mesh.indices);
					assetOut->meshAsset.vertexData = std::move(asset->mesh.vertexData);
					assetOut->submeshes = std::move(asset->submeshes);

					*lock = 0;
					AssetManager::AddCommand([model = assetOut, importedModel = asset, f = flag]()
						{
							model->materialIndices = AssetManager::Get().LoadMaterials(importedModel->materials, f);
						});

					if (flag & AssetLoadFlag::GPUMemory || flag & AssetLoadFlag::GPUMemoryOnly)
					{
						AssetManager::AddCommand([idToMove = id, f = flag]() { AssetManager::Get().MoveModelToGPU(idToMove, f); });
					}
				});
		}
		else
		{
			DOG::AssimpImporter assimpImporter = DOG::AssimpImporter(path);
			auto asset = assimpImporter.GetResult();

			assetOut->meshAsset.indices = std::move(asset->mesh.indices);
			assetOut->meshAsset.vertexData = std::move(asset->mesh.vertexData);
			assetOut->submeshes = std::move(asset->submeshes);
			assetOut->materialIndices = LoadMaterials(asset->materials, flag);

			m_assets[id]->stateFlag |= AssetStateFlag::ExistOnCPU;

			AssetManager::AddCommand([idToMove = id, f = flag]() { AssetManager::Get().MoveModelToGPU(idToMove, f); });
		}
	}

	void AssetManager::LoadTextureAssetInternal(const std::string& path, u32 id, AssetLoadFlag flag, TextureAsset* assetOut)
	{
		//LoadTextureSTBI(path, flag, assetOut);

		if (flag & AssetLoadFlag::Async)
		{
			m_assets[id]->stateFlag |= AssetStateFlag::ExistOnCPU;
			m_assets[id]->stateFlag |= AssetStateFlag::LoadingAsync;
			m_assets[id]->m_isLoadingConcurrent = 1;
			CallAsync([=, lock = &m_assets[id]->m_isLoadingConcurrent]()
				{
					AssetManager::LoadTextureCommpresonator(path, flag, assetOut);
					*lock = 0;
					if (flag & AssetLoadFlag::GPUMemory || flag & AssetLoadFlag::GPUMemoryOnly)
					{
						AssetManager::AddCommand([idToMove = id, f = flag]() { AssetManager::Get().MoveTextureToGPU(idToMove, f); });
					}
				});
		}
		else
		{
			LoadTextureCommpresonator(path, flag, assetOut);
			m_assets[id]->stateFlag |= AssetStateFlag::ExistOnCPU;

			if (flag & AssetLoadFlag::GPUMemory || flag & AssetLoadFlag::GPUMemoryOnly)
			{
				AssetManager::AddCommand([idToMove = id, f = flag]() { AssetManager::Get().MoveTextureToGPU(idToMove, f); });
			}
		}
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

	u32 AssetManager::LoadModelAsset(const std::string& path, AssetLoadFlag flag)
	{
		u32 id = 0;
		if (AssetNeedsToBeLoaded(path, id))
		{
			if (id == 0)
			{
				id = NextKey();
				m_assets.insert({ id, new ManagedAsset<ModelAsset>(AssetStateFlag::Unknown, new ModelAsset) });
				m_pathTOAssetID[path] = id;
			}
			ModelAsset* p = GetAsset<ModelAsset>(id);
			LoadModelAssetInternal(path, id, flag, p);
		}
		assert(id != 0);
		return id;
	}

	u32 AssetManager::LoadTexture(const std::string& path, AssetLoadFlag flag)
	{
		u32 id = 0;
		if (AssetNeedsToBeLoaded(path, id))
		{
			if (id == 0)
			{
				id = NextKey();
				m_assets.insert({ id, new ManagedAsset<TextureAsset>(AssetStateFlag::Unknown, new TextureAsset) });
				m_pathTOAssetID[path] = id;
			}
			TextureAsset* p = GetAsset<TextureAsset>(id);
			LoadTextureAssetInternal(path, id, flag, p);
		}
		assert(id != 0);
		return id;
	}

	u32 AssetManager::LoadAudio(const std::string& path, AssetLoadFlag)
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

		u32 id = NextKey();
		m_assets.insert({ id, new ManagedAsset<AudioAsset>(AssetStateFlag::ExistOnCPU, newAudio) });
		return id;
	}

	Asset* AssetManager::GetBaseAsset(u32 id) const
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
		s_commandQueueMutex.lock();
		int n = s_commandQueue.size();
		s_commandQueueMutex.unlock();
		while (n > 0)
		{
			s_commandQueueMutex.lock();
			auto command = std::move(s_commandQueue.front());
			s_commandQueue.pop();
			s_commandQueueMutex.unlock();
			command();
			n--;
		}
	}

	u32 AssetManager::AddMesh(const ImportedMesh& mesh)
	{
		MeshAsset* newMesh = new MeshAsset;
		newMesh->indices = mesh.indices;
		newMesh->vertexData = mesh.vertexData;

		u32 id = NextKey();
		m_assets.insert({ id, new ManagedAsset<MeshAsset>(AssetStateFlag::ExistOnCPU, newMesh) });
		return id;
	}

	void AssetManager::UnLoadAsset(u32 id, AssetUnLoadFlag flag)
	{
		if (m_assets.contains(id) && !m_assets[id]->CheckIfLoadingAsync())
		{
			assert(m_assets[id]->stateFlag != AssetStateFlag::Unknown);

			m_assets[id]->UnloadAsset(flag);
		}
	}

	std::vector<u32> AssetManager::LoadMaterials(const std::vector<ImportedMaterial>& importedMats, AssetLoadFlag flag)
	{
		std::vector<u32> newMats;
		newMats.reserve(importedMats.size());
		AssetLoadFlag flagWithSrgb = AssetLoadFlag::Srgb;
		flagWithSrgb |= flag;
		for (auto& m : importedMats)
		{
			Material material;

			material.albedo = m.albedoPath.empty() ? 0 : LoadTexture(m.albedoPath, flagWithSrgb);
			material.metallicRoughness = m.metallicRoughnessPath.empty() ? 0 : LoadTexture(m.metallicRoughnessPath, flagWithSrgb);
			material.normalMap = m.normalMapPath.empty() ? 0 : LoadTexture(m.normalMapPath, flag);
			material.emissive = m.emissivePath.empty() ? 0 : LoadTexture(m.emissivePath, flagWithSrgb);

			material.albedoFactor = DirectX::XMFLOAT4(m.albedoFactor);
			material.emissiveFactor = DirectX::XMFLOAT3(m.emissiveFactor);
			material.metallicFactor = m.metallicFactor;
			material.roughnessFactor = m.roughnessFactor;

			newMats.push_back(m_materialManager.AddMaterial(material));
		}

		return newMats;
	}

	void AssetManager::LoadTextureSTBI(const std::string& path, AssetLoadFlag flag, TextureAsset* assetOut)
	{
		int width;
		int height;
		int numChannels; // Number of channels the image contained, we will force it to load with rgba
		u8* imageData = stbi_load(path.c_str(), &width, &height, &numChannels, STBI_rgb_alpha);
		numChannels = STBI_rgb_alpha; // we will have rgba
		assert(imageData);

		assetOut->mipLevels = 1; // Mip maps will be handled later on when the assetTool is implemented.
		assetOut->width = width;
		assetOut->height = height;
		assetOut->textureData.resize(static_cast<size_t>(width) * height * numChannels);

		memcpy(assetOut->textureData.data(), imageData, assetOut->textureData.size());
		stbi_image_free(imageData);
		//STBI_FREE(imageData);
		assetOut->srgb = flag & AssetLoadFlag::Srgb;
	}

	void AssetManager::LoadTextureCommpresonator(const std::string& path, AssetLoadFlag flag, TextureAsset* assetOut)
	{
		auto importedTex = TextureFileImporter(path, false).GetResult();
		assetOut->mipLevels = 1; // Mip maps will be handled later on when the assetTool is implemented.
		assetOut->width = importedTex->dataPerMip.front().width;
		assetOut->height = importedTex->dataPerMip.front().height;
		assert(static_cast<size_t>(assetOut->width) * assetOut->height * 4 == importedTex->dataPerMip.front().data.size());
		assetOut->textureData.resize(importedTex->dataPerMip.front().data.size());
		memcpy(assetOut->textureData.data(), importedTex->dataPerMip.front().data.data(), importedTex->dataPerMip.front().data.size());
		assetOut->srgb = flag & AssetLoadFlag::Srgb;
	}

	void AssetManager::MoveModelToGPU(u32 modelID, AssetLoadFlag flag)
	{
		gfx::GraphicsBuilder* builder = m_renderer->GetBuilder();

		// Convert ModelAsset and its MeshAsset to MeshSpecification
		ModelAsset* modelAsset = GetAsset<ModelAsset>(modelID);
		assert(modelAsset);

		auto&& needToWait = [&](u32 texID) {
			if (texID == 0) return false;
			if (m_assets[texID]->CheckIfLoadingAsync()) return true;
			if (!(m_assets[texID]->stateFlag & AssetStateFlag::ExistOnGPU)) return true;
			
			return false;
		};
		for (auto& matIndex : modelAsset->materialIndices)
		{
			bool yield = false;
			const Material& mat = m_materialManager.GetMaterial(matIndex);

			yield = yield || needToWait(mat.albedo);
			yield = yield || needToWait(mat.emissive);
			yield = yield || needToWait(mat.metallicRoughness);
			yield = yield || needToWait(mat.normalMap);
			if (yield)
			{
				CallAsync([=]() {
					std::this_thread::sleep_for(std::chrono::milliseconds(20));
					AssetManager::AddCommand([](u32 idToMove, AssetLoadFlag f) { AssetManager::Get().MoveModelToGPU(idToMove, f); }, modelID, flag);
					});
				return;
			}
		}
		gfx::MeshTable::MeshSpecification loadSpec{};

		loadSpec.submeshData = modelAsset->submeshes;
		loadSpec.indices = modelAsset->meshAsset.indices;
		for (auto& [attr, data] : modelAsset->meshAsset.vertexData)
			loadSpec.vertexDataPerAttribute[attr] = data;


		// Convert Materials and its TextureAssets to MaterialSpecification
		std::vector<gfx::MaterialTable::MaterialSpecification> matSpecs;
		matSpecs.reserve(modelAsset->materialIndices.size());
		for (auto& matIndex : modelAsset->materialIndices)
		{
			const Material& mat = m_materialManager.GetMaterial(matIndex);
			auto& matSpec = matSpecs.emplace_back();

			assert(!mat.albedo || GetAsset<TextureAsset>(mat.albedo));
			assert(!mat.metallicRoughness || GetAsset<TextureAsset>(mat.metallicRoughness));
			assert(!mat.emissive || GetAsset<TextureAsset>(mat.emissive));
			assert(!mat.normalMap || GetAsset<TextureAsset>(mat.normalMap));

			if (mat.albedo && m_assets[mat.albedo]->stateFlag & AssetStateFlag::ExistOnGPU) matSpec.albedo = GetAsset<TextureAsset>(mat.albedo)->textureViewGPU;
			if (mat.metallicRoughness && m_assets[mat.metallicRoughness]->stateFlag & AssetStateFlag::ExistOnGPU) matSpec.metallicRoughness = GetAsset<TextureAsset>(mat.metallicRoughness)->textureViewGPU;
			if (mat.emissive && m_assets[mat.emissive]->stateFlag & AssetStateFlag::ExistOnGPU) matSpec.emissive = GetAsset<TextureAsset>(mat.emissive)->textureViewGPU;
			if (mat.normalMap && m_assets[mat.normalMap]->stateFlag & AssetStateFlag::ExistOnGPU) matSpec.normal = GetAsset<TextureAsset>(mat.normalMap)->textureViewGPU;

			matSpec.albedoFactor = mat.albedoFactor;
			matSpec.emissiveFactor = DirectX::SimpleMath::Vector4(mat.emissiveFactor.x, mat.emissiveFactor.y, mat.emissiveFactor.z, 1.f);
			matSpec.roughnessFactor = mat.roughnessFactor;
			matSpec.metallicFactor = mat.metallicFactor;
		}

		modelAsset->gfxModel = builder->LoadCustomModel(loadSpec, matSpecs);

		if (flag & AssetLoadFlag::GPUMemoryOnly)
		{
			AssetUnLoadFlag unloadFlag = AssetUnLoadFlag::MeshCPU;
			unloadFlag |= AssetUnLoadFlag::TextureCPU;
			AssetManager::Get().UnLoadAsset(modelID, unloadFlag);
		}
	}

	void AssetManager::MoveTextureToGPU(u32 textureID, AssetLoadFlag flag)
	{
		assert(m_renderer);
		gfx::GraphicsBuilder* builder = m_renderer->GetBuilder();

		gfx::GraphicsBuilder::MippedTexture2DSpecification textureSpec{};

		TextureAsset* asset = GetAsset<TextureAsset>(textureID);
		assert(asset); // This could happen and if it does i want to know about it.

		gfx::GraphicsBuilder::TextureSubresource subres{};
		subres.width = asset->width;
		subres.height = asset->height;
		subres.data = asset->textureData;

		textureSpec.srgb = asset->srgb;
		textureSpec.dataPerMip.push_back(subres);

		asset->textureGPU = builder->LoadTexture(textureSpec);

		TextureViewDesc desc = TextureViewDesc(ViewType::ShaderResource, TextureViewDimension::Texture2D,
			textureSpec.srgb ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM);
		

		asset->textureViewGPU = builder->CreateTextureView(asset->textureGPU, desc);

		m_assets[textureID]->stateFlag |= AssetStateFlag::ExistOnGPU;

		if (flag & AssetLoadFlag::GPUMemoryOnly)
		{
			AssetManager::Get().UnLoadAsset(textureID, AssetUnLoadFlag::TextureCPU);
		}
	}

	u32 AssetManager::NextKey()
	{
		return ++m_lastKey;
	}

	bool AssetManager::AssetNeedsToBeLoaded(const std::string& path, u32& assetIDOut)
	{
		if (!std::filesystem::exists(path))
		{
			// assert wont catch if we have wrong path only in release mode
			std::cout << "AssetManager::AssetShouldBeLoaded throw. " + path + " does not exist" << std::endl;
			throw std::runtime_error(path + " does not exist");
		}

		u32 id = 0;
		bool assetNeedsToBeLoaded = false;
		if (m_pathTOAssetID.contains(path))
		{
			id = m_pathTOAssetID.at(path);
			assert(m_assets.contains(id));
			auto& managedAsset = m_assets[id];

			if (!managedAsset->CheckIfLoadingAsync() && managedAsset->stateFlag & AssetStateFlag::Evicted)
			{
				assetNeedsToBeLoaded = true;
			}
		}
		else
		{
			assetNeedsToBeLoaded = true;
		}

		assetIDOut = id;
		return assetNeedsToBeLoaded;
	}

	gfx::GraphicsBuilder& AssetManager::GetGraphicsBuilder()
	{
		assert(m_renderer);
		assert(m_renderer->GetBuilder());
		return *m_renderer->GetBuilder();
	}
}