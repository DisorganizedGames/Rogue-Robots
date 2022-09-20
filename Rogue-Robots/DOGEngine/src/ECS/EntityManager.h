#pragma once
#include "Component.h"
namespace DOG
{
	constexpr const u32 MAX_ENTITIES = 64'000u;
	constexpr const u32 NULL_ENTITY = MAX_ENTITIES;
	typedef u32 entity;
	struct SparseSetBase;
	class Collection;
	typedef std::unique_ptr<SparseSetBase> ComponentPool;
	#define set (static_cast<SparseSet<ComponentType>*>(m_components[ComponentType::ID].get())) 

	struct SparseSetBase
	{
		SparseSetBase() noexcept = default;
		virtual ~SparseSetBase() noexcept = default;
		std::vector<entity> sparseArray;
		std::vector<entity> denseArray;

		virtual void DestroyInternal(const entity entityID) = 0;
	};

	template<typename ComponentType>
	struct SparseSet : public SparseSetBase
	{
		SparseSet() noexcept = default;
		virtual ~SparseSet() noexcept override final = default;
		std::vector<ComponentType> components;
		
	private:
		virtual void DestroyInternal(const entity entityID) override final 
		{
			std::swap(components.back(), components[sparseArray[entityID]]);
			components.pop_back();
		}
	};

	class EntityManager
	{
	public:
		[[nodiscard]] static constexpr EntityManager& Get() noexcept { return s_instance; }

		entity CreateEntity() noexcept;
		void DestroyEntity(const entity entityID) noexcept;
		[[nodiscard]] std::vector<entity>& GetAllEntities() noexcept;
		void Reset() noexcept;
		[[nodiscard]] bool Exists(const entity entityID) const noexcept;

		template<typename ComponentType, typename ...Args>
		requires std::is_base_of_v<ComponentBase, ComponentType>
		ComponentType& AddComponent(const entity entityID, Args&& ...args) noexcept;

		template<typename ComponentType>
		requires std::is_base_of_v<ComponentBase, ComponentType>
		void RemoveComponent(const entity entityID) noexcept;

		template<typename ComponentType>
		requires std::is_base_of_v<ComponentBase, ComponentType>
		[[nodiscard]] ComponentType& GetComponent(const entity entityID) const noexcept;

		template<typename... ComponentType>
		requires std::is_base_of_v<ComponentBase, ComponentType...>
		[[nodiscard]] Collection GetCollection() noexcept;

		template<typename ComponentType>
		requires std::is_base_of_v<ComponentBase, ComponentType>
		[[nodiscard]] std::vector<entity>* GetEntitiesForComponentType() const noexcept;

		template<typename ComponentType>
		requires std::is_base_of_v<ComponentBase, ComponentType>
		[[nodiscard]] bool HasComponent(const entity entityID) const noexcept;

	private:
		EntityManager() noexcept;
		~EntityManager() noexcept = default;
		DELETE_COPY_MOVE_CONSTRUCTOR(EntityManager);

		void Initialize() noexcept;

		[[nodiscard]] bool HasComponentInternal(const u32 componentPoolIndex, const entity entityID) const noexcept;
		void DestroyComponentInternal(const u32 componentPoolIndex, const entity entityID) noexcept;

		template<typename ComponentType>
		requires std::is_base_of_v<ComponentBase, ComponentType>
		void ValidateComponentPool() noexcept;

		template<typename ComponentType> 
		requires std::is_base_of_v<ComponentBase, ComponentType>
		void AddSparseSet() noexcept;
	private:
		friend class Collection;
		static EntityManager s_instance;
		std::vector<entity> m_entities;
		std::queue<entity> m_freeList;
		std::vector<ComponentPool> m_components;
	};

	class Collection
	{
	public:
		explicit Collection(EntityManager* mgr, const size_t size) noexcept : m_mgr{ mgr } { m_entities.reserve(size); }

		template<typename... ComponentType>
		requires std::is_base_of_v<ComponentBase, ComponentType...>
		[[nodiscard]] std::tuple<ComponentType&...> Get(const entity entityID) const noexcept;

		[[nodiscard]] std::vector<entity>::reverse_iterator begin() { return m_entities.rbegin(); }
		[[nodiscard]] std::vector<entity>::reverse_iterator end() { return m_entities.rend(); }

	private:
		friend class EntityManager;
		void Add(entity entityID) noexcept;
	private:
		std::vector<entity> m_entities;
		EntityManager* m_mgr;
	};

	template<typename... ComponentType>
	requires std::is_base_of_v<ComponentBase, ComponentType...>
	std::tuple<ComponentType&...> Collection::Get(const entity entityID) const noexcept
	{
		return { m_mgr->GetComponent<ComponentType>(entityID)... };
	}

