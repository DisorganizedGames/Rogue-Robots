#include "EntityManager.h"
namespace DOG
{
	EntityManager EntityManager::s_instance;

	constexpr const u32 INITIAL_SYSTEM_CAPACITY = 250u;
	
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

		ECS_DEBUG_OP([&](){m_aliveEntities.push_back(indexToInsert); });

		return m_entities[indexToInsert];
	}

	void EntityManager::DestroyEntity(const entity entityID) noexcept
	{
		ASSERT(Exists(entityID), "Entity is invalid.");

		for (auto& [componentPoolIndex, componentPool] : m_components)
		{
			if (HasComponentInternal(componentPoolIndex, entityID))
			{
				DestroyComponentInternal(componentPoolIndex, entityID);
			}
		}
		m_entities[entityID] = MAX_ENTITIES;
		m_freeList.push(entityID);
		
		ECS_DEBUG_OP([&](){
			for (u32 i{ 0u }; i < m_aliveEntities.size(); ++i)
			{
				if (m_aliveEntities[i] == entityID)
				{
					std::swap(m_aliveEntities[i], m_aliveEntities[m_aliveEntities.size() - 1]);
					m_aliveEntities.pop_back();
					break;
				}
			}});
	}

	void EntityManager::DeferredEntityDestruction(const entity entityID) noexcept
	{
		//Add flag for deletion at the end of the frame
		AddComponent<DeferredDeletionComponent>(entityID);
	}

	void EntityManager::DestroyDeferredEntities() noexcept
	{
		//Destroy all of the entities with the deferred deletion flag set
		Collect<DeferredDeletionComponent>().Do([&](entity entity, DeferredDeletionComponent&)
			{
				DestroyEntity(entity);
			});
	}

	const std::vector<entity>& EntityManager::GetAllEntities() const noexcept
	{
		return m_entities;
	}

	void EntityManager::Reset() noexcept
	{
		std::queue<entity> temp;
		std::swap(m_freeList, temp);

		m_entities.clear();
		m_components.clear();
		m_bundles.clear();
		m_systems.clear();
		ECS_DEBUG_OP([&](){ m_aliveEntities.clear(); });

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

		m_systems.reserve(INITIAL_SYSTEM_CAPACITY);
		ECS_DEBUG_OP([&]() { m_aliveEntities.reserve(MAX_ENTITIES); });
	}

	[[nodiscard]] bool EntityManager::HasComponentInternal(const sti::TypeIndex componentPoolIndex, const entity entityID) const noexcept
	{
		return m_components.contains(componentPoolIndex) && entityID < m_components.at(componentPoolIndex)->sparseArray.size()
			&& (m_components.at(componentPoolIndex)->sparseArray[entityID] < m_components.at(componentPoolIndex)->denseArray.size())
			&& (m_components.at(componentPoolIndex)->sparseArray[entityID] != NULL_ENTITY);
	}

	void EntityManager::DestroyComponentInternal(const sti::TypeIndex componentPoolIndex, const entity entityID) noexcept
	{
		if (m_components.at(componentPoolIndex)->bundle != nullptr)
		{
			m_bundles[m_components.at(componentPoolIndex)->bundle]->UpdateOnRemove(entityID);
		}

		const auto last = m_components.at(componentPoolIndex)->denseArray.back();
		std::swap(m_components.at(componentPoolIndex)->denseArray.back(), m_components.at(componentPoolIndex)->denseArray[m_components.at(componentPoolIndex)->sparseArray[entityID]]);
		m_components.at(componentPoolIndex)->DestroyInternal(entityID);
		std::swap(m_components.at(componentPoolIndex)->sparseArray[last], m_components.at(componentPoolIndex)->sparseArray[entityID]);
		m_components.at(componentPoolIndex)->denseArray.pop_back();
		m_components.at(componentPoolIndex)->sparseArray[entityID] = NULL_ENTITY;
	}

	void EntityManager::RegisterSystem(std::unique_ptr<ISystem>&& pSystem) noexcept
	{
		ECS_DEBUG_OP([&]() { for (auto& system : m_systems) ASSERT(typeid(*system) != typeid(*pSystem), "System already exists."); })
		pSystem->Create();
		m_systems.emplace_back(std::move(pSystem));
	}
}