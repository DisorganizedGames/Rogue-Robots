#include "AgentManager.h"
#include "AgentBehaviorSystems.h"
#include "Game/GameLayer.h"
#include "../LoadSplitModels.h"
#include "../ItemManager/ItemManager.h"
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

	// Register agent systems
	EntityManager& em = EntityManager::Get();
	em.RegisterSystem(std::make_unique<AgentBehaviorTreeSystem>());
	em.RegisterSystem(std::make_unique<AgentDetectPlayerSystem>());
	em.RegisterSystem(std::make_unique<AgentAggroSystem>());
	em.RegisterSystem(std::make_unique<AgentAttackSystem>());

	em.RegisterSystem(std::make_unique<AgentMovementSystem>()); //Is LATE System
	em.RegisterSystem(std::make_unique<AgentHitDetectionSystem>());
	em.RegisterSystem(std::make_unique<AgentHitSystem>());
	em.RegisterSystem(std::make_unique<AgentFrostTimerSystem>());
	em.RegisterSystem(std::make_unique<AgentFireTimerSystem>());
	em.RegisterSystem(std::make_unique<AgentDestructSystem>());

	// Register shadow agent systems
	em.RegisterSystem(std::make_unique<ShadowAgentSeekPlayerSystem>());

	// Register late update agent systems
	em.RegisterSystem(std::make_unique<LateAgentDestructCleanupSystem>());

	//entity hero = em.CreateEntity();
	//em.AddComponent<HeroComponent>(hero);
	//em.AddComponent<TransformComponent>(hero).SetPosition({ 14.0f, 0.0f, 0.0f });
	//
	//CreateVillain({ 15.0f, 0.0f, 0.0f });
	//CreateVillain({ 17.0f, 0.0f, 0.0f });
	//CreateVillain({ 20.0f, 0.0f, 0.0f });
	//CreateVillain({ 25.0f, 0.0f, 0.0f });
	//
	////InitialBehaviourTreeSystem:
	//EntityManager::Get().Collect<VillainComponent, BehaviorTreeComponent>().Do([](entity villain, VillainComponent&, BehaviorTreeComponent& btc)
	//	{
	//		btc.currentRunningNode->Process(villain);
	//	});
	//
	//while (true)
	//{
	//	//DetectPlayerSystem:
	//	EntityManager::Get().Collect<VillainComponent, BTDetectPlayerComponent, TransformComponent>().Do([&](entity villain, VillainComponent&, BTDetectPlayerComponent&, TransformComponent& vtc)
	//		{
	//			bool detectedHero = false;
	//			EntityManager::Get().Collect<HeroComponent, TransformComponent>().Do([&](HeroComponent&, TransformComponent& htc)
	//				{
	//					bool isWithinNineMetresOfPlayer = Vector3::Distance(htc.GetPosition(), vtc.GetPosition()) <= 10.0f;
	//					if (isWithinNineMetresOfPlayer)
	//					{
	//						detectedHero = true;
	//					}
	//				});
	//
	//			auto& btc = EntityManager::Get().GetComponent<BehaviorTreeComponent>(villain);
	//			if (detectedHero)
	//				LEAF(btc.currentRunningNode)->Succeed(villain);
	//			else
	//				LEAF(btc.currentRunningNode)->Fail(villain);
	//		});
	//
	//	//SignalGroupSystem:
	//	EntityManager::Get().Collect<VillainComponent, BTSignalGroupComponent, TransformComponent>().Do([&](entity villain, VillainComponent&, BTSignalGroupComponent&, TransformComponent& tc)
	//		{
	//			bool signaledAnyAgentInGroup = false;
	//			EntityManager::Get().Collect<VillainComponent, TransformComponent>().Do([&](entity otherVillain, VillainComponent&, TransformComponent& otc)
	//				{
	//					if (villain == otherVillain)
	//						return;
	//
	//					//We must not signal an agent that already has the SignalGroupComponent (Remember BT setups...):
	//					if (EntityManager::Get().HasComponent<BTSignalGroupComponent>(otherVillain))
	//						return;
	//
	//					const float distanceBetweenAgents = Vector3::Distance(tc.GetPosition(), otc.GetPosition());
	//					bool agentIsInRangeOfOtherAgent = (distanceBetweenAgents <= 5.0f);
	//					if (agentIsInRangeOfOtherAgent)
	//					{
	//
	//						if (!EntityManager::Get().HasComponent<BTAttackComponent>(otherVillain))
	//						{
	//							signaledAnyAgentInGroup = true;
	//							EntityManager::Get().AddComponent<BTAttackComponent>(otherVillain);
	//							auto& btc = EntityManager::Get().GetComponent<BehaviorTreeComponent>(otherVillain);
	//							LEAF(btc.currentRunningNode)->Fail(otherVillain);
	//						}
	//					}
	//				});
	//
	//			auto& btc = EntityManager::Get().GetComponent<BehaviorTreeComponent>(villain);
	//			if (signaledAnyAgentInGroup)
	//				LEAF(btc.currentRunningNode)->Succeed(villain);
	//			else
	//				LEAF(btc.currentRunningNode)->Fail(villain);
	//		});
	//
	//	//AttackSystem:
	//	EntityManager::Get().Collect<VillainComponent, BTAttackComponent, TransformComponent>().Do([&](entity villain, VillainComponent&, BTAttackComponent&, TransformComponent& tc)
	//		{
	//			EntityManager::Get().Collect<HeroComponent, TransformComponent>().Do([&](HeroComponent&, TransformComponent& htc)
	//				{
	//					//(We only have 1 hero...)
	//					const bool agentIsInAttackDistance = (Vector3::Distance(tc.GetPosition(), htc.GetPosition()) <= 1.0f);
	//					auto& btc = EntityManager::Get().GetComponent<BehaviorTreeComponent>(villain);
	//					if (agentIsInAttackDistance)
	//					{
	//						//Success! Deal damage!
	//						//...
	//						LEAF(btc.currentRunningNode)->Succeed(villain);
	//					}
	//					else
	//					{
	//						//Fail! The hero is out of reach!!
	//						LEAF(btc.currentRunningNode)->Fail(villain);
	//					}
	//				});
	//		});
	//
	//	//MoveToPlayerSystem:
	//	EntityManager::Get().Collect<VillainComponent, BTMoveToPlayerComponent, TransformComponent>().Do([&](entity villain, VillainComponent&, BTMoveToPlayerComponent&, TransformComponent& tc)
	//		{
	//			Vector3 heroPosition;
	//			EntityManager::Get().Collect<HeroComponent, TransformComponent>().Do([&heroPosition](HeroComponent&, TransformComponent& htc)
	//				{
	//					//We only have 1 hero:
	//					heroPosition = htc.GetPosition();
	//				});
	//			//They are lined up only on the x-axis:
	//			if (heroPosition.x > tc.GetPosition().x)
	//				tc.SetPosition({ tc.GetPosition().x + 1, tc.GetPosition().y, tc.GetPosition().z });
	//			else
	//				tc.SetPosition({ tc.GetPosition().x - 1, tc.GetPosition().y, tc.GetPosition().z });
	//
	//			//Obviously, if NO PATH can be found to the player/hero, this would fail (even before the calculations above).
	//			//Now we can just assume it always succeeds:
	//			auto& btc = EntityManager::Get().GetComponent<BehaviorTreeComponent>(villain);
	//			LEAF(btc.currentRunningNode)->Succeed(villain);
	//
	//		});
	//
	//	//PatrolSystem:
	//	EntityManager::Get().Collect<VillainComponent, BTPatrolComponent, TransformComponent>().Do([&](entity villain, VillainComponent&, BTPatrolComponent&, TransformComponent& tc)
	//		{
	//			//How would this even fail? If enemy somehow is stuck?
	//			//Trivial example: move X-component of vector, will always walk in +X-direction:
	//			float xOffset = (((float)rand() / (float)RAND_MAX) + 1.0f);
	//			xOffset >= 1.5f ? xOffset = 2.0f : xOffset = 1.0f;
	//			tc.SetPosition({ tc.GetPosition().x + xOffset, tc.GetPosition().y, tc.GetPosition().z });
	//
	//			auto& btc = EntityManager::Get().GetComponent<BehaviorTreeComponent>(villain);
	//			LEAF(btc.currentRunningNode)->Succeed(villain);
	//		});
	//}
	

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

	em.AddComponent<AgentHPComponent>(e);

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
			ItemManager::Get().CreateItemHost(EntityTypes(((u32)Time::ElapsedTime() + agent.id) % u32(EntityTypes::Default)), agentTrans.GetPosition());
	}


	em.DeferredEntityDestruction(e);

	if (EntityManager::Get().HasComponent<FireEffectComponent>(e))
	{
		EntityManager::Get().DeferredEntityDestruction(EntityManager::Get().GetComponent<FireEffectComponent>(e).particleEntity);
	}
}

void AgentManager::CreateVillain(const Vector3& position)
{
	
	DOG::entity villain = EntityManager::Get().CreateEntity();
	EntityManager::Get().AddComponent<VillainComponent>(villain);
	EntityManager::Get().AddComponent<TransformComponent>(villain).SetPosition(position);
	CreateScorpioBehaviourTree(villain);
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

	rootSelector->AddChild(seekAndDestroySequence);
	rootSelector->AddChild(std::make_shared<PatrolNode>("PatrolNode"));
	seekAndDestroySequence->AddChild(std::make_shared<DetectPlayerNode>("DetectPlayerNode"));
	seekAndDestroySequence->AddChild(std::move(seekAndDestroySucceeder));

	seekAndDestroySequence->AddChild(std::move(attackOrMoveToPlayerSelector));

	auto& btc = DOG::EntityManager::Get().AddComponent<BehaviorTreeComponent>(agent);
	btc.rootNode = std::move(std::make_unique<Root>("ScorpioRootNode"));
	btc.rootNode->AddChild(std::move(rootSelector));
	btc.currentRunningNode = btc.rootNode.get();
}