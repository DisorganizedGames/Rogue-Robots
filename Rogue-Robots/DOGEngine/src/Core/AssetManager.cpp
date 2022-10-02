#include "AssetManager.h"
#include "../Graphics/Rendering/Renderer.h"
#include "../Core/TextureFileImporter.h"
#include "ImGuiMenuLayer.h"
#include "ImGUI/imgui.h"

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
		ImGuiMenuLayer::RegisterDebugWindow("LoadModel", [](bool& open) { AssetManager::Get().ImguiLoadModel(open); });
		ImGuiMenuLayer::RegisterDebugWindow("AssetManager", [](bool& open) { AssetManager::Get().AssetManagerGUI(open); });
	}

	void AssetManager::LoadModelAssetInternal(const std::string& path, u32 id, ModelAsset* assetOut)
	{
		assert(m_assets[id]->loadFlag & AssetLoadFlag::GPUMemory);
		assert(!(m_assets[id]->loadFlag & AssetLoadFlag::CPUMemory));

		if (m_assets[id]->loadFlag & AssetLoadFlag::Async)
		{
			ManagedAsset<ModelAsset>* managedModel = static_cast<ManagedAsset<ModelAsset>*>(m_assets[id]);
			m_assets[id]->m_isLoadingConcurrent = 1;

			CallAsync([managedModel = managedModel, id = id, model = assetOut, path = path, lock = &m_assets[id]->m_isLoadingConcurrent]()
				{
					DOG::AssimpImporter assimpImporter = DOG::AssimpImporter(path);
					std::shared_ptr<ImportedModel> asset = assimpImporter.GetResult();

					model->meshAsset.indices = std::move(asset->mesh.indices);
					model->meshAsset.vertexData = std::move(asset->mesh.vertexData);
					model->animation = std::move(asset->animation);
					model->submeshes = std::move(asset->submeshes);

					// Add command that runs on the main thread
					AssetManager::AddCommand([id = id, managedModel = managedModel, model = model, importedModel = asset](AssetLoadFlag textureLoadFlag)
						{
							managedModel->stateFlag |= AssetStateFlag::ExistOnCPU;

							if (managedModel->loadFlag & AssetLoadFlag::CPUMemory)
								managedModel->loadFlag &= ~AssetLoadFlag::CPUMemory;
							else
								managedModel->unLoadFlag |= AssetUnLoadFlag::AllCPU;

							model->materialIndices = AssetManager::Get().LoadMaterials(importedModel->materials, textureLoadFlag);
							if (managedModel->loadFlag & AssetLoadFlag::GPUMemory)
							{
								AssetManager::AddCommand([id = id]() { AssetManager::Get().MoveModelToGPU(id); });
							}
						}, managedModel->loadFlag); // send a copy of the flag, as it is not safe to assume it will not change.

					*lock = 0;
				});
		}
		else
		{
			DOG::AssimpImporter assimpImporter = DOG::AssimpImporter(path);
			auto asset = assimpImporter.GetResult();

			assetOut->meshAsset.indices = std::move(asset->mesh.indices);
			assetOut->meshAsset.vertexData = std::move(asset->mesh.vertexData);
			assetOut->submeshes = std::move(asset->submeshes);
			assetOut->animation = std::move(asset->animation);

			assetOut->materialIndices = LoadMaterials(asset->materials, m_assets[id]->loadFlag);

			m_assets[id]->stateFlag |= AssetStateFlag::ExistOnCPU;

			if (m_assets[id]->loadFlag & AssetLoadFlag::CPUMemory)
				m_assets[id]->loadFlag &= ~AssetLoadFlag::CPUMemory;
			else
				m_assets[id]->unLoadFlag |= AssetUnLoadFlag::AllCPU;

			AssetManager::AddCommand([idToMove = id]() { AssetManager::Get().MoveModelToGPU(idToMove); });
		}
	}

	void AssetManager::LoadShapeAssetInternal(const std::string& path, const Shape shape, const u32 tessFactor1, const u32 tessFactor2, u32 id, ModelAsset* assetOut)
	{
		assert(m_assets[id]->loadFlag & AssetLoadFlag::GPUMemory);
		assert(!(m_assets[id]->loadFlag & AssetLoadFlag::CPUMemory));

		if (m_assets[id]->loadFlag & AssetLoadFlag::Async)
		{
			ManagedAsset<ModelAsset>* managedModel = static_cast<ManagedAsset<ModelAsset>*>(m_assets[id]);
			m_assets[id]->m_isLoadingConcurrent = 1;

			CallAsync([managedModel = managedModel, id = id, model = assetOut, path = path, shape = shape, tessFactor1 = tessFactor1, tessFactor2 = tessFactor2, lock = &m_assets[id]->m_isLoadingConcurrent]()
				{
					DOG::ShapeCreator shapeCreator = DOG::ShapeCreator(shape, tessFactor1, tessFactor2);
					std::shared_ptr<ImportedModel> asset = shapeCreator.GetResult();

					model->meshAsset.indices = std::move(asset->mesh.indices);
					model->meshAsset.vertexData = std::move(asset->mesh.vertexData);
					model->animation = std::move(asset->animation);
					model->submeshes = std::move(asset->submeshes);

					// Add command that runs on the main thread
					AssetManager::AddCommand([id = id, managedModel = managedModel, model = model, importedModel = asset](AssetLoadFlag textureLoadFlag)
						{
							managedModel->stateFlag |= AssetStateFlag::ExistOnCPU;

							if (managedModel->loadFlag & AssetLoadFlag::CPUMemory)
								managedModel->loadFlag &= ~AssetLoadFlag::CPUMemory;
							else
								managedModel->unLoadFlag |= AssetUnLoadFlag::AllCPU;

							model->materialIndices = AssetManager::Get().LoadMaterials(importedModel->materials, textureLoadFlag);
							if (managedModel->loadFlag & AssetLoadFlag::GPUMemory)
							{
								AssetManager::AddCommand([id = id]() { AssetManager::Get().MoveModelToGPU(id); });
							}
						}, managedModel->loadFlag); // send a copy of the flag, as it is not safe to assume it will not change.

					*lock = 0;
				});
		}
		else
		{
			DOG::AssimpImporter assimpImporter = DOG::AssimpImporter(path);
			auto asset = assimpImporter.GetResult();

			assetOut->meshAsset.indices = std::move(asset->mesh.indices);
			assetOut->meshAsset.vertexData = std::move(asset->mesh.vertexData);
			assetOut->submeshes = std::move(asset->submeshes);
			assetOut->animation = std::move(asset->animation);

			assetOut->materialIndices = LoadMaterials(asset->materials, m_assets[id]->loadFlag);

			m_assets[id]->stateFlag |= AssetStateFlag::ExistOnCPU;

			if (m_assets[id]->loadFlag & AssetLoadFlag::CPUMemory)
				m_assets[id]->loadFlag &= ~AssetLoadFlag::CPUMemory;
			else
				m_assets[id]->unLoadFlag |= AssetUnLoadFlag::AllCPU;

			AssetManager::AddCommand([idToMove = id]() { AssetManager::Get().MoveModelToGPU(idToMove); });
		}
	}

	void AssetManager::LoadTextureAssetInternal(const std::string& path, u32 id, TextureAsset* assetOut)
	{
		//LoadTextureSTBI(path, flag, assetOut);

		if (m_assets[id]->loadFlag & AssetLoadFlag::Async)
		{
			m_assets[id]->m_isLoadingConcurrent = 1;
			CallAsync([=, lock = &m_assets[id]->m_isLoadingConcurrent]()
				{
					AssetManager::LoadTextureCommpresonator(path, assetOut);
					AssetManager::AddCommand([idToMove = id]()
						{
							AssetManager::Get().m_assets[idToMove]->stateFlag |= AssetStateFlag::ExistOnCPU;
							if (AssetManager::Get().m_assets[idToMove]->loadFlag & AssetLoadFlag::CPUMemory)
								AssetManager::Get().m_assets[idToMove]->loadFlag &= ~AssetLoadFlag::CPUMemory;
							else
								AssetManager::Get().m_assets[idToMove]->unLoadFlag |= AssetUnLoadFlag::TextureCPU;

							AssetManager::Get().MoveTextureToGPU(idToMove);
						});
					*lock = 0;
				});
		}
		else
		{
			LoadTextureCommpresonator(path, assetOut);
			m_assets[id]->stateFlag |= AssetStateFlag::ExistOnCPU;

			if (m_assets[id]->loadFlag & AssetLoadFlag::CPUMemory)
				m_assets[id]->loadFlag &= ~AssetLoadFlag::CPUMemory;
			else
				m_assets[id]->unLoadFlag |= AssetUnLoadFlag::TextureCPU;

			if (m_assets[id]->loadFlag & AssetLoadFlag::GPUMemory)
			{
				AssetManager::AddCommand([idToMove = id]() { AssetManager::Get().MoveTextureToGPU(idToMove); });
			}
		}
	}


	AssetManager::~AssetManager()
	{
		ImGuiMenuLayer::UnRegisterDebugWindow("LoadModel");
		ImGuiMenuLayer::UnRegisterDebugWindow("AssetManager");
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
		if (AssetNeedsToBeLoaded(path, flag, id))
		{
			if (id == 0)
			{
				id = NextKey();
				m_assets.insert({ id, new ManagedAsset<ModelAsset>(new ModelAsset) });
				m_pathTOAssetID[path] = id;
			}
			ModelAsset* p = GetAsset<ModelAsset>(id); // GetAsset will clear the async bit of loadFlag
			m_assets[id]->loadFlag |= flag; // importand that we add the flag after call to GetAsset
			
			// Stop a potential unload;
			if (flag & AssetLoadFlag::CPUMemory)
				m_assets[id]->unLoadFlag &= ~AssetUnLoadFlag::AllCPU;
			if (flag & AssetLoadFlag::GPUMemory)
				m_assets[id]->unLoadFlag &= ~AssetUnLoadFlag::AllGPU;

			LoadModelAssetInternal(path, id, p);
		}
		assert(id != 0);
		return id;
	}

	u32 AssetManager::LoadShapeAsset(const Shape shape, const u32 tessFactor1, const u32 tessFactor2, AssetLoadFlag flag)
	{
		u32 id = 0;
		const std::string path = std::to_string(static_cast<std::underlying_type<Shape>::type>(shape)) +" "+ std::to_string(tessFactor1) + " " + std::to_string(tessFactor2);
		if (ShapeAssetNeedsToBeLoaded(path, flag, id))
		{
			if (id == 0)
			{
				id = NextKey();
				m_assets.insert({ id, new ManagedAsset<ModelAsset>(new ModelAsset) });
				m_pathTOAssetID[path] = id;
			}
			ModelAsset* p = GetAsset<ModelAsset>(id); // GetAsset will clear the async bit of loadFlag
			m_assets[id]->loadFlag |= flag; // importand that we add the flag after call to GetAsset

			// Stop a potential unload;
			if (flag & AssetLoadFlag::CPUMemory)
				m_assets[id]->unLoadFlag &= ~AssetUnLoadFlag::AllCPU;
			if (flag & AssetLoadFlag::GPUMemory)
				m_assets[id]->unLoadFlag &= ~AssetUnLoadFlag::AllGPU;

			LoadShapeAssetInternal(path, shape, tessFactor1, tessFactor2, id, p);
		}
		assert(id != 0);
		return id;
	}

	u32 AssetManager::LoadTexture(const std::string& path, AssetLoadFlag flag)
	{
		u32 id = 0;
		if (AssetNeedsToBeLoaded(path, flag, id))
		{
			if (id == 0)
			{
				id = NextKey();
				m_assets.insert({ id, new ManagedAsset<TextureAsset>(new TextureAsset) });
				m_pathTOAssetID[path] = id;
			}
			TextureAsset* p = GetAsset<TextureAsset>(id);
			p->srgb = flag & AssetLoadFlag::Srgb;
			m_assets[id]->loadFlag |= flag;
			if (flag & AssetLoadFlag::CPUMemory)
				m_assets[id]->unLoadFlag &= ~AssetUnLoadFlag::AllCPU;
			if (flag & AssetLoadFlag::GPUMemory)
				m_assets[id]->unLoadFlag &= ~AssetUnLoadFlag::AllGPU;

			LoadTextureAssetInternal(path, id, p);
		}
		assert(id != 0);
		return id;
	}

	u32 AssetManager::LoadAudio(const std::string& path, AssetLoadFlag flag)
	{
		if (!std::filesystem::exists(path))
		{
			throw FileNotFoundError(path);
		}

		u32 id = 0;
		if (!AssetNeedsToBeLoaded(path, flag, id))
			return id;
		
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
		
		if(id == 0)
		{
			id = NextKey();
			m_assets.insert({ id, new ManagedAsset<AudioAsset>(newAudio) });
		}
		m_assets[id]->stateFlag = AssetStateFlag::ExistOnCPU;
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
		auto n = std::ssize(s_commandQueue);
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
		m_assets.insert({ id, new ManagedAsset<MeshAsset>(newMesh) });
		m_assets[id]->stateFlag = AssetStateFlag::ExistOnCPU;
		return id;
	}

	void AssetManager::UnLoadAsset(u32 id, AssetUnLoadFlag flag)
	{
		if (m_assets.contains(id) && !m_assets[id]->CheckIfLoadingAsync())
		{
			assert(m_assets[id]->stateFlag != AssetStateFlag::None);

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

	void AssetManager::LoadTextureSTBI(const std::string& path, TextureAsset* assetOut)
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
	}

	void AssetManager::LoadTextureCommpresonator(const std::string& path, TextureAsset* assetOut)
	{
		auto importedTex = TextureFileImporter(path, false).GetResult();
		assetOut->mipLevels = 1; // Mip maps will be handled later on when the assetTool is implemented.
		assetOut->width = importedTex->dataPerMip.front().width;
		assetOut->height = importedTex->dataPerMip.front().height;
		assert(static_cast<size_t>(assetOut->width) * assetOut->height * 4 == importedTex->dataPerMip.front().data.size());
		assetOut->textureData.resize(importedTex->dataPerMip.front().data.size());
		memcpy(assetOut->textureData.data(), importedTex->dataPerMip.front().data.data(), importedTex->dataPerMip.front().data.size());
	}

	void AssetManager::MoveModelToGPU(u32 modelID)
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
					std::this_thread::sleep_for(std::chrono::milliseconds(20)); // Wait a bit before spamming the main thread
					AssetManager::AddCommand([](u32 idToMove) { AssetManager::Get().MoveModelToGPU(idToMove); }, modelID);
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

		AssetManager::Get().UnLoadAsset(modelID, m_assets[modelID]->unLoadFlag);
	}

	void AssetManager::MoveTextureToGPU(u32 textureID)
	{
		if (!(m_assets[textureID]->loadFlag & AssetLoadFlag::GPUMemory))
		{
			assert(false);
			return;
		}

		assert(!(m_assets[textureID]->stateFlag & AssetStateFlag::ExistOnGPU));

		assert(m_renderer);
		gfx::GraphicsBuilder* builder = m_renderer->GetBuilder();

		gfx::GraphicsBuilder::MippedTexture2DSpecification textureSpec{};

		TextureAsset* asset = GetAsset<TextureAsset>(textureID);
		assert(asset); // Could this happen? And if it does i want to know about it. //nr of times it has happend: 1

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

		AssetManager::Get().UnLoadAsset(textureID, m_assets[textureID]->unLoadFlag);
	}

	u32 AssetManager::NextKey()
	{
		return ++m_lastKey;
	}

	bool AssetManager::AssetNeedsToBeLoaded(const std::string& path, AssetLoadFlag loadFlag, u32& assetIDOut)
	{
		assert(loadFlag & AssetLoadFlag::GPUMemory || loadFlag & AssetLoadFlag::CPUMemory);
		if (!std::filesystem::exists(path))
		{
			// assert wont catch if we have wrong path only in release mode
			std::cout << "AssetManager::AssetShouldBeLoaded throw. " + path + " does not exist" << std::endl;
			throw std::runtime_error(path + " does not exist");
		}

		u32 id = 0;
		bool assetNeedsToBeLoaded = true;
		if (m_pathTOAssetID.contains(path))
		{
			id = m_pathTOAssetID.at(path);
			assert(m_assets.contains(id));
			auto& managedAsset = m_assets[id];

			if (managedAsset->CheckIfLoadingAsync())
			{
				// We lack information on where it will be loaded, (need access to the load flag that the async function uses)
				// This can be problematic if a previus async later want to evict the resource from the memory type a later request wants it to
				// Assume that it will have the same load flag for now
				assetNeedsToBeLoaded = false;
			}
			else
			{
				bool existOnCpuMemory = managedAsset->stateFlag & AssetStateFlag::ExistOnCPU;
				bool requestedOnCpuMemory = managedAsset->loadFlag & AssetLoadFlag::CPUMemory;
				bool existOnGpuMemory = managedAsset->stateFlag & AssetStateFlag::ExistOnGPU;
				bool requestedOnGpuMemory = managedAsset->loadFlag & AssetLoadFlag::GPUMemory;

				bool needToLoadToCpu = false;
				bool needToLoadToGpu = false;

				if (loadFlag & AssetLoadFlag::CPUMemory)
				{
					needToLoadToCpu = !(existOnCpuMemory || requestedOnCpuMemory);
				}

				if (loadFlag & AssetLoadFlag::GPUMemory)
				{
					needToLoadToGpu = !(existOnGpuMemory || requestedOnGpuMemory);
				}
				assetNeedsToBeLoaded = needToLoadToCpu || needToLoadToGpu;
			}
		}

		assetIDOut = id;
		return assetNeedsToBeLoaded;
	}

	bool AssetManager::ShapeAssetNeedsToBeLoaded(const std::string& path, AssetLoadFlag loadFlag, u32& assetIDOut)
	{
		assert(loadFlag & AssetLoadFlag::GPUMemory || loadFlag & AssetLoadFlag::CPUMemory);

		u32 id = 0;
		bool assetNeedsToBeLoaded = true;
		if (m_pathTOAssetID.contains(path))
		{
			id = m_pathTOAssetID.at(path);
			assert(m_assets.contains(id));
			auto& managedAsset = m_assets[id];

			if (managedAsset->CheckIfLoadingAsync())
			{
				// We lack information on where it will be loaded, (need access to the load flag that the async function uses)
				// This can be problematic if a previus async later want to evict the resource from the memory type a later request wants it to
				// Assume that it will have the same load flag for now
				assetNeedsToBeLoaded = false;
			}
			else
			{
				bool existOnCpuMemory = managedAsset->stateFlag & AssetStateFlag::ExistOnCPU;
				bool requestedOnCpuMemory = managedAsset->loadFlag & AssetLoadFlag::CPUMemory;
				bool existOnGpuMemory = managedAsset->stateFlag & AssetStateFlag::ExistOnGPU;
				bool requestedOnGpuMemory = managedAsset->loadFlag & AssetLoadFlag::GPUMemory;

				bool needToLoadToCpu = false;
				bool needToLoadToGpu = false;

				if (loadFlag & AssetLoadFlag::CPUMemory)
				{
					needToLoadToCpu = !(existOnCpuMemory || requestedOnCpuMemory);
				}

				if (loadFlag & AssetLoadFlag::GPUMemory)
				{
					needToLoadToGpu = !(existOnGpuMemory || requestedOnGpuMemory);
				}
				assetNeedsToBeLoaded = needToLoadToCpu || needToLoadToGpu;
			}
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

	void AssetManager::ImguiLoadModel(bool&)
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::BeginMenu("Import"))
			{
				if (ImGui::BeginMenu("Model"))
				{
					if (ImGui::MenuItem("Sponza"))
					{
						constexpr std::string_view path = "Assets/Sponza_gltf/glTF/Sponza.gltf";
						if (std::filesystem::exists(path))
						{
							u32 id = DOG::AssetManager::Get().LoadModelAsset(path.data());
							std::cout << "model ID: " << id << ", path: " << path << std::endl;
						}
					}
					if (ImGui::MenuItem("Suzanne"))
					{
						constexpr std::string_view path = "Assets/suzanne.glb";
						if (std::filesystem::exists(path))
						{
							u32 id = DOG::AssetManager::Get().LoadModelAsset(path.data());
							std::cout << "model ID: " << id << ", path: " << path << std::endl;
						}
					}
					ImGui::EndMenu(); // "Model"
				}

				if (ImGui::BeginMenu("Texture"))
				{
					if (ImGui::MenuItem("Default"))
					{

					}
					ImGui::EndMenu(); // "Texture"
				}
				ImGui::EndMenu(); // "Import"
			}
			ImGui::EndMenu(); // "File"
		}
	}

	void AssetManager::AssetManagerGUI(bool& open)
	{
		if (ImGui::BeginMenu("View"))
		{
			if (ImGui::MenuItem("AssetManager"))
			{
				open = true;
			}
			ImGui::EndMenu(); // "View"
		}

		if (open)
		{
			if (ImGui::Begin("Asset manager", &open))
			{
				static int selectedModelPath = 0;
				std::array<const char*, 2> modelPaths = { "Assets/Sponza_gltf/glTF/Sponza.gltf", "Assets/suzanne.glb" };
				ImGui::Combo("Model path", &selectedModelPath, modelPaths.data(), static_cast<int>(modelPaths.size()));


				static int id = 0;
				if (ImGui::InputInt("Model ID", &id))
				{
					std::cout << id << std::endl;
				}
				ImGui::Separator();

				ImGui::Text("LoadFlag");
				static bool loadAsync = false;
				static bool loadCPU = false;
				static bool loadGPU = false;
				ImGui::Checkbox("load async", &loadAsync); ImGui::SameLine();
				ImGui::Checkbox("load RAM", &loadCPU); ImGui::SameLine();
				ImGui::Checkbox("load VRAM", &loadGPU); ImGui::Separator();
				AssetLoadFlag loadFlag = AssetLoadFlag::None;
				if (loadAsync) loadFlag |= AssetLoadFlag::Async;
				if (loadCPU) loadFlag |= AssetLoadFlag::CPUMemory;
				if (loadGPU) loadFlag |= AssetLoadFlag::GPUMemory;


				ImGui::Text("UnLoadFlag");
				static bool unloadCPU = false;
				static bool unloadGPU = false;
				ImGui::Checkbox("unload RAM", &unloadCPU); ImGui::SameLine();
				ImGui::Checkbox("unload VRAM", &unloadGPU); ImGui::Separator();
				AssetUnLoadFlag unloadFlag = AssetUnLoadFlag::None;
				if (unloadCPU) unloadFlag |= AssetUnLoadFlag::AllCPU;
				if (unloadGPU) unloadFlag |= AssetUnLoadFlag::AllGPU;


				if (ImGui::Button("LoadModel"))
				{
					if(loadFlag & AssetLoadFlag::GPUMemory || loadFlag & AssetLoadFlag::CPUMemory)
						id = AssetManager::Get().LoadModelAsset(modelPaths[selectedModelPath], loadFlag);
				}

				if (ImGui::Button("Unload"))
				{
					AssetManager::Get().UnLoadAsset(id, unloadFlag);
				}
			}
			ImGui::End(); // "Asset manager"
		}
	}
}