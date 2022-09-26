#pragma once
#include "Component.h"
namespace DOG
{
	constexpr const u32 MAX_ENTITIES = 64'000u;
	constexpr const u32 NULL_ENTITY = MAX_ENTITIES;
	typedef u32 entity;
	struct SparseSetBase;

	template<typename... ComponentType>
	class Collection;

	class BundleBase;
	template<typename... ComponentType>
	class BundleImpl;
	
	typedef std::unique_ptr<SparseSetBase> ComponentPool;
	#define set (static_cast<SparseSet<ComponentType>*>(m_components[ComponentType::ID].get())) 

	//##################### SPARSE SET #####################

	struct SparseSetBase
	{
		SparseSetBase() noexcept = default;
		virtual ~SparseSetBase() noexcept = default;
		std::vector<entity> sparseArray;
		std::vector<entity> denseArray;
		int bundle = -1;

		virtual void DestroyInternal(const entity entityID) noexcept = 0;
	};

	template<typename ComponentType>
	struct SparseSet : public SparseSetBase
	{
		SparseSet() noexcept = default;
		virtual ~SparseSet() noexcept override final = default;
		std::vector<ComponentType> components;
		
	private:
		virtual void DestroyInternal(const entity entityID) noexcept override final 
		{
			std::swap(components.back(), components[sparseArray[entityID]]);
			components.pop_back();
		}
	};

	//##################### ENTITY MANAGER #####################

	class EntityManager
	{
	public:
		[[nodiscard]] static constexpr EntityManager& Get() noexcept { return s_instance; }

		entity CreateEntity() noexcept;
		void DestroyEntity(const entity entityID) noexcept;
		[[nodiscard]] const std::vector<entity>& GetAllEntities() const noexcept;
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
		requires (std::is_base_of_v<ComponentBase, ComponentType> && ...)
		[[nodiscard]] Collection<ComponentType...> Collect() noexcept;

		template<typename ComponentType>
		requires std::is_base_of_v<ComponentBase, ComponentType>
		[[nodiscard]] bool HasComponent(const entity entityID) const noexcept;

		template<typename... ComponentType>
		requires (std::is_base_of_v<ComponentBase, ComponentType> && ...)
		[[nodiscard]] bool HasAllOf(const entity entityID) const noexcept;

		template<typename... ComponentType>
		requires (std::is_base_of_v<ComponentBase, ComponentType> && ...)
		[[nodiscard]] bool HasAnyOf(const entity entityID) const noexcept;

		template<typename... ComponentType>
		requires (std::is_base_of_v<ComponentBase, ComponentType> && ...)
		[[nodiscard]] BundleImpl<ComponentType...>& Bundle() noexcept;

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
		template<typename... ComponentType>
		friend class Collection;
		template<typename... ComponentType>
		friend class BundleImpl;
		static EntityManager s_instance;
		std::vector<entity> m_entities;
		std::queue<entity> m_freeList;
		std::vector<ComponentPool> m_components;
		std::vector<std::unique_ptr<BundleBase>> m_bundles;
	};

	template<typename... ComponentType>
	requires (std::is_base_of_v<ComponentBase, ComponentType> && ...)
	BundleImpl<ComponentType...>& EntityManager::Bundle() noexcept
	{
		std::tuple<SparseSet<ComponentType>*...> pools = { set... };
		const std::array<u32, sizeof... (ComponentType)> componentIDs{ ComponentType::ID... };
		auto minComponentID = *std::min_element(componentIDs.begin(), componentIDs.end());

		if (m_bundles[minComponentID] == nullptr)
		{
#if defined _DEBUG
			std::apply([](const auto&... pool) {(ASSERT((pool != nullptr) && pool->bundle == -1, "Bundle creation failed."), ...); }, pools);
#endif
			std::apply([&](const auto&... pool) {((pool->bundle = minComponentID), ...); }, pools);
			m_bundles[minComponentID] = (std::unique_ptr<BundleImpl<ComponentType...>>(new BundleImpl<ComponentType...>(this, { set... })));
		}
		return *(static_cast<BundleImpl<ComponentType...>*>(m_bundles[minComponentID].get()));
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

		if (set->bundle != -1)
		{
			auto componentIdx = m_bundles[set->bundle]->UpdateOnAdd(entityID);
			if (componentIdx)
			{
				return set->components[*componentIdx];
			}
		}
			return set->components.back();
	}