	template<typename ComponentType, typename ...Args>
	requires std::is_base_of_v<ComponentBase, ComponentType>
	ComponentType& EntityManager::AddComponent(const entity entityID, Args&& ...args) noexcept
	{
		ASSERT(Exists(entityID), "Entity is invalid");
		ASSERT(!HasComponent<ComponentType>(entityID), "Entity already has component!");

		ValidateComponentPool<ComponentType>();

		const size_t position = set->denseArray.size();
		set->denseArray.emplace_back(entityID);
		set->components.emplace_back(ComponentType(std::forward<Args>(args)...));
		set->sparseArray[entityID] = static_cast<entity>(position);

		return set->components.back();
	}

	template<typename ComponentType>
	requires std::is_base_of_v<ComponentBase, ComponentType>
	void EntityManager::RemoveComponent(const entity entityID) noexcept
	{
		ASSERT(Exists(entityID), "Entity is invalid");
		ASSERT(HasComponent<ComponentType>(entityID), "Entity does not have that component.");

		const auto last = set->denseArray.back();
		std::swap(set->denseArray.back(), set->denseArray[set->sparseArray[entityID]]);
		std::swap(set->components.back(), set->components[set->sparseArray[entityID]]);
		std::swap(set->sparseArray[last], set->sparseArray[entityID]);
		set->denseArray.pop_back();
		set->components.pop_back();
		set->sparseArray[entityID] = NULL_ENTITY;
	}

	template<typename ComponentType>
	requires std::is_base_of_v<ComponentBase, ComponentType>
	ComponentType& EntityManager::GetComponent(const entity entityID) const noexcept
	{
		ASSERT(Exists(entityID), "Entity is invalid");
		ASSERT(HasComponent<ComponentType>(entityID), "Entity does not have that component.");

		return set->components[set->sparseArray[entityID]];
	}

	template<typename... ComponentType>
	requires std::is_base_of_v<ComponentBase, ComponentType...>
	Collection EntityManager::GetCollection() noexcept
	{
		/*Create a better solution...*/
		std::vector<std::vector<entity>*> entityVectors = { GetEntitiesForComponentType<ComponentType>()... };
		std::vector<entity>* vectorWithLeastEntities = entityVectors[0];

		for (u32 i {0u}; i < entityVectors.size(); i++)
		{
			if (entityVectors[i]->size() < vectorWithLeastEntities->size())
			{
				vectorWithLeastEntities = entityVectors[i];
			}
		}
		
		Collection collection(this, vectorWithLeastEntities->size());
		for (u32 j{ 0u }; j < vectorWithLeastEntities->size(); j++)
		{
			bool existInAll = true;
			for (u32 k{ 0u }; k < entityVectors.size() && existInAll; k++)
			{
					if (std::find(entityVectors[k]->begin(), entityVectors[k]->end(), (*vectorWithLeastEntities)[j]) == entityVectors[k]->end())
					{
						existInAll = false;
					}
			}
			if (existInAll)
				collection.Add((*vectorWithLeastEntities)[j]);
		}
		return collection;
	}

	template<typename ComponentType>
	requires std::is_base_of_v<ComponentBase, ComponentType>
	std::vector<entity>* EntityManager::GetEntitiesForComponentType() const noexcept
	{
		ASSERT(m_components[ComponentType::ID] != nullptr, "Component type does not exist.");
		return &set->denseArray;
	}

	template<typename ComponentType> 
	requires std::is_base_of_v<ComponentBase, ComponentType>
	bool EntityManager::HasComponent(const entity entityID) const noexcept
	{
		ASSERT(Exists(entityID), "Entity is invalid");

		return 
			(
			m_components.capacity() > ComponentType::ID
			&& m_components[ComponentType::ID] != nullptr
			&& entityID < set->sparseArray.size()) 
			&& (set->sparseArray[entityID] < set->denseArray.size()) 
			&& (set->sparseArray[entityID] != NULL_ENTITY
			);
	}

	template<typename ComponentType>
	requires std::is_base_of_v<ComponentBase, ComponentType>
	void EntityManager::ValidateComponentPool() noexcept
	{
		if (ComponentType::ID >= m_components.capacity())
		{
			for (size_t i{ m_components.size() }; i < ComponentType::ID * 2; i++)
			{
				m_components.emplace_back(nullptr);
			}
		}
		if (m_components[ComponentType::ID] == nullptr)
		{
			AddSparseSet<ComponentType>();
		}
	}

	template<typename ComponentType> 
	requires std::is_base_of_v<ComponentBase, ComponentType>
	void EntityManager::AddSparseSet() noexcept
	{
		ASSERT((m_components.begin() + ComponentType::ID) < m_components.end(), "All available component pools are in use.");
		m_components[ComponentType::ID] = std::move(std::make_unique<SparseSet<ComponentType>>());

		set->sparseArray.reserve(MAX_ENTITIES);
		for (u32 i{ 0u }; i < MAX_ENTITIES; i++)
		{
			set->sparseArray.push_back(NULL_ENTITY);
		}

		set->denseArray.reserve(MAX_ENTITIES);
		set->components.reserve(MAX_ENTITIES);
	}
}