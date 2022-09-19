#include "EntityManager.h"
namespace DOG
{

	static u32 componentID{ 0u };

	const u32 ComponentBase::GetID() noexcept
	{
		return componentID++;
	}

	EntityManager::EntityManager() noexcept
	{
		Initialize();
	}

	void EntityManager::Initialize() noexcept
	{
		m_entities.reserve(MAX_ENTITIES);
		for (u32 entityId{ 0u }; entityId < MAX_ENTITIES; entityId++)
			m_freeList.push(entityId);

		m_components.resize(150);
		//m_components.reserve(150);
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
}