	template<typename ComponentType>
	requires std::is_base_of_v<ComponentBase, ComponentType>
	void EntityManager::RemoveComponent(const entity entityID) noexcept
	{
		ASSERT(Exists(entityID), "Entity is invalid");
		ASSERT(HasComponent<ComponentType>(entityID), "Entity does not have that component.");

		if (set->bundle != -1)
		{
			m_bundles[set->bundle]->UpdateOnRemove(entityID);
		}

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
	requires (std::is_base_of_v<ComponentBase, ComponentType> && ...)
	Collection<ComponentType...> EntityManager::Collect() noexcept
	{
		return Collection<ComponentType...>( this, {set... });
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

	template<typename... ComponentType>
	requires (std::is_base_of_v<ComponentBase, ComponentType> && ...)
	bool EntityManager::HasAllOf(const entity entityID) const noexcept
	{
		ASSERT(Exists(entityID), "Entity is invalid");

		return (EntityManager::HasComponent<ComponentType>(entityID) && ...);
	}

	template<typename... ComponentType>
	requires (std::is_base_of_v<ComponentBase, ComponentType> && ...)
	bool EntityManager::HasAnyOf(const entity entityID) const noexcept
	{
		ASSERT(Exists(entityID), "Entity is invalid");

		return (EntityManager::HasComponent<ComponentType>(entityID) || ...);
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

	//##################### COLLECTIONS #####################

	template<typename... ComponentType>
	class Collection
	{
	public:
		explicit Collection(EntityManager* mgr, const std::tuple<SparseSet<ComponentType>*...>& pools) noexcept;
		~Collection() noexcept = default;

		void Do(std::invocable<ComponentType&...> auto&& func) const noexcept;
		void Do(std::invocable<entity, ComponentType&...> auto&& func) const noexcept;

	private:
		DELETE_COPY_MOVE_CONSTRUCTOR(Collection);
		friend class EntityManager;
		std::tuple<SparseSet<ComponentType>*...> m_pools;
		EntityManager* m_mgr;
	};

	template<typename... ComponentType>
	Collection<ComponentType...>::Collection(EntityManager* mgr, const std::tuple<SparseSet<ComponentType>*...>& pools) noexcept
		: m_mgr{ mgr }, m_pools(std::move(pools))
	{
#if defined _DEBUG
		std::apply([](const auto... pool) {(ASSERT(pool, "Component type does not exist."), ...); }, m_pools);
#endif
	}

	template<typename... ComponentType>
	void Collection<ComponentType...>::Do(std::invocable<ComponentType&...> auto&& func) const noexcept
	{
		std::vector<entity>* ePointer = &(get<0>(m_pools)->denseArray);
		std::apply([&ePointer](const auto&... pool)
			{
				((ePointer = (pool->denseArray.size() < ePointer->size()) ? &pool->denseArray : ePointer), ...);
			}, m_pools);

		for (int i{ (int)ePointer->size() - 1 }; i >= 0; --i)
		{
			if (m_mgr->HasAllOf<ComponentType...>((*ePointer)[i]))
			{
				func(m_mgr->GetComponent<ComponentType>((*ePointer)[i])...);
			}
		}
	}

	template<typename... ComponentType>
	void Collection<ComponentType...>::Do(std::invocable<entity, ComponentType&...> auto&& func) const noexcept
	{
		std::vector<entity>* ePointer = &(get<0>(m_pools)->denseArray);
		std::apply([&ePointer](const auto&... pool)
			{
				((ePointer = (pool->denseArray.size() < ePointer->size()) ? &pool->denseArray : ePointer), ...);
			}, m_pools);

		for (int i{ (int)ePointer->size() - 1 }; i >= 0; --i)
		{
			if (m_mgr->HasAllOf<ComponentType...>((*ePointer)[i]))
			{
				func((*ePointer)[i], m_mgr->GetComponent<ComponentType>((*ePointer)[i])...);
			}
		}
	}

	//##################### BUNDLES #####################

	class BundleBase
	{
	public:
		BundleBase() noexcept = default;
		virtual ~BundleBase() noexcept = default;

		virtual [[nodiscard]] std::optional<u32> UpdateOnAdd(const entity entityID) noexcept = 0;
		virtual void UpdateOnRemove(const entity entityID) noexcept = 0;
	};

	template<typename... ComponentType>
	class BundleImpl : public BundleBase
	{
	public:
		explicit BundleImpl(EntityManager* mgr, const std::tuple<SparseSet<ComponentType>*...>& pools) noexcept;
		virtual ~BundleImpl() noexcept override final = default;

		void Do(std::invocable<ComponentType&...> auto&& func) const noexcept;
		void Do(std::invocable<entity, ComponentType&...> auto&& func) const noexcept;
	private:
		DELETE_COPY_MOVE_CONSTRUCTOR(BundleImpl);
		[[nodiscard]] std::optional<u32> UpdateOnAdd(const entity entityID) noexcept override final;
		virtual void UpdateOnRemove(const entity entityID) noexcept override final;
	private:
		friend EntityManager;
		EntityManager* m_mgr;
		std::tuple<SparseSet<ComponentType>*...> m_pools;
		std::vector<entity>* ePointer;
		int m_bundleStart;
	};

	template<typename... ComponentType>
	BundleImpl<ComponentType...>::BundleImpl(EntityManager* mgr, const std::tuple<SparseSet<ComponentType>*...>& pools) noexcept
		: m_mgr{ mgr }, m_pools{ std::move(pools) }, m_bundleStart{ -1 }, ePointer{ nullptr }
	{
#if defined _DEBUG
		std::apply([](const auto... pool) {(ASSERT(pool, "Component type does not exist."), ...); }, m_pools);
#endif
		ePointer = &(get<0>(m_pools)->denseArray);
		std::apply([&](const auto... pool)
			{
				((ePointer = (pool->denseArray.size() < ePointer->size()) ? &pool->denseArray : ePointer), ...);
			}, m_pools);

		const auto&& myLambda = []<typename ComponentType>(SparseSet<ComponentType>*pool, u32 index, int bundleStart)
		{
			std::swap(pool->denseArray[index], pool->denseArray[bundleStart + 1]);
			std::swap(pool->components[index], pool->components[bundleStart + 1]);
			std::swap(pool->sparseArray[pool->denseArray[index]], pool->sparseArray[pool->denseArray[bundleStart + 1]]);
		};

		for (u32 i{ 0u }; i < ePointer->size(); ++i)
		{
			if ((m_mgr->HasComponent<ComponentType>((*ePointer)[i]) && ...))
			{
				std::apply([&](const auto... pool)
					{
						(myLambda(pool, pool->sparseArray[(*ePointer)[i]], m_bundleStart), ...);
					}, m_pools);

				m_bundleStart++;
			}
		}
	}

	template<typename... ComponentType>
	void BundleImpl<ComponentType...>::Do(std::invocable<ComponentType&...> auto&& func) const noexcept
	{
		for (int i{ m_bundleStart }; i >= 0; --i)
		{
			func(m_mgr->GetComponent<ComponentType>((*ePointer)[i])...);
		}
	}

	template<typename... ComponentType>
	void BundleImpl<ComponentType...>::Do(std::invocable<entity, ComponentType&...> auto&& func) const noexcept
	{
		for (int i{ m_bundleStart }; i >= 0; --i)
		{
			func(i, m_mgr->GetComponent<ComponentType>((*ePointer)[i])...);
		}
	}

	template<typename... ComponentType>
	std::optional<u32> BundleImpl<ComponentType...>::UpdateOnAdd(const entity entityID) noexcept
	{
		const auto&& SwapLambda = []<typename ComponentType>(SparseSet<ComponentType>* pool, u32 index, int bundleStart)
		{
			std::swap(pool->denseArray[index], pool->denseArray[bundleStart + 1]);
			std::swap(pool->components[index], pool->components[bundleStart + 1]);
			std::swap(pool->sparseArray[pool->denseArray[index]], pool->sparseArray[pool->denseArray[bundleStart + 1]]);
		};

		if (m_mgr->HasAllOf<ComponentType...>(entityID))
		{
			std::apply([&](const auto... pool)
				{
					(SwapLambda(pool, pool->sparseArray[entityID], m_bundleStart), ...);
				}, m_pools);

			m_bundleStart++;
			return m_bundleStart;
		}
		return std::nullopt;
	}

	template<typename... ComponentType>
	void BundleImpl<ComponentType...>::UpdateOnRemove(const entity entityID) noexcept
	{
		const auto&& SwapLambda = []<typename ComponentType>(SparseSet<ComponentType>* pool, u32 index, int bundleStart)
		{
			std::swap(pool->denseArray[index], pool->denseArray[bundleStart]);
			std::swap(pool->components[index], pool->components[bundleStart]);
			std::swap(pool->sparseArray[pool->denseArray[index]], pool->sparseArray[pool->denseArray[bundleStart]]);
		};

		if (m_mgr->HasAllOf<ComponentType...>(entityID))
		{
			std::apply([&](const auto... pool)
				{
					(SwapLambda(pool, pool->sparseArray[entityID], m_bundleStart), ...);
				}, m_pools);

			m_bundleStart--;
		}
	}
}