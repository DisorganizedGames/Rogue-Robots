#pragma once
#include <DOGEngine.h>
#include "GameComponent.h"


class Scene
{
public:
	Scene(SceneComponent::Type scene);
	Scene(const Scene& other) = delete;
	Scene& operator=(const Scene& other) = delete;
	virtual ~Scene();

	virtual void SetUpScene(std::vector<std::function<std::vector<DOG::entity>()>> entityCreators = {}) = 0;
	[[nodiscard]] DOG::entity CreateEntity() const noexcept;

	template<typename ComponentType, typename ...Args>
	ComponentType& AddComponent(const DOG::entity entityID, Args&& ...args) noexcept
	{
		return s_entityManager.AddComponent<ComponentType, Args...>(entityID, std::forward<Args>(args)...);
	}

	SceneComponent::Type GetSceneType() const noexcept;
protected:
	SceneComponent::Type m_sceneType;
	static DOG::EntityManager& s_entityManager;
};
