#include "EntityManager.h"
namespace DOG
{
	constexpr const u32 INITIAL_COMPONENT_CAPACITY{ 8u };

	EntityManager EntityManager::s_instance;
	
	EntityManager::EntityManager() noexcept
	{
		Initialize();
	}

	entity EntityManager::CreateEntity() noexcept
	{
		ASSERT(!m_freeList.empty(), "Entity capacity reached!");
		u32 indexToInsert = m_freeList.front();
		m_entities[indexToInsert] = indexToInsert;
		m_freeList.pop();
		return m_entities[indexToInsert];
	}

	void EntityManager::DestroyEntity(const entity entityID) noexcept
	{
		ASSERT(Exists(entityID), "Entity is invalid.");

		m_entities[entityID] = MAX_ENTITIES;
		for (u32 componentPoolIndex{ 0u }; componentPoolIndex < m_components.size(); componentPoolIndex++)
		{
			if (HasComponentInternal(componentPoolIndex, entityID))
			{
				DestroyComponentInternal(componentPoolIndex, entityID);
			}
		}
		m_freeList.push(entityID);
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

	void EntityManager::Initialize() noexcept
	{
		m_entities.reserve(MAX_ENTITIES);
		for (u32 i{ 0u }; i < MAX_ENTITIES; i++)
		{
			m_entities.emplace_back(MAX_ENTITIES);
		}

		for (u32 entityId{ 0u }; entityId < MAX_ENTITIES; entityId++)
			m_freeList.push(entityId);

		m_components.reserve(INITIAL_COMPONENT_CAPACITY);
		for (u32 i{ 0u }; i < INITIAL_COMPONENT_CAPACITY; i++)
		{
			m_components.emplace_back(nullptr);
		}
	}

	[[nodiscard]] bool EntityManager::HasComponentInternal(const u32 componentPoolIndex, const entity entityID) const noexcept
	{
		return m_components[componentPoolIndex] != nullptr && entityID < m_components[componentPoolIndex]->sparseArray.size()
			&& (m_components[componentPoolIndex]->sparseArray[entityID] < m_components[componentPoolIndex]->denseArray.size())
			&& (m_components[componentPoolIndex]->sparseArray[entityID] != NULL_ENTITY);
	}

	void EntityManager::DestroyComponentInternal(const u32 componentPoolIndex, const entity entityID) noexcept
	{
		const auto last = m_components[componentPoolIndex]->denseArray.back();
		std::swap(m_components[componentPoolIndex]->denseArray.back(), m_components[componentPoolIndex]->denseArray[m_components[componentPoolIndex]->sparseArray[entityID]]);
		m_components[componentPoolIndex]->DestroyInternal(entityID);
		std::swap(m_components[componentPoolIndex]->sparseArray[last], m_components[componentPoolIndex]->sparseArray[entityID]);
		m_components[componentPoolIndex]->denseArray.pop_back();
		m_components[componentPoolIndex]->sparseArray[entityID] = NULL_ENTITY;
	}
}