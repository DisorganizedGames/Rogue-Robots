#include "AgentManager.h"
#include "AgentBehaviorSystems.h"

using namespace DOG;
using namespace DirectX::SimpleMath;


/*******************************
		Public Methods
*******************************/

entity AgentManager::CreateAgent(EntityTypes type, const Vector3& pos)
{
	u32 i = static_cast<u32>(type) - static_cast<u32>(EntityTypes::AgentsBegin);
	entity e = CreateAgentCore(m_models[i], pos, type);

	m_entityManager.AddComponent<AgentSeekPlayerComponent>(e);

	// Add CreateAndDestroyEntityComponent to ECS
	return e;
}


void AgentManager::CreateOrDestroyShadowAgent(CreateAndDestroyEntityComponent& entityDesc)
{
	u32 i = static_cast<u32>(entityDesc.entityTypeId) - static_cast<u32>(EntityTypes::AgentsBegin);
	if (entityDesc.alive)
	{
		entity e = CreateAgentCore(m_models[i], entityDesc.position, entityDesc.entityTypeId);

		m_entityManager.AddComponent<ShadowAgentSeekPlayerComponent>(e);
	}
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
	EntityManager::Get().RegisterSystem(std::make_unique<AgentHitDetectionSystem>());
	EntityManager::Get().RegisterSystem(std::make_unique<AgentHitSystem>());
	EntityManager::Get().RegisterSystem(std::make_unique<AgentAggroSystem>());
	EntityManager::Get().RegisterSystem(std::make_unique<AgentFrostTimerSystem>());
	EntityManager::Get().RegisterSystem(std::make_unique<AgentDestructSystem>());

	// Register shadow agent systems
	EntityManager::Get().RegisterSystem(std::make_unique<ShadowAgentSeekPlayerSystem>());
}


/*******************************
		Private Methods
*******************************/

entity AgentManager::CreateAgentCore(u32 model, const Vector3& pos, EntityTypes type)
{
	entity e = m_entityManager.CreateEntity();

	// Set default components
	TransformComponent& trans = m_entityManager.AddComponent<TransformComponent>(e);
	trans.SetPosition(pos);

	m_entityManager.AddComponent<ModelComponent>(e, model);

	m_entityManager.AddComponent<CapsuleColliderComponent>(e, e, 0.5f, 0.5f, true, 50.0f);
	
	RigidbodyComponent& rb = m_entityManager.AddComponent<RigidbodyComponent>(e, e);
	rb.ConstrainRotation(true, true, true);
	rb.disableDeactivation = true;
	rb.getControlOfTransform = true;
	
	AgentIdComponent& agent = m_entityManager.AddComponent<AgentIdComponent>(e);
	agent.id = m_agentIdCounter++;
	agent.type = type;

	AgentMovementComponent& move = m_entityManager.AddComponent<AgentMovementComponent>(e);
	move.station = pos + GenerateRandomVector3(agent.id);
	move.forward = move.station - pos;
	move.forward.Normalize();
	
	m_entityManager.AddComponent<AgentPathfinderComponent>(e);

	m_entityManager.AddComponent<AgentHPComponent>(e);

	//LuaMain::GetScriptManager()->AddScript(e, "AgentHit.lua");

	// Add networking components
	if (m_useNetworking)
	{
		m_entityManager.AddComponent<NetworkTransform>(e).objectId = agent.id;
	}

	if (!m_entityManager.HasComponent<ShadowReceiverComponent>(e))
		m_entityManager.AddComponent<ShadowReceiverComponent>(e);

	return e;
}

Vector3 AgentManager::GenerateRandomVector3(u32 seed, f32 max, f32 min)
{
	//static std::random_device rdev;
	//static std::mt19937 gen(rdev());
	static std::mt19937 gen(seed);
	static std::uniform_real_distribution<f32> udis(min, max);
	return Vector3(udis(gen), udis(gen), udis(gen));
}

