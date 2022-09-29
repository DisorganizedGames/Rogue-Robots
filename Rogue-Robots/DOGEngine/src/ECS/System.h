#pragma once
#include "EntityManager.h"
namespace DOG
{
	template<typename... ComponentType>
	class ISystemBase
	{
	public:
		ISystemBase() noexcept
			: m_mgr{ EntityManager::Get() }
		{

		}
		virtual ~ISystemBase() noexcept = default;
		void Update()
		{
			std::vector<entity>* ePointer = &(get<0>(m_pools)->denseArray);
			std::apply([&ePointer](const auto&... pool)
				{
					((ePointer = (pool->denseArray.size() < ePointer->size()) ? &pool->denseArray : ePointer), ...);
				}, m_pools);

			for (int i{ (int)ePointer->size() - 1 }; i >= 0; --i)
			{
				if (m_mgr.HasAllOf<ComponentType...>((*ePointer)[i]))
				{
					OnUpdate(m_mgr.GetComponent<ComponentType>((*ePointer)[i])...);
				}
			}
		}

		virtual void OnCreate() {}
		virtual void OnAwake() {}
		virtual void OnEarlyUpdate(ComponentType& ...) {}
		virtual void OnUpdate(ComponentType& ...){}
		virtual void OnLateUpdate(ComponentType& ...) {}
		virtual void OnAsleep() {}
	private:
		std::tuple<SparseSet<ComponentType>* ...> m_pools;
		DELETE_COPY_MOVE_CONSTRUCTOR(ISystemBase);
		EntityManager& m_mgr;
	};

	class TestSystem : public ISystemBase<TransformComponent>
	{
	public:
		TestSystem() noexcept = default;
		virtual ~TestSystem() noexcept override final = default;
		virtual void OnUpdate(TransformComponent& tc) override final 
		{
			
		}
	private:

	};
}