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
	m_entityManager.AddComponent<NetworkAgentStats>(e);
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
		m_entityManager.Collect<AgentIdComponent, TransformComponent>().Do(
			[&](entity e, AgentIdComponent& agent, TransformComponent& trans)
			{
				if (agent.id == entityDesc.id)
				{
					DestroyLocalAgent(e);
					trans.SetPosition(entityDesc.position);
				}
			}
		);
		
	}
}


AgentManager::AgentManager() : m_entityManager(EntityManager::Get()), m_agentIdCounter(0)
{
#ifdef _DEBUG
	// Some unit tests
	u32 agentID_1 = GenAgentID(0);
	bool test_1 = agentID_1 == 1;
	
	u32 agentID_2 = GenAgentID(1);
	bool test_2 = agentID_2 == (1 << GROUP_BITS);
	bool test_3 = m_agentIdCounter == GROUP_SIZE + 1;
	
	u32 agentID_3 = GenAgentID(4);
	bool test_4 = agentID_3 == (1 << (GROUP_BITS * 4));
	bool test_5 = m_agentIdCounter == GROUP_SIZE * 4 + GROUP_SIZE + 1;

	u32 agentID_1 = GenAgentID(0);
	bool test_6 = agentID_1 == 2;
	bool test_7 = m_agentIdCounter == GROUP_SIZE * 4 + GROUP_SIZE + 3;

	bool all_tests = test_1 && test_2 && test_3 && test_4 && test_5 && test_6 && test_7;
	assert(all_tests);
	m_agentIdCounter = 0;
#endif // _DEBUG


	// Load (all) agent model asset(s)
	m_models.push_back(AssetManager::Get().LoadModelAsset("Assets/Models/Enemies/enemy1.gltf"));

	// Register agent systems
	m_entityManager.RegisterSystem(std::make_unique<AgentSeekPlayerSystem>());
	m_entityManager.RegisterSystem(std::make_unique<AgentMovementSystem>());
	m_entityManager.RegisterSystem(std::make_unique<AgentAttackSystem>());
	m_entityManager.RegisterSystem(std::make_unique<AgentHitDetectionSystem>());
	m_entityManager.RegisterSystem(std::make_unique<AgentHitSystem>());
	m_entityManager.RegisterSystem(std::make_unique<AgentAggroSystem>());
	m_entityManager.RegisterSystem(std::make_unique<AgentFrostTimerSystem>());
	m_entityManager.RegisterSystem(std::make_unique<AgentDestructSystem>());

	// Register shadow agent systems
	m_entityManager.RegisterSystem(std::make_unique<ShadowAgentSeekPlayerSystem>());
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

	m_entityManager.AddComponent<CapsuleColliderComponent>(e, e, 0.25f, 0.25f, true, 50.0f);
	
	RigidbodyComponent& rb = m_entityManager.AddComponent<RigidbodyComponent>(e, e);
	rb.ConstrainRotation(true, true, true);
	rb.disableDeactivation = true;
	rb.getControlOfTransform = true;
	
	AgentIdComponent& agent = m_entityManager.AddComponent<AgentIdComponent>(e);
	agent.id = m_agentIdCounter++;
	agent.type = type;

	m_entityManager.Collect<ThisPlayer, NetworkPlayerComponent>().Do(
		[&](ThisPlayer&, NetworkPlayerComponent& player)
		{
			if (player.playerId == 0)
			{
				AgentMovementComponent& move = m_entityManager.AddComponent<AgentMovementComponent>(e);
				move.station = pos + GenerateRandomVector3(agent.id);
				move.forward = move.station - pos;
				move.forward.Normalize();
			}
		});
	m_entityManager.AddComponent<AgentPathfinderComponent>(e);

	m_entityManager.AddComponent<AgentHPComponent>(e);

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
	return m_models[static_cast<u32>(type) - static_cast<u32>(EntityTypes::AgentsBegin)];
}

u32 AgentManager::GetModel(CreateAndDestroyEntityComponent& entityDesc)
{
	return m_models[static_cast<u32>(entityDesc.entityTypeId) - static_cast<u32>(EntityTypes::AgentsBegin)];
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

u32 AgentManager::GenAgentID(u32 groupID)
{
	u32 shift = groupID * GROUP_BITS;
	u32 mask = MASK << shift;
	u32 agentID = m_agentIdCounter & mask;
	if (agentID == 0)	// special case
		++agentID;
	u32 groupCount = (agentID >> shift) + 1;

#ifdef _DEBUG
	if (GROUP_SIZE <= groupCount)
	{
		std::cout << "Agent group " << groupID << " overflow error" << std::endl;
		assert(false);
	}
#endif // _DEBUG
	
	m_agentIdCounter = ((m_agentIdCounter & (~mask)) & (groupCount << shift));
	return agentID;
}

void AgentManager::CountAgentKilled(u32 agentID)
{
	u32 shift = GroupID(agentID) * GROUP_BITS;					// bit shift for group
	u32 mask = MASK << shift;
	u32 groupCount = m_agentIdCounter & mask;
	u32 killCount = m_agentKillCounter & mask;
	killCount = ((killCount >> shift) + 1) << shift;			// increment kill count
	m_agentKillCounter = m_agentKillCounter & (~mask);			// set kill counter to 0
	if (groupCount == killCount)
		m_agentIdCounter = m_agentIdCounter & (~mask);			// set group counter to 0
	else
		m_agentKillCounter = m_agentKillCounter & killCount;	// add kill count to group
}

u32 AgentManager::GroupID(u32 agentID)
{
	constexpr u32 GROUPS = sizeof(m_agentIdCounter) / GROUP_BITS;
	u32 mask = MASK;
	u32 i = 0;
	if (agentID == 0)
		// find first empty group
		for (; i < GROUPS; ++i)
		{
			if ((m_agentIdCounter & mask) == 0)
				break;
			else
				mask = mask << GROUP_BITS;
		}
	else
		// find group of agentID
		for (; i < GROUPS; ++i)
		{
			if ((m_agentIdCounter & mask) == 0)
				mask = mask << GROUP_BITS;
			else
				break;
		}
	return i;
}