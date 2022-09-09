#include "MaterialManager.h"
namespace DOG
{
	u64 MaterialManager::AddMaterial(const Material& newMaterial)
	{
		m_materials.emplace_back(newMaterial).dirty = true;
		return m_materials.size();
	}

	Material& MaterialManager::GetMaterial(u64 materialID)
	{
		assert(0 < materialID && materialID <= m_materials.size());
		m_materials[materialID - 1].dirty = true;
		return m_materials[materialID - 1];
	}

	const Material& MaterialManager::GetMaterial(u64 materialID) const
	{
		assert(0 < materialID && materialID <= m_materials.size());
		return m_materials[materialID-1];
	}
}