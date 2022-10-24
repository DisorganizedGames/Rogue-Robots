#include "AgentManager.h"
#include "AgentBehaviorSystems.h"

using namespace DOG;


/*******************************
		Public Methods
*******************************/

entity AgentManager::CreateAgent(/*entityTypeId*/u32 type, Vector3 pos, SceneType scene) noexcept
{
	entity e = CreateAgentCore(m_models[type], pos, scene);
	// Add CreateAndDestroyEntityComponent to ECS
	return e;
}


void AgentManager::CreateOrDestroyShadowAgent(/*CreateAndDestroyEntityComponent& entityDesc*/)
{
	// if (entityDesc.alive)
	CreateAgentCore(m_models[0], Vector3(0, 0, 0), SceneType::MainScene);
	// else destroy
}


AgentManager::AgentManager() : m_entityManager(EntityManager::Get()), m_agentIdCounter(0)
{
	// Load (all) agent model asset(s)
	m_models.push_back(AssetManager::Get().LoadModelAsset("Assets/Models/Temporary_Assets/enemy.glb"));

	// Register agent systems
	EntityManager::Get().RegisterSystem(std::make_unique<AgentSeekPlayerSystem>());
}



/*******************************
		Private Methods
*******************************/

entity AgentManager::CreateAgentCore(u32 model, Vector3 pos, SceneType scene)
{
	entity e = m_entityManager.CreateEntity();

	// Set default components
	if (!m_entityManager.HasComponent<TransformComponent>(e))
		m_entityManager.AddComponent<TransformComponent>(e);

	if (!m_entityManager.HasComponent<ModelComponent>(e))
		m_entityManager.AddComponent<ModelComponent>(e, model);

	if (!m_entityManager.HasComponent<BoxColliderComponent>(e))
		m_entityManager.AddComponent<BoxColliderComponent>(e, e, Vector3{ 1, 1, 1 }, true);
	
	if (!m_entityManager.HasComponent<RigidbodyComponent>(e))
		m_entityManager.AddComponent<RigidbodyComponent>(e, e, true);
	
	if (!m_entityManager.HasComponent<AgentIdComponent>(e))
	{
		AgentIdComponent& id = m_entityManager.AddComponent<AgentIdComponent>(e);
		id.id = m_agentIdCounter++;
		id.inScene = scene;
	}

	if (!m_entityManager.HasComponent<AgentSeekPlayerComponent>(e))
		m_entityManager.AddComponent<AgentSeekPlayerComponent>(e);
	
	// Add networking components
	if (m_useNetworking)
	{
		if (!m_entityManager.HasComponent<NetworkTransform>(e))
			m_entityManager.AddComponent<NetworkTransform>(e).objectId = e;
	}

	return e;
}
