#pragma once
#include <DOGEngine.h>
#include "GameComponent.h"
#include "Network/Network.h"

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

	void AddEntity(DOG::entity e) const noexcept;
	void AddEntities(const std::vector<DOG::entity>& entities) const noexcept;

	SceneComponent::Type GetSceneType() const noexcept;


	void CreateTrampolinePickup(DirectX::SimpleMath::Vector3 position);
	void CreateMissilePickup(DirectX::SimpleMath::Vector3 position);
	void CreateGrenadePickup(DirectX::SimpleMath::Vector3 position);
	void CreateMaxHealthBoostPickup(DirectX::SimpleMath::Vector3 position);
	void CreateFrostModificationPickup(DirectX::SimpleMath::Vector3 position);
protected:
	SceneComponent::Type m_sceneType;
	static DOG::EntityManager& s_entityManager;
};
