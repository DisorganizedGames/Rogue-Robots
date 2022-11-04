#include "Scene.h"

using namespace DOG;

EntityManager& Scene::s_entityManager = EntityManager::Get();

Scene::Scene(SceneComponent::Type scene) : m_sceneType(scene) {}

Scene::~Scene()
{
	s_entityManager.Collect<SceneComponent>().Do([&](entity e, SceneComponent& sceneC)
		{
			if (sceneC.scene == m_sceneType)
			{
				s_entityManager.DeferredEntityDestruction(e);
			}
		});
}

entity Scene::CreateEntity() const noexcept
{
	entity e = s_entityManager.CreateEntity();
	s_entityManager.AddComponent<SceneComponent>(e, m_sceneType);
	return e;
}



void Scene::AddEntity(DOG::entity e) const noexcept
{
	assert(!s_entityManager.HasComponent<SceneComponent>(e));
	s_entityManager.AddComponent<SceneComponent>(e, m_sceneType);
}

void Scene::AddEntities(const std::vector<DOG::entity>& entities) const noexcept
{
	for (auto& e : entities)
		AddEntity(e);
}

SceneComponent::Type Scene::GetSceneType() const noexcept
{
	return m_sceneType;
}