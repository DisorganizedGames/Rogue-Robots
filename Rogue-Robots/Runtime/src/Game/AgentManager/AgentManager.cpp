#include "AgentManager.h"
#include "AgentBehaviorSystems.h"
#include "Game/GameLayer.h"
#include "../LoadSplitModels.h"
#include "../ItemManager/ItemManager.h"
#include "../PlayerManager/PlayerManager.h"

using namespace DOG;
using namespace DirectX::SimpleMath;


AgentManager AgentManager::s_amInstance;
bool AgentManager::s_notInitialized = true;

/*******************************
		Public Methods
*******************************/


entity AgentManager::CreateAgent(EntityTypes type, u32 groupID, const Vector3& pos, SceneComponent::Type scene)
{
	entity e = CreateAgentCore(GetModel(type), groupID, pos, type);
	EntityManager& em = EntityManager::Get();

	em.AddComponent<SceneComponent>(e, scene);
	em.AddComponent<AgentSeekPlayerComponent>(e);
	em.AddComponent<NetworkAgentStats>(e);

	// Add CreateAndDestroyEntityComponent to ECS
	if (GameLayer::GetNetworkStatus() == NetworkStatus::Hosting)
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
					trans.SetPosition(entityDesc.position);
					DestroyLocalAgent(e, false);
				}
			}
		);
		
	}
}

u32 AgentManager::GenAgentID(u32 groupID)
{
	u32 agentID = (m_agentIdCounter[groupID] << GROUP_BITS) | groupID;
	++m_agentIdCounter[groupID];
	return agentID;
}

void AgentManager::CountAgentKilled(u32 agentID)
{
	u32 i = GroupID(agentID);
	++m_agentKillCounter[i];

	if (m_agentKillCounter[i] == m_agentIdCounter[i])
		m_agentKillCounter[i] = m_agentIdCounter[i] = 0;	// reset both counters
}

u32 AgentManager::GroupID(u32 agentID)
{
	if (agentID == NULL_AGENT)
	{
		// find first empty group
		for (u32 i = 0; i < m_agentIdCounter.size(); ++i)
		{
			if (m_agentIdCounter[i] == 0)
			{
				ASSERT(i < m_agentIdCounter.size(), "found no empty agent group");
				return i;
			}
		}
	}

	// return embedded groupID
	return agentID & MASK;
}

AgentManager::AgentStats AgentManager::GetAgentStats(EntityTypes type)
{
	ASSERT(type < EntityTypes::Agents, "Invalid agent type!");

	switch (type)
	{
	case EntityTypes::Scorpio:
	{
		AgentStats scorpio{};
		scorpio.visionDistance = 8.0;
		scorpio.visionConeDotValue = 0.35f;
		scorpio.lidarDistance = 3.0f;
		scorpio.baseSpeed = 10.0f;
		return scorpio;
	}
		break;
	default:
		break;
	}

	return {};
}

/*******************************
		Private Methods
*******************************/

AgentManager::AgentManager() noexcept
{

}


void AgentManager::Initialize()
{
	// Load (all) agent model asset(s)
	s_amInstance.m_models.push_back(AssetManager::Get().LoadModelAsset("Assets/Models/Enemies/enemy1.gltf"));

	// Register early agent systems
	EntityManager& em = EntityManager::Get();
	em.RegisterSystem(std::make_unique<AgentBehaviorTreeSystem>());
	
	em.RegisterSystem(std::make_unique<AgentDistanceToPlayersSystem>());
	em.RegisterSystem(std::make_unique<AgentLineOfSightToPlayerSystem>());
	em.RegisterSystem(std::make_unique<AgentDetectHitSystem>());
	em.RegisterSystem(std::make_unique<AgentDetectPlayerSystem>());
	
	em.RegisterSystem(std::make_unique<AgentGetPathSystem>());

	// Register agent systems
	em.RegisterSystem(std::make_unique<AgentHitDetectionSystem>());
	em.RegisterSystem(std::make_unique<AgentAggroSystem>());
	em.RegisterSystem(std::make_unique<AgentAttackSystem>());
	
	em.RegisterSystem(std::make_unique<AgentHitSystem>());

	em.RegisterSystem(std::make_unique<AgentFrostTimerSystem>());
	em.RegisterSystem(std::make_unique<AgentFireTimerSystem>());
	em.RegisterSystem(std::make_unique<AgentDestructSystem>());

	// Register shadow agent systems
	em.RegisterSystem(std::make_unique<ShadowAgentSeekPlayerSystem>());

	// Register late update agent systems
	em.RegisterSystem(std::make_unique<AgentMovementSystem>());
	em.RegisterSystem(std::make_unique<LateAgentDestructCleanupSystem>());

	// Set status to initialized
	s_notInitialized = false;
}

