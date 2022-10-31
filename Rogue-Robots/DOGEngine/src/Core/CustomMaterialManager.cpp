#include "CustomMaterialManager.h"
#include "../Graphics/Rendering/Renderer.h"
#include "../Graphics/Rendering/MaterialTable.h"

namespace DOG
{
	CustomMaterialManager* CustomMaterialManager::s_instance = nullptr;

	void CustomMaterialManager::Initialize(gfx::Renderer* renderer)
	{
		if (!s_instance)
			s_instance = new CustomMaterialManager(renderer);
	}

	void CustomMaterialManager::Destroy()
	{
		if (s_instance)
		{
			delete s_instance;
			s_instance = nullptr;
		}
	}

	CustomMaterialManager& CustomMaterialManager::Get()
	{
		assert(s_instance);
		return *s_instance;
	}


	CustomMaterialManager::CustomMaterialManager(gfx::Renderer* renderer) :
		m_materialTable(renderer->GetMaterialTable())
	{
		
	}

	MaterialHandle CustomMaterialManager::AddMaterial(const MaterialDesc& desc)
	{
		gfx::MaterialTable::MaterialSpecification spec;
		spec.albedo = desc.albedo;
		spec.normal = desc.normal;
		spec.emissive = desc.emissive;
		spec.metallicRoughness = desc.metallicRoughness;

		spec.albedoFactor = desc.albedoFactor;
		spec.roughnessFactor = desc.roughnessFactor;
		spec.emissiveFactor = desc.emissiveFactor;
		spec.metallicFactor = desc.metallicFactor;

		auto ret = m_materialTable->LoadMaterial(spec);

		m_refs[ret.handle] = 1;
		return ret;
	}

	void CustomMaterialManager::UpdateMaterial(MaterialHandle handle, const MaterialDesc& desc)
	{
		gfx::MaterialTable::MaterialSpecification spec;
		spec.albedo = desc.albedo;
		spec.normal = desc.normal;
		spec.emissive = desc.emissive;
		spec.metallicRoughness = desc.metallicRoughness;

		spec.albedoFactor = desc.albedoFactor;
		spec.roughnessFactor = desc.roughnessFactor;
		spec.emissiveFactor = desc.emissiveFactor;
		spec.metallicFactor = desc.metallicFactor;

		m_materialTable->UpdateMaterial(handle, spec);
	}	

	void CustomMaterialManager::RemoveMaterial(MaterialHandle handle)
	{
		m_materialTable->FreeMaterial(handle);
	}

	void CustomMaterialManager::AddRef(MaterialHandle handle)
	{
		m_refs[handle.handle]++;
	}

	void CustomMaterialManager::DecrRef(MaterialHandle handle)
	{
		auto& ref = m_refs[handle.handle];
		assert(ref > 0);

		--ref;
		if (ref == 0)	// automatically remove material if no refs left
			RemoveMaterial(handle);
	}


}