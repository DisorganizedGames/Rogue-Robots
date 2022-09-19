#pragma once
namespace DOG
{
	constexpr const u32 MAX_ENTITIES = 64'000u;
	constexpr const u32 NULL_ENTITY = MAX_ENTITIES;
	typedef u32 entity;
	struct SparseSetBase;
	typedef std::unique_ptr<SparseSetBase> ComponentPool;

	struct ComponentBase
	{
		ComponentBase() noexcept = default;
		virtual ~ComponentBase() noexcept = default;
		[[nodiscard]] static const u32 GetID() noexcept;
	};

	template<typename T>
	struct Component : public ComponentBase
	{
		Component() noexcept = default;
		virtual ~Component() noexcept override = default;
		static u32 ID;
	};

	template<typename T>
	u32 Component<T>::ID{ ComponentBase::GetID() };

	struct SparseSetBase
	{
		SparseSetBase() noexcept = default;
		virtual ~SparseSetBase() noexcept = default;
	};

	template<typename ComponentType>
	struct SparseSet : public SparseSetBase
	{
		SparseSet() noexcept = default;
		virtual ~SparseSet() noexcept override final = default;
		std::vector<entity> sparseArray;
		std::vector<entity> denseArray;
		std::vector<ComponentType> components;
	};

	struct TransformComponent : public Component<TransformComponent>
	{
		TransformComponent(Vector3f position = {0.0f, 0.0f, 0.0f}) noexcept : position{position} {}
		virtual ~TransformComponent() noexcept override final = default;
		Vector3f position;
	};

	struct MeshComponent : public Component<MeshComponent>
	{
		MeshComponent(u32 id = 0) noexcept : id{ id } {}
		virtual ~MeshComponent() noexcept override final = default;
		u32 id;
	};

	class EntityManager
	{
	public:
		EntityManager() noexcept;
		~EntityManager() noexcept = default;

		entity CreateEntity() noexcept;
		[[nodiscard]] std::vector<entity>& GetAllEntities() noexcept;
		void Reset() noexcept;

		template<typename ComponentType, typename ...Args>
		ComponentType& AddComponent(const entity entityID, Args&& ...args) noexcept;

		template<typename ComponentType>
		void RemoveComponent(const entity entityID) noexcept;

		template<typename ComponentType>
		[[nodiscard]] ComponentType& GetComponent(const entity entityID) const noexcept;

		template<typename ComponentType>
		[[nodiscard]] std::vector<ComponentType>& GetComponentsOfType() noexcept;

		template<typename... ComponentType>
		void GetByCollection() noexcept;

		template<typename ComponentType>
		void bar() noexcept;

		template<typename ComponentType>
		[[nodiscard]] bool HasComponent(const entity entityID) const noexcept;
	private:
		template<typename ComponentType>
		void AddSparseSet() noexcept;
	private:
		std::vector<entity> m_entities;
		std::queue<entity> m_freeList;
		std::vector<ComponentPool> m_components;
	};

	template<typename ComponentType, typename ...Args>
	ComponentType& EntityManager::AddComponent(const entity entityID, Args&& ...args) noexcept
	{
		static_assert(std::is_base_of<ComponentBase, ComponentType>::value);
		ASSERT(!HasComponent<ComponentType>(entityID), "Entity already has component!");
		#define set (static_cast<SparseSet<ComponentType>*>(m_components[ComponentType::ID].get())) 

		if (!(m_components.size() > ComponentType::ID))
		{
			AddSparseSet<ComponentType>();
		}

		const size_t position = set->denseArray.size();
		set->denseArray.emplace_back(entityID);
		set->components.emplace_back(ComponentType(std::forward<Args>(args)...));
		set->sparseArray[entityID] = static_cast<entity>(position);

		return set->components.back();
	}

	template<typename ComponentType>
	void EntityManager::RemoveComponent(const entity entityID) noexcept
	{
		static_assert(std::is_base_of<ComponentBase, ComponentType>::value);
		ASSERT(HasComponent<ComponentType>(entityID), "Entity does not have that component.");
		#define set (static_cast<SparseSet<ComponentType>*>(m_components[ComponentType::ID].get())) 

		const auto last = set->denseArray.back();
		std::swap(set->denseArray.back(), set->denseArray[set->sparseArray[entityID]]);
		std::swap(set->components.back(), set->components[set->sparseArray[entityID]]);
		std::swap(set->sparseArray[last], set->sparseArray[entityID]);
		set->denseArray.pop_back();
		set->components.pop_back();
		set->sparseArray[entityID] = NULL_ENTITY;
	}

	template<typename ComponentType>
	ComponentType& EntityManager::GetComponent(const entity entityID) const noexcept
	{
		static_assert(std::is_base_of<ComponentBase, ComponentType>::value);
		ASSERT(HasComponent<ComponentType>(entityID), "Entity does not have that component.");
		#define set (static_cast<SparseSet<ComponentType>*>(m_components[ComponentType::ID].get())) 

		return set->components[set->sparseArray[entityID]];
	}

	template<typename ComponentType>
	std::vector<ComponentType>& EntityManager::GetComponentsOfType() noexcept
	{
		static_assert(std::is_base_of<ComponentBase, ComponentType>::value);
		#define set (static_cast<SparseSet<ComponentType>*>(m_components[ComponentType::ID].get())) 

		return set->components;
	}

	template<typename... ComponentType>
	void EntityManager::GetByCollection() noexcept
	{
		constexpr auto num_types = sizeof... (ComponentType);
		std::array<uint32_t, num_types> test;
		(bar<ComponentType>(), ...);
	}

	template<typename ComponentType>
	void EntityManager::bar() noexcept
	{
		std::cout << "Component ID: " << ComponentType::ID;
	}

	template<typename ComponentType>
	bool EntityManager::HasComponent(const entity entityID) const noexcept
	{
		static_assert(std::is_base_of<ComponentBase, ComponentType>::value);
		#define set (static_cast<SparseSet<ComponentType>*>(m_components[ComponentType::ID].get())) 
		
		return 
			(
			m_components.size() > ComponentType::ID
			&& entityID < set->sparseArray.size()) 
			&& (set->sparseArray[entityID] < set->denseArray.size()) 
			&& (set->sparseArray[entityID] != NULL_ENTITY
			);
	}

	template<typename ComponentType>
	void EntityManager::AddSparseSet() noexcept
	{
		static_assert(std::is_base_of<ComponentBase, ComponentType>::value);
		#define set (static_cast<SparseSet<ComponentType>*>(m_components[ComponentType::ID].get()))

		m_components.emplace_back(std::make_unique<SparseSet<ComponentType>>()); //PoolAllocator?

		set->sparseArray.reserve(MAX_ENTITIES);
		for (u32 i{ 0u }; i < MAX_ENTITIES; i++)
		{
			set->sparseArray.push_back(NULL_ENTITY);
		}

		set->denseArray.reserve(MAX_ENTITIES);
		set->components.reserve(MAX_ENTITIES);
	}
}