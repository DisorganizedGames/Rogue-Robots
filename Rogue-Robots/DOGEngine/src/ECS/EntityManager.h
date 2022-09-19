#pragma once
namespace DOG
{
	constexpr const u32 MAX_ENTITIES = 64'000u;
	constexpr const u32 NULL_ENTITY = MAX_ENTITIES;
	typedef u32 entity;
	struct SparseSetBase;
	typedef std::unique_ptr<SparseSetBase> ComponentPool;

	class generator {
		inline static std::size_t counter{};

	public:
		template<typename Type>
		inline static const std::size_t type = counter++;
	};

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

	struct SoundComponent : public Component<SoundComponent>
	{
		SoundComponent(u32 id = 0) noexcept : id{ id } {}
		virtual ~SoundComponent() noexcept override final = default;
		u32 id;
	};

	class Collection;
	class EntityManager
	{
	public:
		EntityManager() noexcept;
		~EntityManager() noexcept = default;

		void Initialize() noexcept;
		entity CreateEntity() noexcept;
		[[nodiscard]] std::vector<entity>& GetAllEntities() noexcept;
		void Reset() noexcept;
		[[nodiscard]] bool Exists(const entity entityID) const noexcept;

		template<typename ComponentType, typename ...Args>
		ComponentType& AddComponent(const entity entityID, Args&& ...args) noexcept;

		template<typename ComponentType>
		void RemoveComponent(const entity entityID) noexcept;

		template<typename ComponentType>
		[[nodiscard]] ComponentType& GetComponent(const entity entityID) const noexcept;

		template<typename ComponentType>
		[[nodiscard]] std::vector<ComponentType>& GetComponentsOfType() noexcept;

		template<typename... ComponentType>
		Collection GetByCollection() noexcept;

		template<typename ComponentType>
		std::vector<entity>* bar() noexcept;

		template<typename ComponentType>
		[[nodiscard]] bool HasComponent(const entity entityID) const noexcept;

	private:
		template<typename ComponentType>
		void AddSparseSet() noexcept;
	private:
		friend class Collection;
		std::vector<entity> m_entities;
		std::queue<entity> m_freeList;
		std::vector<ComponentPool> m_components;
	};

	class Collection
	{
	public:
		explicit Collection(EntityManager* mgr) noexcept : mgr{ mgr } {}
		template<typename... ComponentType>
		std::tuple<ComponentType...> Get(const entity entityID) noexcept;

		std::vector<entity> entities;
		[[nodiscard]] std::vector<entity>::reverse_iterator begin() { return entities.rbegin(); }
		[[nodiscard]] std::vector<entity>::reverse_iterator end() { return entities.rend(); }

	private:
		EntityManager* mgr;
	};

	template<typename... ComponentType>
	std::tuple<ComponentType...> Collection::Get(const entity entityID) noexcept
	{
		return { mgr->GetComponent<ComponentType>(entityID)... };
	}

	template<typename ComponentType, typename ...Args>
	ComponentType& EntityManager::AddComponent(const entity entityID, Args&& ...args) noexcept
	{
		static_assert(std::is_base_of<ComponentBase, ComponentType>::value);
		ASSERT(Exists(entityID), "Entity is invalid");
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
		ASSERT(Exists(entityID), "Entity is invalid");
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
		ASSERT(Exists(entityID), "Entity is invalid");
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
	Collection EntityManager::GetByCollection() noexcept
	{
		std::vector<std::vector<entity>*> test = { bar<ComponentType>()... };
		std::vector<entity>* vectorWithLeastComponents = test[0];

		for (u32 i {0u}; i < test.size(); i++)
		{
			if (test[i]->size() < vectorWithLeastComponents->size())
			{
				vectorWithLeastComponents = test[i];
			}
		}
		
		std::vector<entity> finalVector;

		std::vector<entity>::iterator it = vectorWithLeastComponents->end();
		for (u32 j{ 0u }; j < vectorWithLeastComponents->size(); j++)
		{
			bool existInAll = true;
			for (u32 k{ 0u }; k < test.size() && existInAll; k++)
			{
					if (std::find(test[k]->begin(), test[k]->end(), (*vectorWithLeastComponents)[j]) == test[k]->end())
					{
						existInAll = false;
						it--;
					}
			}
			if (existInAll)
				finalVector.push_back((*vectorWithLeastComponents)[j]);
		}
		Collection c(this);
		c.entities = finalVector;
		return c;
	}

	template<typename ComponentType>
	std::vector<entity>* EntityManager::bar() noexcept
	{
		#define set (static_cast<SparseSet<ComponentType>*>(m_components[ComponentType::ID].get()))
		return &set->denseArray;
	}

	template<typename ComponentType> 
	bool EntityManager::HasComponent(const entity entityID) const noexcept
	{
		static_assert(std::is_base_of<ComponentBase, ComponentType>::value);
		ASSERT(Exists(entityID), "Entity is invalid");
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
		std::cout << ComponentType::ID;
		static_assert(std::is_base_of<ComponentBase, ComponentType>::value);
		m_components.emplace_back(std::move(std::make_unique<SparseSet<ComponentType>>())); //PoolAllocator?
		#define set (static_cast<SparseSet<ComponentType>*>(m_components[ComponentType::ID].get()))

		std::cout << ComponentType::ID;

		set->sparseArray.reserve(MAX_ENTITIES);
		for (u32 i{ 0u }; i < MAX_ENTITIES; i++)
		{
			set->sparseArray.push_back(NULL_ENTITY);
		}

		set->denseArray.reserve(MAX_ENTITIES);
		set->components.reserve(MAX_ENTITIES);
	}
}