entity AgentManager::CreateAgentCore(u32 model, u32 groupID, const Vector3& pos, EntityTypes type)
{
	EntityManager& em = EntityManager::Get();
	entity e = em.CreateEntity();

	// Set default components
	TransformComponent& trans = em.AddComponent<TransformComponent>(e);
	trans.SetPosition(pos);

	em.AddComponent<ModelComponent>(e, model);

	em.AddComponent<CapsuleColliderComponent>(e, e, 0.4f, 0.25f, true, 50.0f);
	
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
				move.currentSpeed = GetAgentStats(type).baseSpeed;
				move.station = pos + GenerateRandomVector3(agent.id);
				move.forward = move.station - pos;
				move.forward.Normalize();
			}
		});

	em.AddComponent<AgentHPComponent>(e);

	em.AddComponent<AgentTargetMetricsComponent>(e);

	// Add networking components
	if (GameLayer::GetNetworkStatus() != NetworkStatus::Offline)
	{
		em.AddComponent<NetworkTransform>(e).objectId = agent.id;
	}

	// Should this component exist on ALL agents or is it only related to networking?
	if (!em.HasComponent<ShadowReceiverComponent>(e))
		em.AddComponent<ShadowReceiverComponent>(e);

	switch (type)
	{
	case EntityTypes::Scorpio:
		CreateScorpioBehaviourTree(e);
		break;
	default:
		break;
	}

	return e;
}

u32 AgentManager::GetModel(EntityTypes type)
{
	return m_models[static_cast<u32>(type) - static_cast<u32>(EntityTypes::AgentsBegin)];
}

u32 AgentManager::GetModel(CreateAndDestroyEntityComponent& entityDesc)
{
	return GetModel(entityDesc.entityTypeId);
}

Vector3 AgentManager::GenerateRandomVector3(u32 seed, f32 max, f32 min)
{
	static std::mt19937 gen(seed);
	static std::uniform_real_distribution<f32> udis(min, max);
	return Vector3(udis(gen), udis(gen), udis(gen));
}

void AgentManager::DestroyLocalAgent(entity e, bool local)
{
	EntityManager& em = EntityManager::Get();

	AgentIdComponent& agent = em.GetComponent<AgentIdComponent>(e);
	TransformComponent& agentTrans = em.GetComponent<TransformComponent>(e);
	SceneComponent::Type scene = em.GetComponent<SceneComponent>(e).scene;

	LoadEnemySplitModel(e, scene);

	entity corpse = em.CreateEntity();
	em.AddComponent<AgentCorpse>(corpse);
	em.AddComponent<SceneComponent>(corpse, scene);

	if (local)
	{
		CreateAndDestroyEntityComponent& kill = em.AddComponent<CreateAndDestroyEntityComponent>(corpse);
		kill.alive = false;
		kill.entityTypeId = agent.type;
		kill.id = agent.id;
		em.Collect<ThisPlayer, NetworkPlayerComponent, InputController>().Do(
			[&](ThisPlayer&, NetworkPlayerComponent& net, InputController& inputC)
			{ 
				kill.playerId = net.playerId; 
				inputC.killScore++;

			});
		kill.position = agentTrans.GetPosition();

		//Only host can spawn in items
		if (kill.playerId == 0)
		{

			if ((u32)Time::ElapsedTime() % 5 == 0)
			{
				//can spawn non ammo
				ItemManager::Get().CreateItemHost(EntityTypes(((u32)Time::ElapsedTime() + agent.id) % u32(EntityTypes::Default)), agentTrans.GetPosition());
			}
			else if ((u32)Time::ElapsedTime() % (1 + MAX_PLAYER_COUNT - PlayerManager::Get().GetNrOfPlayers()) == 0)
			{
				ItemManager::Get().CreateItemHost(EntityTypes(((u32)Time::ElapsedTime() + agent.id) % (u32(EntityTypes::Barrels) - u32(EntityTypes::BarrelItemsBegin) + 1) + (u32)EntityTypes::BarrelItemsBegin), 
					agentTrans.GetPosition());
			}
				
		}
	}


	em.DeferredEntityDestruction(e);

	if (EntityManager::Get().HasComponent<FireEffectComponent>(e))
	{
		EntityManager::Get().DeferredEntityDestruction(EntityManager::Get().GetComponent<FireEffectComponent>(e).particleEntity);
	}
}

