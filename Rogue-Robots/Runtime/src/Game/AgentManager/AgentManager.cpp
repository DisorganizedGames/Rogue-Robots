#include "AgentManager.h"
#include "AgentBehaviorSystems.h"

using namespace DOG;
using namespace DirectX::SimpleMath;


AgentManager AgentManager::s_amInstance;
bool AgentManager::m_notInitialized = true;

/*******************************
		Public Methods
*******************************/


entity AgentManager::CreateAgent(EntityTypes type, u32 groupID, const Vector3& pos)
{
	entity e = CreateAgentCore(GetModel(type), groupID, pos, type);
	EntityManager& em = EntityManager::Get();

	em.AddComponent<AgentSeekPlayerComponent>(e);
	em.AddComponent<NetworkAgentStats>(e);
	// Add CreateAndDestroyEntityComponent to ECS
	if (m_useNetworking)
	{
		AgentIdComponent& agent = em.GetComponent<AgentIdComponent>(e);
		TransformComponent& agentTrans = em.GetComponent<TransformComponent>(e);
		
		CreateAndDestroyEntityComponent& create = em.AddComponent<CreateAndDestroyEntityComponent>(e);
		create.alive = true;
		create.entityTypeId = agent.type;
		create.id = agent.id;
		create.position = agentTrans.GetPosition();
		em.Collect<ThisPlayer, NetworkPlayerComponent>().Do(
			[&](ThisPlayer&, NetworkPlayerComponent& net) { create.playerId = net.playerId; });
	}
	return e;
}


void AgentManager::CreateOrDestroyShadowAgent(CreateAndDestroyEntityComponent& entityDesc)
{
	if (entityDesc.alive)
	{
		entity e = CreateAgentCore(GetModel(entityDesc), GroupID(entityDesc.id), entityDesc.position, entityDesc.entityTypeId);

		EntityManager::Get().AddComponent<ShadowAgentSeekPlayerComponent>(e);
	}
	else
	{
		EntityManager::Get().Collect<AgentIdComponent, TransformComponent>().Do(
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

/*******************************
		Private Methods
*******************************/

AgentManager::AgentManager() noexcept
{
#ifdef _DEBUG
	// Some unit tests
	constexpr u32 group_0 = 1;
	constexpr u32 group_1 = group_0 << GROUP_BITS;
	constexpr u32 group_2 = group_1 << GROUP_BITS;
	constexpr u32 group_3 = group_2 << GROUP_BITS;
	constexpr u32 group_4 = group_3 << GROUP_BITS;

	u32 agentID_1 = GenAgentID(0);
	bool test_1 = agentID_1 == group_0;

	u32 agentID_2 = GenAgentID(1);
	bool test_2 = agentID_2 == group_1;
	u32 count_1 = m_agentIdCounter;
	bool test_3 = count_1 == group_1 + group_0;

	u32 agentID_3 = GenAgentID(4);
	bool test_4 = agentID_3 == group_4;
	u32 count_2 = m_agentIdCounter;
	bool test_5 = count_2 == group_4 + group_1 + group_0;

	u32 agentID_4 = GenAgentID(0);
	bool test_6 = agentID_4 == group_0 * 2;
	u32 count_3 = m_agentIdCounter;
	bool test_7 = count_3 == group_4 + group_1 + group_0 * 2;

	bool all_tests = test_1 && test_2 && test_3 && test_4 && test_5 && test_6 && test_7;
	assert(all_tests);
	m_agentIdCounter = 0;
#endif // _DEBUG
}


void AgentManager::Initialize()
{
	// Load (all) agent model asset(s)
	s_amInstance.m_models.push_back(AssetManager::Get().LoadModelAsset("Assets/Models/Enemies/enemy1.gltf"));

	// Register agent systems
	EntityManager& em = EntityManager::Get();
	em.RegisterSystem(std::make_unique<AgentSeekPlayerSystem>());
	em.RegisterSystem(std::make_unique<AgentMovementSystem>());
	em.RegisterSystem(std::make_unique<AgentAttackSystem>());
	em.RegisterSystem(std::make_unique<AgentHitDetectionSystem>());
	em.RegisterSystem(std::make_unique<AgentHitSystem>());
	em.RegisterSystem(std::make_unique<AgentAggroSystem>());
	em.RegisterSystem(std::make_unique<AgentFrostTimerSystem>());
	em.RegisterSystem(std::make_unique<AgentDestructSystem>());

	// Register shadow agent systems
	em.RegisterSystem(std::make_unique<ShadowAgentSeekPlayerSystem>());

	// Register late update agent systems
	em.RegisterSystem(std::make_unique<LateAgentDestructCleanupSystem>());

	// Set status to initialized
	m_notInitialized = false;
}

entity AgentManager::CreateAgentCore(u32 model, u32 groupID, const Vector3& pos, EntityTypes type)
{
	EntityManager& em = EntityManager::Get();
	entity e = em.CreateEntity();

	// Set default components
	TransformComponent& trans = em.AddComponent<TransformComponent>(e);
	trans.SetPosition(pos);

	em.AddComponent<ModelComponent>(e, model);

	em.AddComponent<CapsuleColliderComponent>(e, e, 0.25f, 0.25f, true, 50.0f);
	
	RigidbodyComponent& rb = em.AddComponent<RigidbodyComponent>(e, e);
	rb.ConstrainRotation(true, true, true);
	rb.disableDeactivation = true;
	rb.getControlOfTransform = true;
	
	AgentIdComponent& agent = em.AddComponent<AgentIdComponent>(e);
	agent.id = GenAgentID(groupID);
	agent.type = type;

	em.Collect<ThisPlayer, NetworkPlayerComponent>().Do(
		[&](ThisPlayer&, NetworkPlayerComponent& player)
		{
			if (player.playerId == 0)
			{
				AgentMovementComponent& move = em.AddComponent<AgentMovementComponent>(e);
				move.station = pos + GenerateRandomVector3(agent.id);
				move.forward = move.station - pos;
				move.forward.Normalize();
			}
		});
	em.AddComponent<AgentPathfinderComponent>(e);

	em.AddComponent<AgentHPComponent>(e);

	// Add networking components
	if (m_useNetworking)
	{
		em.AddComponent<NetworkTransform>(e).objectId = agent.id;
	}

	if (!em.HasComponent<ShadowReceiverComponent>(e))
		em.AddComponent<ShadowReceiverComponent>(e);

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
	u32 groupCount = ((m_agentIdCounter & mask) >> shift) + 1;
	u32 agentID = groupCount << shift;

#ifdef _DEBUG
	if (GROUP_SIZE < groupCount)
	{
		std::cout << "Agent group " << groupID << " overflow error" << std::endl;
		assert(false);
	}
#endif // _DEBUG
	
	m_agentIdCounter = ((m_agentIdCounter & (~mask)) | (groupCount << shift));
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
		m_agentKillCounter = m_agentKillCounter | killCount;	// add kill count to group
}
