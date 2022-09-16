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
		m_entities.reserve(MAX_ENTITIES);
		for (u32 entityId{ 0u }; entityId < MAX_ENTITIES; entityId++)
			m_freeList.push(entityId);

		m_components.reserve(150);
	}

	entity EntityManager::CreateEntity() noexcept
	{
		ASSERT(!m_freeList.empty(), "Entity capacity reached!");
		m_entities.emplace_back(m_freeList.front());
		m_freeList.pop();
		return m_entities.back();
	}
}