void AgentManager::CreateScorpioBehaviourTree(DOG::entity agent) noexcept
{
	std::shared_ptr<Selector> rootSelector = std::move(std::make_shared<Selector>("RootSelector"));
	std::shared_ptr<Selector> attackOrMoveToPlayerSelector = std::move(std::make_shared<Selector>("attackOrMoveToPlayerSelector"));
	attackOrMoveToPlayerSelector->AddChild(std::make_shared<AttackNode>("AttackNode"));
	attackOrMoveToPlayerSelector->AddChild(std::make_shared<MoveToPlayerNode>("MoveToPlayerNode"));

	std::shared_ptr<Sequence> seekAndDestroySequence = std::move(std::make_shared<Sequence>("SeekAndDestroySequence"));
	std::shared_ptr<Succeeder> seekAndDestroySucceeder = std::move(std::make_shared<Succeeder>("SeekAndDestroySucceeder"));
	seekAndDestroySucceeder->AddChild(std::make_shared<SignalGroupNode>("SignalGroupNode"));

	std::shared_ptr<Selector> detectHitOrPlayerSelector = std::move(std::make_shared<Selector>("detectHitOrPlayerSelector"));
	std::shared_ptr<Sequence> detectPlayerSequence = std::move(std::make_shared<Sequence>("DetectPlayerSequence"));

	std::shared_ptr<Selector> detectOrAlertSelector = std::move(std::make_shared<Selector>("DetectOrAlertSelector"));
	std::shared_ptr<Succeeder> lineOfSightToPlayerSucceeder = std::move(std::make_shared<Succeeder>("LineOfSightToPlayerSucceeder"));

	detectOrAlertSelector->AddChild(std::make_shared<DistanceToPlayerNode>("DistanceToPlayerNode"));
	detectOrAlertSelector->AddChild(std::make_shared<IsAlertNode>("IsAlertNode"));
	lineOfSightToPlayerSucceeder->AddChild(std::make_shared<LineOfSightToPlayerNode>("LineOfSightToPlayerNode"));
	detectPlayerSequence->AddChild(std::move(detectOrAlertSelector));
	detectPlayerSequence->AddChild(std::move(lineOfSightToPlayerSucceeder));


	detectHitOrPlayerSelector->AddChild(std::make_shared<DetectHitNode>("DetectHitNode"));
	detectHitOrPlayerSelector->AddChild(std::make_shared<DetectPlayerNode>("DetectPlayerNode"));
	detectPlayerSequence->AddChild(std::move(detectHitOrPlayerSelector));

	rootSelector->AddChild(seekAndDestroySequence);
	seekAndDestroySequence->AddChild(std::move(detectPlayerSequence));
	seekAndDestroySequence->AddChild(std::make_shared<GetPathNode>("GetPathNode"));
	seekAndDestroySequence->AddChild(std::move(seekAndDestroySucceeder));

	seekAndDestroySequence->AddChild(std::move(attackOrMoveToPlayerSelector));

	auto& btc = DOG::EntityManager::Get().AddComponent<BehaviorTreeComponent>(agent);
	btc.rootNode = std::move(std::make_unique<Root>("ScorpioRootNode"));
	btc.rootNode->AddChild(std::move(rootSelector));
	btc.currentRunningNode = btc.rootNode.get();

	//BehaviorTree::ToGraphViz(btc.rootNode.get(), "BehaviorTree_Scorpio.dot");
}