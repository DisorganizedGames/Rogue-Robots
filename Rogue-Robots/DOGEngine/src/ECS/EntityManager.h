#pragma once
#include "Component.h"
#include <StaticTypeInfo/type_id.h>
#include <StaticTypeInfo/type_index.h>
#include <StaticTypeInfo/type_name.h>
#define set(ID) (static_cast<SparseSet<ComponentType>*>(m_components.at(ID).get()))

namespace DOG
{
	#define sti static_type_info
	
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

	//##################### SPARSE SET #####################

	struct SparseSetBase
	{
		SparseSetBase() noexcept = default;
		virtual ~SparseSetBase() noexcept = default;
		std::vector<entity> sparseArray;
		std::vector<entity> denseArray;
		sti::TypeIndex bundle = nullptr;

		virtual void DestroyInternal(const entity entityID) noexcept = 0;
	};

	template<typename ComponentType>
	struct SparseSet : public SparseSetBase
	{
		SparseSet() noexcept = default;
		virtual ~SparseSet() noexcept override final = default;
		std::vector<ComponentType> components;
		
	private:
		virtual void DestroyInternal(const entity entityID) noexcept override final;
	};

	template<typename ComponentType>
	void SparseSet<ComponentType>::DestroyInternal(const entity entityID) noexcept
	{
		std::swap(components.back(), components[sparseArray[entityID]]);
		components.pop_back();
	}

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
		ComponentType& AddComponent(const entity entityID, Args&& ...args) noexcept;

		template<typename ComponentType>
		void RemoveComponent(const entity entityID) noexcept;

		template<typename ComponentType>
		[[nodiscard]] ComponentType& GetComponent(const entity entityID) const noexcept;

		template<typename... ComponentType>
		[[nodiscard]] Collection<ComponentType...> Collect() noexcept;

		template<typename ComponentType>
		[[nodiscard]] bool HasComponent(const entity entityID) const noexcept;

		template<typename... ComponentType>
		[[nodiscard]] bool HasAllOf(const entity entityID) const noexcept;

		template<typename... ComponentType>
		[[nodiscard]] bool HasAnyOf(const entity entityID) const noexcept;

		template<typename... ComponentType>
		[[nodiscard]] BundleImpl<ComponentType...>& Bundle() noexcept;

	private:
		EntityManager() noexcept;
		~EntityManager() noexcept = default;
		DELETE_COPY_MOVE_CONSTRUCTOR(EntityManager);

		void Initialize() noexcept;

		[[nodiscard]] bool HasComponentInternal(const sti::TypeIndex componentPoolIndex, const entity entityID) const noexcept;
		void DestroyComponentInternal(const sti::TypeIndex componentPoolIndex, const entity entityID) noexcept;

		template<typename ComponentType>
		void ValidateComponentPool() noexcept;

		template<typename ComponentType> 
		void AddSparseSet() noexcept;
	public:
		template<typename ComponentType>
		SparseSet<ComponentType>* ExpandAsTupleArguments() const noexcept;
	private:
		template<typename... ComponentType>
		friend class Collection;
		template<typename... ComponentType>
		friend class BundleImpl;
		
		friend class ISystem;

