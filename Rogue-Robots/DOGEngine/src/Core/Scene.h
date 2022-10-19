#pragma once
#include "../ECS/EntityManager.h"

namespace DOG
{
	enum class SceneType
	{
		Global = 0,
		MainScene,
		TestScene,
	};
	struct SceneComponent
	{
		SceneComponent(SceneType scene) : scene(scene) {}
		SceneType scene;
	};

	class Scene
	{
	public:
		Scene(SceneType scene);
		Scene(const Scene& other) = delete;
		Scene& operator=(const Scene& other) = delete;
		virtual ~Scene();

		virtual void SetUpScene() = 0;
		[[nodiscard]] entity CreateEntity() const noexcept;

		template<typename ComponentType, typename ...Args>
		ComponentType& AddComponent(const entity entityID, Args&& ...args) noexcept
		{
			return s_entityManager.AddComponent<ComponentType, Args...>(entityID, std::forward<Args>(args)...);
		}

		SceneType GetSceneType() const noexcept;
	protected:
		SceneType m_sceneType;
		static EntityManager& s_entityManager;
	};
}
