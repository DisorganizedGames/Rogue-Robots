#include "AgentManager.h"
#include "AgentBehaviorSystems.h"

using namespace DOG;


/*******************************
		Public Methods
*******************************/

entity AgentManager::CreateAgent(EntityTypes type, const Vector3& pos)
{
	u32 i = static_cast<u32>(type) - static_cast<u32>(EntityTypes::AgentsBegin);
	entity e = CreateAgentCore(m_models[i], pos);
	// Add CreateAndDestroyEntityComponent to ECS
	return e;
}


void AgentManager::CreateOrDestroyShadowAgent(CreateAndDestroyEntityComponent& entityDesc)
{
	u32 i = static_cast<u32>(entityDesc.entityTypeId) - static_cast<u32>(EntityTypes::AgentsBegin);
	if (entityDesc.alive)
		CreateAgentCore(m_models[i], entityDesc.position);
	// else destroy
}


AgentManager::AgentManager() : m_entityManager(EntityManager::Get()), m_agentIdCounter(0)
{
	// Load (all) agent model asset(s)
	m_models.push_back(AssetManager::Get().LoadModelAsset("Assets/Models/Enemies/enemy1.gltf"));

	// Register agent systems
	EntityManager::Get().RegisterSystem(std::make_unique<AgentSeekPlayerSystem>());
	EntityManager::Get().RegisterSystem(std::make_unique<AgentMovementSystem>());
	EntityManager::Get().RegisterSystem(std::make_unique<AgentAttackSystem>());
	EntityManager::Get().RegisterSystem(std::make_unique<AgentHitSystem>());
	EntityManager::Get().RegisterSystem(std::make_unique<AgentFrostTimerSystem>());
	EntityManager::Get().RegisterSystem(std::make_unique<AgentDestructSystem>());
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

	if (!m_entityManager.HasComponent<CapsuleColliderComponent>(e))
		m_entityManager.AddComponent<CapsuleColliderComponent>(e, e, 0.25f, 1.0f, true);
	
	if (!m_entityManager.HasComponent<RigidbodyComponent>(e))
	{
		RigidbodyComponent& rb = m_entityManager.AddComponent<RigidbodyComponent>(e, e);
		rb.ConstrainRotation(true, false, true);
		rb.disableDeactivation = true;
		rb.getControlOfTransform = true;
	}
	
	if (!m_entityManager.HasComponent<AgentIdComponent>(e))
	{
		AgentIdComponent& id = m_entityManager.AddComponent<AgentIdComponent>(e);
		id.id = m_agentIdCounter++;
	}

	if (!m_entityManager.HasComponent<AgentSeekPlayerComponent>(e))
		m_entityManager.AddComponent<AgentSeekPlayerComponent>(e);
	
	if (!m_entityManager.HasComponent<AgentMovementComponent>(e))
		m_entityManager.AddComponent<AgentMovementComponent>(e);
	
	if (!m_entityManager.HasComponent<AgentPathfinderComponent>(e))
		m_entityManager.AddComponent<AgentPathfinderComponent>(e);

	if (!m_entityManager.HasComponent<AgentHPComponent>(e))
		m_entityManager.AddComponent<AgentHPComponent>(e);

	LuaMain::GetScriptManager()->AddScript(e, "AgentHit.lua");

	// Add networking components
	if (m_useNetworking)
	{
		if (!m_entityManager.HasComponent<NetworkTransform>(e))
			m_entityManager.AddComponent<NetworkTransform>(e).objectId = e;
	}

	if (!m_entityManager.HasComponent<ShadowReceiverComponent>(e))
		m_entityManager.AddComponent<ShadowReceiverComponent>(e);

	return e;
}
