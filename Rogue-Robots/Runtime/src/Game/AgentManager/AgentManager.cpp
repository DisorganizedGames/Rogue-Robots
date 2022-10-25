#include "AgentManager.h"
#include "AgentBehaviorSystems.h"

using namespace DOG;


/*******************************
		Public Methods
*******************************/

entity AgentManager::CreateAgent(EntityTypes type, const Vector3& pos)
{
	u32 i = static_cast<u32>(type) - static_cast<u32>(EntityTypes::AgentsBegin); // RangeCastEntityTypes(EntityTypes::AgentsBegin, type);
	//std::cout << sizeof(EntityTypes) << " " << i << " " << "(" << pos.x << "," << pos.y << "," << pos.z << ")" << std::endl;
	entity e = CreateAgentCore(m_models[i], pos);
	// Add CreateAndDestroyEntityComponent to ECS
	return e;
}


void AgentManager::CreateOrDestroyShadowAgent(CreateAndDestroyEntityComponent& entityDesc)
{
	 if (entityDesc.alive)
		CreateAgentCore(m_models[0], Vector3(0, 0, 0));
	// else destroy
}


AgentManager::AgentManager() : m_entityManager(EntityManager::Get()), m_agentIdCounter(0)
{
	// Load (all) agent model asset(s)
	m_models.push_back(AssetManager::Get().LoadModelAsset("Assets/Models/Temporary_Assets/suzanne.glb"));
	//m_models.push_back(AssetManager::Get().LoadModelAsset("Assets/Models/Enemies/enemy.gltf"));

	// Register agent systems
	EntityManager::Get().RegisterSystem(std::make_unique<AgentSeekPlayerSystem>());
}



/*******************************
		Private Methods
*******************************/

entity AgentManager::CreateAgentCore(u32 model, const Vector3& pos)
{
	entity e = m_entityManager.CreateEntity();

	// Set default components
	if (!m_entityManager.HasComponent<TransformComponent>(e))
	{
		TransformComponent& trans = m_entityManager.AddComponent<TransformComponent>(e);
		trans.SetPosition(pos);
	}

	if (!m_entityManager.HasComponent<ModelComponent>(e))
		m_entityManager.AddComponent<ModelComponent>(e, model);

	if (!m_entityManager.HasComponent<BoxColliderComponent>(e))
		m_entityManager.AddComponent<BoxColliderComponent>(e, e, Vector3{ 1, 1, 1 }, true);
	
	if (!m_entityManager.HasComponent<RigidbodyComponent>(e))
	{
		RigidbodyComponent& rb = m_entityManager.AddComponent<RigidbodyComponent>(e, e);
		rb.ConstrainRotation(true, true, true);
		rb.disableDeactivation = true;
		rb.getControlOfTransform = false;
	}
	
	if (!m_entityManager.HasComponent<AgentIdComponent>(e))
	{
		AgentIdComponent& id = m_entityManager.AddComponent<AgentIdComponent>(e);
		id.id = m_agentIdCounter++;
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
