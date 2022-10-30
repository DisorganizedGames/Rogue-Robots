#include "AgentManager.h"
#include "AgentBehaviorSystems.h"

using namespace DOG;
using namespace DirectX::SimpleMath;


/*******************************
		Public Methods
*******************************/

entity AgentManager::CreateAgent(EntityTypes type, const Vector3& pos)
{
	entity e = CreateAgentCore(GetModel(type), pos, type);

	m_entityManager.AddComponent<AgentSeekPlayerComponent>(e);

	// Add CreateAndDestroyEntityComponent to ECS
	if (m_useNetworking)
	{
		AgentIdComponent& agent = m_entityManager.GetComponent<AgentIdComponent>(e);
		TransformComponent& agentTrans = m_entityManager.GetComponent<TransformComponent>(e);
		
		CreateAndDestroyEntityComponent& create = m_entityManager.AddComponent<CreateAndDestroyEntityComponent>(e);
		create.alive = true;
		create.entityTypeId = agent.type;
		create.id = agent.id;
		create.position = agentTrans.GetPosition();
		m_entityManager.Collect<ThisPlayer, NetworkPlayerComponent>().Do(
			[&](ThisPlayer&, NetworkPlayerComponent& net) { create.playerId = net.playerId; });
	}
	return e;
}


void AgentManager::CreateOrDestroyShadowAgent(CreateAndDestroyEntityComponent& entityDesc)
{
	if (entityDesc.alive)
	{
		entity e = CreateAgentCore(GetModel(entityDesc), entityDesc.position, entityDesc.entityTypeId);

		m_entityManager.AddComponent<ShadowAgentSeekPlayerComponent>(e);
	}
	else
	{
		entity toDestroy = DOG::NULL_ENTITY;
		m_entityManager.Collect<AgentIdComponent, TransformComponent>().Do(
			[&](entity e, AgentIdComponent& agent, TransformComponent& trans)
			{
				if (agent.id == entityDesc.id)
				{
					toDestroy = e;
					trans.SetPosition(entityDesc.position);
				}
			}
		);
		DestroyLocalAgent(toDestroy);
	}
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

	m_entityManager.AddComponent<CapsuleColliderComponent>(e, e, 1.f, 1.f, true, 50.0f);
	
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

u32 AgentManager::GetModel(EntityTypes type)
{
	return m_models[0];	// temporary hack
	//return m_models[static_cast<u32>(type) - static_cast<u32>(EntityTypes::AgentsBegin)];
}

u32 AgentManager::GetModel(CreateAndDestroyEntityComponent& entityDesc)
{
	return m_models[0];	// temporary hack
	//return m_models[static_cast<u32>(entityDesc.entityTypeId) - static_cast<u32>(EntityTypes::AgentsBegin)];
}

Vector3 AgentManager::GenerateRandomVector3(u32 seed, f32 max, f32 min)
{
	//static std::random_device rdev;
	//static std::mt19937 gen(rdev());
	static std::mt19937 gen(seed);
	static std::uniform_real_distribution<f32> udis(min, max);
	return Vector3(udis(gen), udis(gen), udis(gen));
}

void AgentManager::DestroyLocalAgent(entity e)
{
	EntityManager& em = EntityManager::Get();

	AgentIdComponent& agent = em.GetComponent<AgentIdComponent>(e);
	TransformComponent& agentTrans = em.GetComponent<TransformComponent>(e);
	ModelComponent& agentModel = em.GetComponent<ModelComponent>(e);

	entity corpse = em.CreateEntity();
	em.AddComponent<AgentCorpse>(corpse);
	em.AddComponent<ModelComponent>(corpse, agentModel.id);
	TransformComponent& corpseTrans = em.AddComponent<TransformComponent>(corpse);
	corpseTrans = agentTrans;
	corpseTrans.SetRotation(Vector3(-2, 0, -2));

	CreateAndDestroyEntityComponent& kill = em.AddComponent<CreateAndDestroyEntityComponent>(corpse);
	kill.alive = false;
	kill.entityTypeId = agent.type;
	kill.id = agent.id;
	em.Collect<ThisPlayer, NetworkPlayerComponent>().Do(
		[&](ThisPlayer&, NetworkPlayerComponent& net) { kill.playerId = net.playerId; });
	kill.position = agentTrans.GetPosition();

	em.DeferredEntityDestruction(e);
}

