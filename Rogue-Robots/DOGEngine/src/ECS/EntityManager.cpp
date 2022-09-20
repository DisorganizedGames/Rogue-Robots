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
		for (u32 i{ 0u }; i < MAX_ENTITIES; i++)
		{
			m_entities.push_back(MAX_ENTITIES);
		}
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
		u32 indexToInsert = m_freeList.front();
		m_entities[indexToInsert] = indexToInsert;
		m_freeList.pop();
		return m_entities[indexToInsert];
	}

	void EntityManager::DestroyEntity(const entity entityID) noexcept
	{
		ASSERT(Exists(entityID), "Entity is invalid.");

		m_entities[entityID] = MAX_ENTITIES;
		for (u32 i{ 0u }; i < m_components.size(); i++)
		{
			if (m_components[i] != nullptr && entityID < m_components[i]->sparseArray.size()
				&& (m_components[i]->sparseArray[entityID] < m_components[i]->denseArray.size())
				&& (m_components[i]->sparseArray[entityID] != NULL_ENTITY))
			{
				const auto last = m_components[i]->denseArray.back();
				std::swap(m_components[i]->denseArray.back(), m_components[i]->denseArray[m_components[i]->sparseArray[entityID]]);
				//std::swap(m_components[i]->components.back(), m_components[i]->components[m_components[i]->sparseArray[entityID]]);
				std::swap(m_components[i]->sparseArray[last], m_components[i]->sparseArray[entityID]);
				m_components[i]->denseArray.pop_back();
				m_components[i]->sparseArray[entityID] = NULL_ENTITY;
			}
		}

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