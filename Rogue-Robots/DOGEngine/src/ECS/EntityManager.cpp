#include "EntityManager.h"
namespace DOG
{
	constexpr const u32 MAX_COMPONENT_TYPES{ 250u };
	
	EntityManager::EntityManager() noexcept
	{
		Initialize();
	}

	void EntityManager::Initialize() noexcept
	{
		m_entities.reserve(MAX_ENTITIES);
		for (u32 entityId{ 0u }; entityId < MAX_ENTITIES; entityId++)
			m_freeList.push(entityId);

		m_components.resize(MAX_COMPONENT_TYPES);
		for (u32 i{ 0u }; i < MAX_COMPONENT_TYPES; i++)
		{
			m_components.push_back(nullptr);
		}
	}

	entity EntityManager::CreateEntity() noexcept
	{
		ASSERT(!m_freeList.empty(), "Entity capacity reached!");
		m_entities.emplace_back(m_freeList.front());
		m_freeList.pop();
		return m_entities.back();
	}

	std::vector<entity>& EntityManager::GetAllEntities() noexcept
	{
		return m_entities;
	}

	void EntityManager::Reset() noexcept
	{
		std::queue<entity> temp;
		std::swap(m_freeList, temp);

		m_entities.clear();
		m_components.clear();
		Initialize();
	}

	bool EntityManager::Exists(const entity entityID) const noexcept
	{
		return ((entityID < m_entities.size()) && (m_entities[entityID] == entityID));
	}

	void Collection::Add(entity entityID) noexcept
	{
		m_entities.emplace_back(entityID);
	}
}