		static EntityManager s_instance;
		std::vector<entity> m_entities;
		std::queue<entity> m_freeList;
		std::unordered_map<sti::TypeIndex ,ComponentPool> m_components;
		std::unordered_map<sti::TypeIndex, std::unique_ptr<BundleBase>> m_bundles;
	};

	template<typename ComponentType>
	SparseSet<ComponentType>* EntityManager::ExpandAsTupleArguments() const noexcept
	{
		constexpr auto componentID = sti::getTypeIndex<ComponentType>();

		return static_cast<SparseSet<ComponentType>*>(m_components.at(componentID).get());
	}

	template<typename... ComponentType>
	BundleImpl<ComponentType...>& EntityManager::Bundle() noexcept
	{
		constexpr std::array<sti::TypeIndex, sizeof... (ComponentType)> componentIDs{ sti::getTypeIndex<ComponentType>()... };
		constexpr sti::TypeIndex minComponentID = *std::min_element(std::begin(componentIDs), std::end(componentIDs));

		if (m_bundles[minComponentID] == nullptr)
		{
			std::tuple<SparseSet<ComponentType>*...> pools = { (ExpandAsTupleArguments<ComponentType>()) ...};
#if defined _DEBUG
			std::apply([](const auto&... pool) {(ASSERT((pool != nullptr) && pool->bundle == nullptr, "Bundle creation failed."), ...); }, pools);
#endif
			std::apply([&minComponentID](const auto... pool) {((pool->bundle = minComponentID), ...); }, pools);
			m_bundles[minComponentID] = (std::unique_ptr<BundleImpl<ComponentType...>>(new BundleImpl<ComponentType...>(this, std::move(pools))));
		}
		return *(static_cast<BundleImpl<ComponentType...>*>(m_bundles[minComponentID].get()));
	}

	template<typename ComponentType, typename ...Args>
	ComponentType& EntityManager::AddComponent(const entity entityID, Args&& ...args) noexcept
	{
		constexpr auto ComponentID = sti::getTypeIndex<ComponentType>();

		ASSERT(Exists(entityID), "Entity is invalid");
		ASSERT(!HasComponent<ComponentType>(entityID), "Entity already has component!");

		ValidateComponentPool<ComponentType>();

		const size_t position = set(ComponentID)->denseArray.size();
		set(ComponentID)->denseArray.emplace_back(entityID);
		set(ComponentID)->components.emplace_back(ComponentType(std::forward<Args>(args)...));
		set(ComponentID)->sparseArray[entityID] = static_cast<entity>(position);

		if (set(ComponentID)->bundle != nullptr)
		{
			auto componentIdx = m_bundles[set(ComponentID)->bundle]->UpdateOnAdd(entityID);
			if (componentIdx)
			{
				return set(ComponentID)->components[*componentIdx];
			}
		}
		return set(ComponentID)->components.back();
	}

	template<typename ComponentType>
	void EntityManager::RemoveComponent(const entity entityID) noexcept
	{
		auto constexpr componentID = sti::getTypeIndex<ComponentType>();

		ASSERT(Exists(entityID), "Entity is invalid");
		ASSERT(HasComponent<ComponentType>(entityID), "Entity does not have that component.");

		if (set(componentID)->bundle != nullptr)
		{
			m_bundles[set(componentID)->bundle]->UpdateOnRemove(entityID);
		}

		const auto last = set(componentID)->denseArray.back();
		std::swap(set(componentID)->denseArray.back(), set(componentID)->denseArray[set(componentID)->sparseArray[entityID]]);
		std::swap(set(componentID)->components.back(), set(componentID)->components[set(componentID)->sparseArray[entityID]]);
		std::swap(set(componentID)->sparseArray[last], set(componentID)->sparseArray[entityID]);
		set(componentID)->denseArray.pop_back();
		set(componentID)->components.pop_back();
		set(componentID)->sparseArray[entityID] = NULL_ENTITY;
	}

	template<typename ComponentType>
	ComponentType& EntityManager::GetComponent(const entity entityID) const noexcept
	{
		auto constexpr componentID = sti::getTypeIndex<ComponentType>();
		ASSERT(Exists(entityID), "Entity is invalid");
		ASSERT(HasComponent<ComponentType>(entityID), "Entity does not have that component.");

		return set(componentID)->components[set(componentID)->sparseArray[entityID]];
	}

	template<typename... ComponentType>
	Collection<ComponentType...> EntityManager::Collect() noexcept
	{
		return Collection<ComponentType...>( this, { ExpandAsTupleArguments<ComponentType>() ...});
	}

	template<typename ComponentType> 
	bool EntityManager::HasComponent(const entity entityID) const noexcept
	{
		auto constexpr componentID = sti::getTypeIndex<ComponentType>();
		ASSERT(Exists(entityID), "Entity is invalid");

		return 
			(
			m_components.contains(componentID)
			&& entityID < set(componentID)->sparseArray.size())
			&& (set(componentID)->sparseArray[entityID] < set(componentID)->denseArray.size())
			&& (set(componentID)->sparseArray[entityID] != NULL_ENTITY
			);
	}

	template<typename... ComponentType>
	bool EntityManager::HasAllOf(const entity entityID) const noexcept
	{
		ASSERT(Exists(entityID), "Entity is invalid");

		return (EntityManager::HasComponent<ComponentType>(entityID) && ...);
	}

	template<typename... ComponentType>
	bool EntityManager::HasAnyOf(const entity entityID) const noexcept
	{
		ASSERT(Exists(entityID), "Entity is invalid");

		return (EntityManager::HasComponent<ComponentType>(entityID) || ...);
	}

	template<typename ComponentType>
	void EntityManager::ValidateComponentPool() noexcept
	{
		constexpr auto ComponentID = sti::getTypeIndex<ComponentType>();
		if (!m_components.contains(ComponentID))
		{
			AddSparseSet<ComponentType>();
		}
	}

	template<typename ComponentType> 
	void EntityManager::AddSparseSet() noexcept
	{
		constexpr auto ComponentID = sti::getTypeIndex<ComponentType>();
		m_components[ComponentID] = std::move(std::make_unique<SparseSet<ComponentType>>());

		set(ComponentID)->sparseArray.reserve(MAX_ENTITIES);
		for (u32 i{ 0u }; i < MAX_ENTITIES; i++)
		{
			set(ComponentID)->sparseArray.push_back(NULL_ENTITY);
		}

		set(ComponentID)->denseArray.reserve(MAX_ENTITIES);
		set(ComponentID)->components.reserve(MAX_ENTITIES);
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

	class ISystem
	{
	public:
		ISystem() noexcept = default;
		virtual ~ISystem() noexcept = default;
		virtual void Create() noexcept {}
		virtual void Update() noexcept {}
	};
	
	template<typename... ComponentType>
	class SystemHelper
	{
	public:
		SystemHelper() noexcept
			: m_pools{ EntityManager::Get().ExpandAsTupleArguments<ComponentType>() ...} {}
	
	std::tuple<SparseSet<ComponentType>* ...> m_pools;
	};
#ifndef SYSTEM_CLASS
	#define SYSTEM_CLASS(...)	\
	SystemHelper<__VA_ARGS__> systemHelper;	
#endif

#ifndef ON_UPDATE
	#define ON_UPDATE(...)	\
	void Update() noexcept override final	\
	{	\
		UpdateImpl<__VA_ARGS__>();	\
	}	\
	\
	template<typename... ComponentType>	\
	void UpdateImpl()	\
	{	\
		std::vector<entity>* ePointer = &(get<0>(systemHelper.m_pools)->denseArray);	\
		std::apply([&ePointer](const auto&... pool)	\
		{	\
			((ePointer = (pool->denseArray.size() < ePointer->size()) ? &pool->denseArray : ePointer), ...);	\
		}, systemHelper.m_pools);	\
	\
	for (int i{ (int)ePointer->size() - 1 }; i >= 0; --i)	\
	{	\
		if (EntityManager::Get().HasAllOf<ComponentType...>((*ePointer)[i]))	\
		{	\
			OnUpdate((*ePointer)[i], EntityManager::Get().GetComponent<ComponentType>((*ePointer)[i]) ...);	\
		}	\
	}	\
	}
#endif

	//################## ON CREATE ####################

#ifndef ON_CREATE
#define ON_CREATE(...)	\
	void Create() noexcept override final	\
	{	\
		CreateImpl<__VA_ARGS__>();	\
	}	\
	\
	template<typename... ComponentType>	\
	void CreateImpl()	\
	{	\
		std::vector<entity>* ePointer = &(get<0>(systemHelper.m_pools)->denseArray);	\
		std::apply([&ePointer](const auto&... pool)	\
		{	\
			((ePointer = (pool->denseArray.size() < ePointer->size()) ? &pool->denseArray : ePointer), ...);	\
		}, systemHelper.m_pools);	\
	\
	for (int i{ (int)ePointer->size() - 1 }; i >= 0; --i)	\
	{	\
		if (EntityManager::Get().HasAllOf<ComponentType...>((*ePointer)[i]))	\
		{	\
			OnCreate((*ePointer)[i], EntityManager::Get().GetComponent<ComponentType>((*ePointer)[i]) ...);	\
		}	\
	}	\
	}
#endif

	class TestSystem : public ISystem
	{
	public:
		SYSTEM_CLASS(TransformComponent, ModelComponent);
		ON_UPDATE(TransformComponent);
		ON_CREATE();
		
		void OnUpdate(entity entityID, TransformComponent& tc)
		{
			std::cout << tc.GetPosition().x << "\n";
		}

		void OnCreate(entity entityID)
		{
			std::cout << entityID << "\n";
		}
	};
}