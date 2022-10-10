#pragma once
#include "Component.h"
#include "System.h"
#include <StaticTypeInfo/type_id.h>
#include <StaticTypeInfo/type_index.h>
#include <StaticTypeInfo/type_name.h>
#define set(ID) (static_cast<SparseSet<ComponentType>*>(m_components.at(ID).get()))

namespace DOG
{
#if defined _DEBUG
	#ifndef ECS_DEBUG_OP
		#define ECS_DEBUG_OP(lambda) lambda();
	#endif
	#ifndef ECS_DEBUG_EXPR
		#define ECS_DEBUG_EXPR(expr) expr
	#endif
#else
	#ifndef ECS_DEBUG_OP
		#define ECS_DEBUG_OP(lambda) 
	#endif
	#ifndef ECS_DEBUG_EXPR
		#define ECS_DEBUG_EXPR(expr)
	#endif
#endif

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
		virtual void DestroyInternal(const entity entityID) noexcept = 0;

#if defined _DEBUG
		virtual std::pair<std::vector<entity>&, std::string_view> ReportUsage() noexcept = 0;
#endif
		std::vector<entity> sparseArray;
		std::vector<entity> denseArray;
		sti::TypeIndex bundle = nullptr;
	};

	template<typename ComponentType>
	struct SparseSet : public SparseSetBase
	{
		SparseSet() noexcept = default;
		virtual ~SparseSet() noexcept override final = default;

#if defined _DEBUG
		virtual std::pair<std::vector<entity>&, std::string_view> ReportUsage() noexcept override final
		{
			constexpr std::string_view s = sti::getTypeName<ComponentType>();
			constexpr std::string_view finalString = s.substr(s.find_first_not_of("struct DOG::"));
			return { denseArray, finalString };
		}
#endif
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
		[[nodiscard]] u32 GetNrOfEntities() const noexcept { return MAX_ENTITIES - (u32)m_freeList.size(); }
		void Reset() noexcept;
		[[nodiscard]] bool Exists(const entity entityID) const noexcept;
		[[nodiscard]] const std::unordered_map<sti::TypeIndex, ComponentPool>& GetAllComponentPools() const noexcept { return m_components; }

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

		void RegisterSystem(std::unique_ptr<ISystem>&& pSystem) noexcept;

		[[nodiscard]] constexpr const std::vector<std::unique_ptr<ISystem>>::const_iterator begin() const { return m_systems.begin(); }
		[[nodiscard]] constexpr const std::vector<std::unique_ptr<ISystem>>::const_iterator end() const { return m_systems.end(); }
	
		ECS_DEBUG_EXPR([[nodiscard]] std::vector<std::unique_ptr<ISystem>>& GetAllSystems() noexcept { return m_systems; });
		ECS_DEBUG_EXPR([[nodiscard]] constexpr const std::vector<entity>& GetAllEntitiesAlive() const noexcept { return m_aliveEntities; });

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
		SparseSet<ComponentType>* ExpandAsTupleArguments() noexcept;
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
		std::vector<std::unique_ptr<ISystem>> m_systems;
	
		ECS_DEBUG_EXPR(std::vector<entity> m_aliveEntities;);
	};

	template<typename ComponentType>
	SparseSet<ComponentType>* EntityManager::ExpandAsTupleArguments() noexcept
	{
		constexpr auto componentID = sti::getTypeIndex<ComponentType>();
		ValidateComponentPool<ComponentType>();
		
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

			ECS_DEBUG_EXPR(std::apply([](const auto&... pool) {(ASSERT((pool != nullptr) && pool->bundle == nullptr, "Bundle creation failed."), ...); }, pools););
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
		ECS_DEBUG_EXPR(std::apply([](const auto... pool) {(ASSERT(pool, "Component type does not exist."), ...); }, m_pools););
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
		friend ISystem;
	};

	template<typename... ComponentType>
	class BundleImpl : public BundleBase
	{
	public:
		explicit BundleImpl(EntityManager* mgr, const std::tuple<SparseSet<ComponentType>*...>& pools) noexcept;
		virtual ~BundleImpl() noexcept override final = default;

		void Do(std::invocable<ComponentType&...> auto&& func) const noexcept;
		void Do(std::invocable<entity, ComponentType&...> auto&& func) const noexcept;

		[[nodiscard]] constexpr const std::vector<entity>* GetEntityVectorPointer() const noexcept { return ePointer; }
		[[nodiscard]] constexpr const int* GetBundleStartPointer() const noexcept { return &m_bundleStart; }
	private:
		DELETE_COPY_MOVE_CONSTRUCTOR(BundleImpl);
		[[nodiscard]] std::optional<u32> UpdateOnAdd(const entity entityID) noexcept override final;
		virtual void UpdateOnRemove(const entity entityID) noexcept override final;
	private:
		friend EntityManager;
		friend ISystem;
		EntityManager* m_mgr;
		std::tuple<SparseSet<ComponentType>*...> m_pools;
		std::vector<entity>* ePointer;
		int m_bundleStart;
	};

	template<typename... ComponentType>
	BundleImpl<ComponentType...>::BundleImpl(EntityManager* mgr, const std::tuple<SparseSet<ComponentType>*...>& pools) noexcept
		: m_mgr{ mgr }, m_pools{ std::move(pools) }, m_bundleStart{ -1 }, ePointer{ nullptr }
	{
		ECS_DEBUG_EXPR(std::apply([](const auto... pool) {(ASSERT(pool, "Component type does not exist."), ...); }, m_pools);)
		
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

//##################### SYSTEMS #####################

	template<typename... ComponentType>
	struct SystemHelper
	{
		SystemHelper() noexcept
			: m_mgr{ DOG::EntityManager::Get() }, m_pools{ DOG::EntityManager::Get().ExpandAsTupleArguments<ComponentType>() ... } {}

		inline [[nodiscard]] std::vector<DOG::entity>* GetMinimumEntityVector() const
		{
			std::vector<DOG::entity>* ePointer = &(get<0>(m_pools)->denseArray);
			std::apply([&ePointer](const auto&... pool)
				{
					((ePointer = (pool->denseArray.size() < ePointer->size()) ? &pool->denseArray : ePointer), ...);
				}, m_pools);

			return ePointer;
		}
		DELETE_COPY_MOVE_CONSTRUCTOR(SystemHelper);
		DOG::EntityManager& m_mgr;
		std::tuple<DOG::SparseSet<ComponentType>* ...> m_pools;
	};

	template<typename... ComponentType>
	struct CriticalSystemHelper
	{
		CriticalSystemHelper() noexcept
			: m_mgr{ DOG::EntityManager::Get() } 
		{
			static_assert(sizeof...(ComponentType) > 1);
			auto& bundle = DOG::EntityManager::Get().Bundle<ComponentType...>();
			m_ePointer = bundle.GetEntityVectorPointer();
			m_bundleStart = bundle.GetBundleStartPointer();
		}
		DELETE_COPY_MOVE_CONSTRUCTOR(CriticalSystemHelper);
		DOG::EntityManager& m_mgr;
		const std::vector<DOG::entity>* m_ePointer;
		const int* m_bundleStart;
	};

#if defined _DEBUG																													
#define SYSTEM_CLASS(...)																								\
public:																															\
		SystemHelper<__VA_ARGS__> m_systemHelper;																				\
		[[nodiscard]] virtual std::string_view GetName() const override															\
		{																														\
			constexpr std::string_view s = __FUNCTION__;																		\
			constexpr std::string_view s2 = s.substr(s.find_first_not_of("DOG::"));												\
			constexpr size_t lastIndex = s2.find_first_of(":");																	\
			constexpr std::string_view finalName = s2.substr(0, lastIndex);														\
			return finalName;																									\
		}																														\
		[[nodiscard]] virtual DOG::SystemType GetType() const noexcept															\
		{																														\
			return DOG::SystemType::Standard;																						\
		}																														
#else																															
#define SYSTEM_CLASS(...)																									\
	SystemHelper<__VA_ARGS__> m_systemHelper;
#endif																															


#if defined _DEBUG																													
#define SYSTEM_CLASS_CRITICAL(...)																								\
		CriticalSystemHelper<__VA_ARGS__> m_systemHelper;																		\
		[[nodiscard]] virtual std::string_view GetName() const override															\
		{																														\
			constexpr std::string_view s = __FUNCTION__;																		\
			constexpr std::string_view s2 = s.substr(s.find_first_not_of("DOG::"));												\
			constexpr size_t lastIndex = s2.find_first_of(":");																	\
			constexpr std::string_view finalName = s2.substr(0, lastIndex);														\
			return finalName;																									\
		}																														\
		[[nodiscard]] virtual DOG::SystemType GetType() const noexcept															\
		{																														\
			return DOG::SystemType::Critical;																						\
		}
#else																															
#define SYSTEM_CLASS_CRITICAL(...)																								\
	CriticalSystemHelper<__VA_ARGS__> m_systemHelper;
#endif

	/*ON_CREATE*/

#ifndef ON_CREATE
#define ON_CREATE(...)																										\
	void Create() noexcept override final																					\
	{																														\
		CreateImpl<__VA_ARGS__>();																							\
	}																														\
																															\
	template<typename... ComponentType>																						\
	void CreateImpl()																										\
	{																														\
		auto ePointer = m_systemHelper.GetMinimumEntityVector();															\
		for (int i{ (int)ePointer->size() - 1 }; i >= 0; --i)																\
		{																													\
			if (m_systemHelper.m_mgr.HasAllOf<ComponentType...>((*ePointer)[i]))											\
			{																												\
				OnCreate(m_systemHelper.m_mgr.GetComponent<ComponentType>((*ePointer)[i]) ...);								\
			}																												\
		}																													\
	}
#endif

#ifndef ON_CREATE_ID
#define ON_CREATE_ID(...)																									\
	void Create() noexcept override final																					\
	{																														\																										\
			CreateImpl<__VA_ARGS__>();																						\
	}																														\
																															\
	template<typename... ComponentType>																						\
	void CreateImpl()																										\
	{																														\
		auto ePointer = m_systemHelper.GetMinimumEntityVector();															\
		for (int i{ (int)ePointer->size() - 1 }; i >= 0; --i)																\
		{																													\
			if (m_systemHelper.m_mgr.HasAllOf<ComponentType...>((*ePointer)[i]))											\
			{																												\
				OnCreate((*ePointer)[i], m_systemHelper.m_mgr.GetComponent<ComponentType>((*ePointer)[i]) ...);				\
			}																												\
		}																													\
	}
#endif

#ifndef ON_CREATE_CRITICAL
#define ON_CREATE_CRITICAL(...)																								\
	void Create() noexcept override final																					\
	{																														\
		CreateImpl<__VA_ARGS__>();																							\
	}																														\
																															\
	template<typename... ComponentType>																						\
	void CreateImpl()																										\
	{																														\
		for (int i{*m_systemHelper.m_bundleStart }; i >= 0; --i)															\
		{																													\
			OnCreate(m_systemHelper.m_mgr.GetComponent<ComponentType>((*m_systemHelper.m_ePointer)[i])...);					\
		}																													\
	}
#endif

#ifndef ON_CREATE_CRITICAL_ID
#define ON_CREATE_CRITICAL_ID(...)																							\
	void Create() noexcept override final																					\
	{																														\
		CreateImpl<__VA_ARGS__>();																							\
	}																														\
																															\
	template<typename... ComponentType>																						\
	void CreateImpl()																										\
	{																														\
		for (int i{*m_systemHelper.m_bundleStart }; i >= 0; --i)															\
		{																													\
			OnCreate(i, m_systemHelper.m_mgr.GetComponent<ComponentType>((*m_systemHelper.m_ePointer)[i])...);					\
		}																													\
	}
#endif

	/*ON_EARLY_UPDATE*/

#ifndef ON_EARLY_UPDATE
#define ON_EARLY_UPDATE(...)																								\
	void EarlyUpdate() noexcept override final																				\
	{																														\
		EarlyUpdateImpl<__VA_ARGS__>();																						\
	}																														\
																															\
	template<typename... ComponentType>																						\
	void EarlyUpdateImpl()																									\
	{																														\
		auto ePointer = m_systemHelper.GetMinimumEntityVector();															\
		for (int i{ (int)ePointer->size() - 1 }; i >= 0; --i)																\
		{																													\
			if (m_systemHelper.m_mgr.HasAllOf<ComponentType...>((*ePointer)[i]))											\
			{																												\
				OnEarlyUpdate(m_systemHelper.m_mgr.GetComponent<ComponentType>((*ePointer)[i]) ...);						\
			}																												\
		}																													\
	}
#endif

#ifndef ON_EARLY_UPDATE_ID
#define ON_EARLY_UPDATE_ID(...)																								\
	void EarlyUpdate() noexcept override final																				\
	{																														\
		EarlyUpdateImpl<__VA_ARGS__>();																						\
	}																														\
																															\
	template<typename... ComponentType>																						\
	void EarlyUpdateImpl()																									\
	{																														\
		auto ePointer = m_systemHelper.GetMinimumEntityVector();															\
		for (int i{ (int)ePointer->size() - 1 }; i >= 0; --i)																\
		{																													\
			if (m_systemHelper.m_mgr.HasAllOf<ComponentType...>((*ePointer)[i]))											\
			{																												\
				OnEarlyUpdate((*ePointer)[i], m_systemHelper.m_mgr.GetComponent<ComponentType>((*ePointer)[i]) ...);		\
			}																												\
		}																													\
	}
#endif

#ifndef ON_EARLY_UPDATE_CRITICAL
#define ON_EARLY_UPDATE_CRITICAL(...)																						\
	void EarlyUpdate() noexcept override final																				\
	{																														\
		EarlyUpdateImpl<__VA_ARGS__>();																						\
	}																														\
																															\
	template<typename... ComponentType>																						\
	void EarlyUpdateImpl()																									\
	{																														\
		for (int i{*m_systemHelper.m_bundleStart }; i >= 0; --i)															\
		{																													\
			OnEarlyUpdate(m_systemHelper.m_mgr.GetComponent<ComponentType>((*m_systemHelper.m_ePointer)[i])...);			\
		}																													\
	}
#endif

#ifndef ON_EARLY_UPDATE_CRITICAL_ID
#define ON_EARLY_UPDATE_CRITICAL_ID(...)																					\
	void EarlyUpdate() noexcept override final																				\
	{																														\
		EarlyUpdateImpl<__VA_ARGS__>();																						\
	}																														\
																															\
	template<typename... ComponentType>																						\
	void EarlyUpdateImpl()																									\
	{																														\
		for (int i{*m_systemHelper.m_bundleStart }; i >= 0; --i)															\
		{																													\
			OnEarlyUpdate(i, m_systemHelper.m_mgr.GetComponent<ComponentType>((*m_systemHelper.m_ePointer)[i])...);			\
		}																													\
	}
#endif

	/*ON_UPDATE*/

#ifndef ON_UPDATE
#define ON_UPDATE(...)																										\
	void Update() noexcept override final																					\
	{																														\
		UpdateImpl<__VA_ARGS__>();																							\
	}																														\
																															\
	template<typename... ComponentType>																						\
	void UpdateImpl()																										\
	{																														\
		auto ePointer = m_systemHelper.GetMinimumEntityVector();															\
		for (int i{ (int)ePointer->size() - 1 }; i >= 0; --i)																\
		{																													\
			if (m_systemHelper.m_mgr.HasAllOf<ComponentType...>((*ePointer)[i]))											\
			{																												\
				OnUpdate(m_systemHelper.m_mgr.GetComponent<ComponentType>((*ePointer)[i]) ...);								\
			}																												\
		}																													\
	}
#endif

#ifndef ON_UPDATE_ID
#define ON_UPDATE_ID(...)																									\
	void Update() noexcept override final																					\
	{																														\
		UpdateImpl<__VA_ARGS__>();																							\
	}																														\
																															\
	template<typename... ComponentType>																						\
	void UpdateImpl()																										\
	{																														\
		auto ePointer = m_systemHelper.GetMinimumEntityVector();															\
		for (int i{ (int)ePointer->size() - 1 }; i >= 0; --i)																\
		{																													\
			if (m_systemHelper.m_mgr.HasAllOf<ComponentType...>((*ePointer)[i]))											\
			{																												\
				OnUpdate((*ePointer)[i], m_systemHelper.m_mgr.GetComponent<ComponentType>((*ePointer)[i]) ...);				\
			}																												\
		}																													\
	}
#endif

#ifndef ON_UPDATE_CRITICAL
#define ON_UPDATE_CRITICAL(...)																								\
	void Update() noexcept override final																					\
	{																														\
		UpdateImpl<__VA_ARGS__>();																							\
	}																														\
																															\
	template<typename... ComponentType>																						\
	void UpdateImpl()																										\
	{																														\
		for (int i{*m_systemHelper.m_bundleStart }; i >= 0; --i)															\
		{																													\
			OnUpdate(m_systemHelper.m_mgr.GetComponent<ComponentType>((*m_systemHelper.m_ePointer)[i])...);					\
		}																													\
	}
#endif

#ifndef ON_UPDATE_CRITICAL_ID
#define ON_UPDATE_CRITICAL_ID(...)																							\
	void Update() noexcept override final																					\
	{																														\
		UpdateImpl<__VA_ARGS__>();																							\
	}																														\
																															\
	template<typename... ComponentType>																						\
	void UpdateImpl()																										\
	{																														\
		for (int i{*m_systemHelper.m_bundleStart }; i >= 0; --i)															\
		{																													\
			OnUpdate(i, m_systemHelper.m_mgr.GetComponent<ComponentType>((*m_systemHelper.m_ePointer)[i])...);				\
		}																													\
	}
#endif

	/*ON_LATE_UPDATE*/

#ifndef ON_LATE_UPDATE
#define ON_LATE_UPDATE(...)																									\
	void LateUpdate() noexcept override final																				\
	{																														\
		LateUpdateImpl<__VA_ARGS__>();																						\
	}																														\
																															\
	template<typename... ComponentType>																						\
	void LateUpdateImpl()																									\
	{																														\
		auto ePointer = m_systemHelper.GetMinimumEntityVector();															\
		for (int i{ (int)ePointer->size() - 1 }; i >= 0; --i)																\
		{																													\
			if (m_systemHelper.m_mgr.HasAllOf<ComponentType...>((*ePointer)[i]))											\
			{																												\
				OnLateUpdate(m_systemHelper.m_mgr.GetComponent<ComponentType>((*ePointer)[i]) ...);							\
			}																												\
		}																													\
	}
#endif

#ifndef ON_LATE_UPDATE_ID
#define ON_LATE_UPDATE_ID(...)																								\
	void LateUpdate() noexcept override final																				\
	{																														\
		LateUpdateImpl<__VA_ARGS__>();																						\
	}																														\
																															\
	template<typename... ComponentType>																						\
	void LateUpdateImpl()																									\
	{																														\
		auto ePointer = m_systemHelper.GetMinimumEntityVector();															\
		for (int i{ (int)ePointer->size() - 1 }; i >= 0; --i)																\
		{																													\
			if (m_systemHelper.m_mgr.HasAllOf<ComponentType...>((*ePointer)[i]))											\
			{																												\
				OnLateUpdate((*ePointer)[i], m_systemHelper.m_mgr.GetComponent<ComponentType>((*ePointer)[i]) ...);			\
			}																												\
		}																													\
	}
#endif

#ifndef ON_LATE_UPDATE_CRITICAL
#define ON_LATE_UPDATE_CRITICAL(...)																						\
	void LateUpdate() noexcept override final																				\
	{																														\
		LateUpdateImpl<__VA_ARGS__>();																						\
	}																														\
																															\
	template<typename... ComponentType>																						\
	void LateUpdateImpl()																									\
	{																														\
		for (int i{*m_systemHelper.m_bundleStart }; i >= 0; --i)															\
		{																													\
			OnLateUpdate(m_systemHelper.m_mgr.GetComponent<ComponentType>((*m_systemHelper.m_ePointer)[i])...);				\
		}																													\
	}
#endif

#ifndef ON_LATE_UPDATE_CRITICAL_ID
#define ON_LATE_UPDATE_CRITICAL_ID(...)																						\
	void LateUpdate() noexcept override final																				\
	{																														\
		LateUpdateImpl<__VA_ARGS__>();																						\
	}																														\
																															\
	template<typename... ComponentType>																						\
	void LateUpdateImpl()																									\
	{																														\
		for (int i{*m_systemHelper.m_bundleStart }; i >= 0; --i)															\
		{																													\
			OnLateUpdate(i, m_systemHelper.m_mgr.GetComponent<ComponentType>((*m_systemHelper.m_ePointer)[i])...);			\
		}																													\
	}
#endif
