#include "AgentBehaviorSystems.h"
#include "AgentManager.h"
#include "../DOGEngine/src/Graphics/Rendering/PostProcess.h"
#include "Game/PlayerManager/PlayerManager.h"

using namespace DOG;
using namespace DirectX::SimpleMath;

/**************************************************
*			Early Update Systems
***************************************************/

void AgentBehaviorTreeSystem::OnEarlyUpdate(DOG::entity agent, AgentIdComponent&, BehaviorTreeComponent& btc)
{
	ASSERT(btc.currentRunningNode, "Current running node is nullptr!");

	btc.currentRunningNode->Process(agent);
}

void AgentDistanceToPlayersSystem::OnEarlyUpdate(DOG::entity agent, BTDetectPlayerComponent&, AgentTargetMetricsComponent& atmc, 
	DOG::TransformComponent& tc, BehaviorTreeComponent& btc)
{
	//System checks the (non-squared) distance to every living player.
	//Also resets the currently held player data since it is no longer up to date (This system is the first system to run every frame for agents except the BT-system):
	for (auto& playerData : atmc.playerData)
		playerData = {};

	u8 playerIndex = 0u;
	DOG::EntityManager::Get().Collect<PlayerAliveComponent, DOG::TransformComponent>().Do([&](DOG::entity playerID, PlayerAliveComponent, DOG::TransformComponent& ptc)
		{
			auto& playerData = atmc.playerData[playerIndex++];
			playerData.playerID = playerID;
			playerData.position = ptc.GetPosition();
			playerData.distanceFromAgent = Vector3::Distance(tc.GetPosition(), ptc.GetPosition());
		});
	atmc.nrOfPlayersAlive = playerIndex;

	LEAF(btc.currentRunningNode)->Succeed(agent);
}

void AgentLineOfSightToPlayerSystem::OnEarlyUpdate(DOG::entity agent, BTLineOfSightToPlayerComponent&, AgentTargetMetricsComponent& atmc,
	DOG::TransformComponent& tc, BehaviorTreeComponent& btc)
{
	//This system checks for line of sight to every player. We are still in the gather-data-phase, so all players are analyzed:
	for (u8 i{ 0u }; i < atmc.nrOfPlayersAlive; ++i)
	{
		auto rayCastResult = PhysicsEngine::RayCast(tc.GetPosition(), atmc.playerData[i].position);
		//Nothing was hit at all:
		if (!rayCastResult)
			continue;
		bool thisPlayerHit = (rayCastResult->entityHit == atmc.playerData[i].playerID);
		//The player in question was not hit (line-of-sight does not exist):
		if (!thisPlayerHit)
			continue;

		//We are in line-of-sight, POSSIBLY. What remains is to check the dot product between the agent forward vector and 
		//the vector direction from the agent to the player, since the player could still, e.g., be behind the back:
		Vector3 vectorFromAgentToPlayer = (atmc.playerData[i].position - tc.GetPosition());
		vectorFromAgentToPlayer.Normalize();
		const float dot = tc.GetForward().Dot(vectorFromAgentToPlayer);

		constexpr const float minimumDotValue = 0.35f; //Range is from 0.35 -> 1.0
		if (dot > minimumDotValue)
			atmc.playerData[i].lineOfSight = AgentTargetMetricsComponent::LineOfSight::Full;
		else
			atmc.playerData[i].lineOfSight = AgentTargetMetricsComponent::LineOfSight::Partial;
	}
	LEAF(btc.currentRunningNode)->Succeed(agent);
}	

void AgentDetectPlayerSystem::OnEarlyUpdate(entity agentID, BTDetectPlayerComponent&, AgentSeekPlayerComponent& seek, 
	AgentIdComponent& agent, TransformComponent& transform, AgentTargetMetricsComponent& atmc, BehaviorTreeComponent& btc)
{
	/*This system will determine the aggro focus, if any, of the agent.
	   The rules are as follow:
	 - If a player is in FULL line of sight and within (inclusive) 8 metres it is a potential target.
	 - If a player is in PARTIAL line of sight and within (inclusive) 3 metres it is a potential target.
	 - Otherwise, the player is not a potential target.

	 The resulting target is, as of now, the potential target that is closest, since the agent has a better chance of
	 reaching that player at all before dying.

	 The corresponding BT-node fails if no players are potential targets.
	 */

	constexpr const f32 MINIMUM_AGGRO_DISTANCE = 8.0f; //Not taking into account distance from being shot.
	constexpr const f32 DIRECT_AGGRO_DISTANCE = 3.0f;
	EntityManager& eMan = EntityManager::Get();
	
	//Let's early out if all players are outside of minimum aggro distance (This will be true for a majority of agents):
	if (std::all_of(atmc.playerData.begin(), atmc.playerData.end(), 
		[&](const AgentTargetMetricsComponent::PlayerData& playerData)
		{ return playerData.distanceFromAgent > MINIMUM_AGGRO_DISTANCE; }))
	{
		LEAF(btc.currentRunningNode)->Fail(agentID);
		return;
	}

	//Find all potential targets (std::array to avoid heap from vector):
	u8 nrOfPotentialTargets = 0u;
	constexpr const u8 MAX_PLAYER_COUNT = 4u;
	std::array<AgentTargetMetricsComponent::PlayerData*, MAX_PLAYER_COUNT> potentialtargets;
	for (u8 i{0u}; i < atmc.nrOfPlayersAlive; ++i)
	{
		if (IsPotentialTarget(atmc.playerData[i]))
			potentialtargets[nrOfPotentialTargets++] = &atmc.playerData[i];
	}

	//No potential targets exist, even though some player(s) is/are within minimum range:
	if (nrOfPotentialTargets == 0u)
	{
		LEAF(btc.currentRunningNode)->Fail(agentID);
		return;
	}

	//We now find the final target:
	auto finalTarget = *std::min_element(potentialtargets.begin(), potentialtargets.end(), 
		[](const AgentTargetMetricsComponent::PlayerData* pLHS, const AgentTargetMetricsComponent::PlayerData* pRHS)
		{ return pLHS->distanceFromAgent < pRHS->distanceFromAgent; });

	
	



	//constexpr f32 SEEK_RADIUS_METERS = 5.0f;
	//constexpr f32 SEEK_RADIUS_SQUARED = SEEK_RADIUS_METERS * SEEK_RADIUS_METERS;
	//EntityManager& eMan = EntityManager::Get();
	//
	//AgentTargetMetricsComponent::PlayerData playerData;
	//const Vector3 agentPos(transform.GetPosition());
	//
	//eMan.Collect<PlayerAliveComponent, TransformComponent, NetworkPlayerComponent>().Do(
	//	[&](entity id,PlayerAliveComponent&, TransformComponent& transC, NetworkPlayerComponent& netPlayer) 
	//	{
	//		const f32 sqDist = Vector3::DistanceSquared(transC.GetPosition(), agentPos);
	//		if (sqDist < playerData.sqDist)
	//		{
	//			playerData.entityID = id;
	//			playerData.id = netPlayer.playerId;
	//			playerData.pos = transC.GetPosition();
	//			playerData.sqDist = sqDist;
	//		}
	//	});
	//
	//const bool agentAlreadyHasAggro = eMan.HasComponent<AgentAggroComponent>(e);
	//const bool playerIsWithinAggroRange = (playerData.sqDist < SEEK_RADIUS_SQUARED);
	//const bool agentDetectsPlayer = (agentAlreadyHasAggro || playerIsWithinAggroRange);
	//
	//auto& btc = eMan.GetComponent<BehaviorTreeComponent>(e);
	//if (agentDetectsPlayer)
	//{
	//	//if (!agentAlreadyHasAggro)
	//	//	eMan.AddComponent<AgentAggroComponent>(e);
	//
	//	// update target
	//	bool newTarget = playerData.entityID != seek.entityID;
	//
	//	seek.entityID = playerData.entityID;
	//	seek.direction = playerData.pos - agentPos;
	//	seek.direction.Normalize();
	//	seek.squaredDistance = playerData.sqDist;
	//
	//	if (seek.entityID != NULL_ENTITY)
	//	{
	//		// add or update PathfinderWalkComponent
	//		if (eMan.HasComponent<PathfinderWalkComponent>(e))
	//			eMan.GetComponent<PathfinderWalkComponent>(e).goal = eMan.GetComponent<TransformComponent>(seek.entityID).GetPosition();
	//		else
	//			eMan.AddComponent<PathfinderWalkComponent>(e).goal = eMan.GetComponent<TransformComponent>(seek.entityID).GetPosition();
	//	}
	//
	//	// add network signal
	//	if (newTarget)
	//	{
	//		NetworkAgentSeekPlayer* netSeek;
	//		if (!eMan.HasComponent<NetworkAgentSeekPlayer>(e))
	//			netSeek = &eMan.AddComponent<NetworkAgentSeekPlayer>(e);
	//		else
	//			netSeek = &eMan.GetComponent<NetworkAgentSeekPlayer>(e);
	//
	//		netSeek->playerID = playerData.id;
	//		netSeek->agentID = agent.id;
	//	}
	//
	//	LEAF(btc.currentRunningNode)->Succeed(e);
	//}
	//else
	//{
	//	// Lost target
	//	seek.entityID = DOG::NULL_ENTITY;
	//	// add network signal
	//	NetworkAgentSeekPlayer* netSeek;
	//	if (!eMan.HasComponent<NetworkAgentSeekPlayer>(e))
	//		netSeek = &eMan.AddComponent<NetworkAgentSeekPlayer>(e);
	//	else
	//		netSeek = &eMan.GetComponent<NetworkAgentSeekPlayer>(e);
	//
	//	// remove PathfinderWalkComponent
	//	if (eMan.HasComponent<PathfinderWalkComponent>(e))
	//		eMan.RemoveComponent<PathfinderWalkComponent>(e);
	//
	//	netSeek->playerID = -1;
	//	netSeek->agentID = agent.id;
	//
	//	LEAF(btc.currentRunningNode)->Fail(e);
	//}
}

const bool AgentDetectPlayerSystem::IsPotentialTarget(const AgentTargetMetricsComponent::PlayerData& playerData) noexcept
{
	constexpr const f32 MINIMUM_AGGRO_DISTANCE = 8.0f;
	constexpr const f32 FORCED_AGGRO_DISTANCE = 3.0f;

	const bool playerIsWithinMinimumRange = (playerData.distanceFromAgent <= MINIMUM_AGGRO_DISTANCE);
	const bool playerIsWithinForcedAggroRange = (playerData.distanceFromAgent <= FORCED_AGGRO_DISTANCE);
	const bool playerIsInFullLOS = (playerData.lineOfSight == AgentTargetMetricsComponent::LineOfSight::Full);
	const bool playerIsInPartialLOS = (playerData.lineOfSight == AgentTargetMetricsComponent::LineOfSight::Partial);

	const bool validAggro1 = (playerIsWithinMinimumRange && playerIsInFullLOS);
	const bool validAggro2 = (playerIsWithinForcedAggroRange && playerIsInPartialLOS);

	if (validAggro1 || validAggro2)
		return true;
	else
		return false;
}


/**************************************************
*				Regular Systems
***************************************************/


void AgentAttackSystem::OnUpdate(entity e, BTAttackComponent&, BehaviorTreeComponent& btc, 
	AgentAttackComponent& attack, AgentSeekPlayerComponent& seek)
{
	if (seek.HasTarget() && seek.squaredDistance <= attack.radiusSquared && attack.Ready())
	{
		PlayerManager::Get().HurtIfThisPlayer(seek.entityID, attack.damage);

		// Reset cooldown
		attack.timeOfLast = Time::ElapsedTime();

		if (EntityManager::Get().HasComponent<PlayerAliveComponent>(seek.entityID))
		{
			// Add visual effect
			const auto& pos1 = EntityManager::Get().GetComponent<TransformComponent>(seek.entityID).GetPosition();
			const auto& pos2 = EntityManager::Get().GetComponent<TransformComponent>(e).GetPosition();
			auto dir = pos2 - pos1;
			dir.Normalize();

			DOG::gfx::PostProcess::Get().InstantiateDamageDisk({ dir.x, dir.z }, 2.f, 1.5f);
		}

		LEAF(btc.currentRunningNode)->Succeed(e);
	}
	else
		LEAF(btc.currentRunningNode)->Fail(e);
}

void AgentHitDetectionSystem::OnUpdate(entity e, BTHitDetectComponent&, HasEnteredCollisionComponent& collision, AgentSeekPlayerComponent& seek)
{
	EntityManager& eMan = EntityManager::Get();

	auto& hit = eMan.AddOrGetComponent<AgentHitComponent>(e);
	//AgentHitComponent* hit;
	//if (!eMan.HasComponent<AgentHitComponent>(e))
	//	hit = &eMan.AddComponent<AgentHitComponent>(e);
	//else
	//	hit = &eMan.GetComponent<AgentHitComponent>(e);

	bool hitByPlayer = false;
	for (u32 i = 0; i < collision.entitiesCount; ++i)
	{
		if (eMan.HasComponent<BulletComponent>(collision.entities[i]))
		{
			entity bulletEntity = collision.entities[i];

			BulletComponent& bullet = eMan.GetComponent<BulletComponent>(bulletEntity);
			seek.entityID = bullet.playerEntityID;
			hit.HitBy(bulletEntity, bullet.playerEntityID, bullet.damage);
			if (!EntityManager::Get().HasComponent<AgentAggroComponent>(e))
				EntityManager::Get().AddComponent<AgentAggroComponent>(e);

			// Create a particle emitter for bullet hit effect
			auto& bulletTransform = eMan.GetComponent<TransformComponent>(bulletEntity);
			auto bulletPos = bulletTransform.GetPosition();
			auto& bulletScene = eMan.GetComponent<SceneComponent>(bulletEntity);
			
			auto playerPosition = eMan.GetComponent<TransformComponent>(bullet.playerEntityID).GetPosition();
			auto towardPlayer = (bulletPos - playerPosition);
			towardPlayer.Normalize();

			entity hitParticleEffect = eMan.CreateEntity();
			eMan.AddComponent<SceneComponent>(hitParticleEffect, bulletScene.scene);
			eMan.AddComponent<TransformComponent>(hitParticleEffect, bulletPos)
				.RotateForwardTo(towardPlayer)
				.RotateL(Vector3(DirectX::XM_PIDIV2, 0, 0));

			eMan.AddComponent<LifetimeComponent>(hitParticleEffect, 0.05f);
			eMan.AddComponent<ParticleEmitterComponent>(hitParticleEffect) = {
				.spawnRate = 32.f,
				.particleSize = 0.02f,
				.particleLifetime = .5f,
				.startColor = 3.0f * DirectX::SimpleMath::Vector4(0.5f, 0.1f, 0.1f, 1.f),
				.endColor = 3.0f * DirectX::SimpleMath::Vector4(0.5f, 0.1f, 0.1f, 1.f),
			};
			eMan.AddComponent<ConeSpawnComponent>(hitParticleEffect) = { .angle = DirectX::XM_PI/3, .speed = 2.f };
			
			hitByPlayer = true;
		}
	}

	auto& btc = eMan.GetComponent<BehaviorTreeComponent>(e);
	if (hitByPlayer)
		LEAF(btc.currentRunningNode)->Succeed(e);
	else
		LEAF(btc.currentRunningNode)->Fail(e);
}

void AgentHitSystem::OnUpdate(entity e, AgentHitComponent& hit, AgentHPComponent& hp)
{
	bool hitByPlayer = false;
	for (i8 i = 0; i < hit.count; ++i)
	{
		if (EntityManager::Get().HasComponent<ThisPlayer>(hit.hits[i].playerEntity))
		{
			hp.hp -= hit.hits[i].damage;
			hp.damageThisFrame = true;
			EntityManager::Get().Collect<ThisPlayer, InputController>().Do(
				[&](ThisPlayer&, InputController& inputC)
				{
					inputC.damageDoneToEnemies += hit.hits[i].damage;
				});
			hitByPlayer = true;
		}
	}
	if (hitByPlayer)
	{
		if (!EntityManager::Get().HasComponent<AgentAggroComponent>(e))
			EntityManager::Get().AddComponent<AgentAggroComponent>(e);
	}

	/*Frost status*/
	for (i8 i = 0; i < hit.count; ++i)
	{
		if (EntityManager::Get().HasComponent<FrostEffectComponent>(hit.hits[i].hitByEntity))
		{
			//Bullet has a frost effect. AddOrReplaceComponent-feature would be nice here, as it would
			//vastly shrink the following code block. (Note to self by Emil F)
			auto& fecBullet = EntityManager::Get().GetComponent<FrostEffectComponent>(hit.hits[i].hitByEntity);
			if (EntityManager::Get().HasComponent<FrostEffectComponent>(e))
			{
				auto& fecAgent = EntityManager::Get().GetComponent<FrostEffectComponent>(e);
				fecAgent.frostTimer = fecBullet.frostTimer;
			}
			else
			{
				//Agent speed is set to 1/2 of original for now:
				EntityManager::Get().GetComponent<AgentMovementComponent>(e).currentSpeed /= 2.0f; 
				EntityManager::Get().AddComponent<FrostEffectComponent>(e, fecBullet.frostTimer);
			}
			break;
		}
		if (EntityManager::Get().HasComponent<FireEffectComponent>(hit.hits[i].hitByEntity))
		{
			auto& fecBullet = EntityManager::Get().GetComponent<FireEffectComponent>(hit.hits[i].hitByEntity);

			if (EntityManager::Get().HasComponent<FireEffectComponent>(e))
			{
				EntityManager::Get().GetComponent<FireEffectComponent>(e).fireTimer = fecBullet.fireTimer;
			}
			else
			{
				FireEffectComponent& fire = EntityManager::Get().AddComponent<FireEffectComponent>(e, fecBullet.fireTimer, fecBullet.fireDamagePerSecond);
				fire.playerEntityID = fecBullet.playerEntityID;

				fire.particleEntity = EntityManager::Get().CreateEntity();

				EntityManager::Get().AddComponent<TransformComponent>(fire.particleEntity);
				EntityManager::Get().AddComponent<ChildComponent>(fire.particleEntity).parent = e;
				EntityManager::Get().AddComponent<ConeSpawnComponent>(fire.particleEntity) = { .angle = DirectX::XM_PI / 8.0f, .speed = 5.f };
				EntityManager::Get().AddComponent<ParticleEmitterComponent>(fire.particleEntity) = {
					.spawnRate = 200,
					.particleSize = 0.08f,
					.particleLifetime = 0.6f,
					.startColor = Vector4(1.0f, 0.0f, 0.0f, 0.8f),
					.endColor = Vector4(1.0f, 69.f / 255.0f, 0.0f, 0.0f),
				};
			}

			break;
		}
	}

	/* Signal to player that their bullet hit an enemy */
	for (i8 i = 0; i < std::min((u64)hit.count, hit.hits.size()); ++i)
	{
		if (EntityManager::Get().HasComponent<BulletComponent>(hit.hits[i].hitByEntity))
		{
			auto& bullet = EntityManager::Get().GetComponent<BulletComponent>(hit.hits[i].hitByEntity);
			LuaMain::GetEventSystem()->InvokeEvent("BulletEnemyHit"+std::to_string(bullet.playerEntityID), e);	// should it be entityID of player or hit object?
			//LuaMain::GetEventSystem()->InvokeEvent("BulletEnemyHit"+std::to_string(bullet.playerId), e);		// or is playerId needed, as in previous version?
		}
	}

	EntityManager::Get().RemoveComponent<AgentHitComponent>(e);
}

void AgentDestructSystem::OnUpdate(entity e, AgentHPComponent& hp, TransformComponent& trans)
{
	
	if (hp.hp <= 0 || trans.GetPosition().y < -10.0f)
	{
		AgentManager::Get().DestroyLocalAgent(e);
	}
}

void AgentFrostTimerSystem::OnUpdate(DOG::entity e, AgentMovementComponent& movement, FrostEffectComponent& frostEffect)
{
	frostEffect.frostTimer -= (float)Time::DeltaTime();
	if (frostEffect.frostTimer <= 0.0f)
	{
		movement.currentSpeed = movement.baseSpeed;
		EntityManager::Get().RemoveComponent<FrostEffectComponent>(e);
	}
}

void AgentAggroSystem::OnUpdate(DOG::entity e, BTAggroComponent&, AgentAggroComponent& aggro, AgentIdComponent& agent)
{
	constexpr float minutes = 1.3f;
	constexpr f64 maxAggroTime = minutes * 60.0;

	EntityManager& em = EntityManager::Get();
	AgentManager& am = AgentManager::Get();

	u32 myGroup = am.GroupID(agent.id);

	auto& btc = em.GetComponent<BehaviorTreeComponent>(e);
	if ((DOG::Time::ElapsedTime() - aggro.timeTriggered) > maxAggroTime)
	{
		em.RemoveComponent<AgentAggroComponent>(e);
		em.RemoveComponentIfExists<AgentAttackComponent>(e);
		LEAF(btc.currentRunningNode)->Fail(e);
	}
	else
	{
		// go to attack mode
		if (!em.HasComponent<AgentAttackComponent>(e))
			em.AddComponent<AgentAttackComponent>(e);

		bool signaledAnotherAgent = false;
		em.Collect<AgentIdComponent>().Do(
			[&](entity o, AgentIdComponent& other)
			{
				u32 otherGroup = am.GroupID(other.id);
				if (myGroup == otherGroup && !em.HasComponent<AgentAggroComponent>(o))
				{
					em.AddComponent<AgentAggroComponent>(o);
					signaledAnotherAgent = true;
				}
			}
		);
		if (signaledAnotherAgent)
		{
			LEAF(btc.currentRunningNode)->Succeed(e);
		}
		else
			LEAF(btc.currentRunningNode)->Fail(e);
	}
}


/**************************************************
*			Late Update Systems
***************************************************/


void AgentMovementSystem::OnLateUpdate(entity e, BTMoveToPlayerComponent&, BehaviorTreeComponent& btc, 
	AgentMovementComponent& movement, PathfinderWalkComponent& pfc, 
	RigidbodyComponent& rb, TransformComponent& trans)
{
	if (pfc.path.size() == 0)
		LEAF(btc.currentRunningNode)->Fail(e);
	else
	{
		movement.forward = pfc.path[0] - trans.GetPosition();
		movement.forward.y = 0.0f;

		trans.worldMatrix = Matrix::CreateLookAt(trans.GetPosition(), pfc.path[0], Vector3::Up).Invert();
		movement.forward.Normalize();
		constexpr f32 SKID_FACTOR = 0.1f;
		movement.forward.x += rb.linearVelocity.x * SKID_FACTOR;
		movement.forward.y = 0.0f;
		movement.forward.z += rb.linearVelocity.z * SKID_FACTOR;
		movement.forward.Normalize();
		movement.forward *= movement.currentSpeed;
		rb.linearVelocity.x = movement.forward.x;
		rb.linearVelocity.z = movement.forward.z;

		LEAF(btc.currentRunningNode)->Succeed(e);
	}
}

/***********************************************

			Shadow Agent Systems

***********************************************/


void ShadowAgentSeekPlayerSystem::OnUpdate(ShadowAgentSeekPlayerComponent& seek)
{
	if (seek.playerID == -1)
		seek.entityID = DOG::NULL_ENTITY;
	else
		EntityManager::Get().Collect<NetworkPlayerComponent>().Do([&](entity e, NetworkPlayerComponent& net)
			{
				std::cout << e << " " << (int)net.playerId << " -?-> " << (int)seek.playerID << std::endl;
				if (net.playerId == seek.playerID)
					seek.entityID = e;
			});
}

/***********************************************

			Late Update Agent Systems

***********************************************/

void LateAgentDestructCleanupSystem::OnLateUpdate(AgentIdComponent& agent, DeferredDeletionComponent&)
{
	AgentManager::Get().CountAgentKilled(agent.id);
};

void AgentFireTimerSystem::OnUpdate(DOG::entity e, AgentHPComponent& hpComponent, FireEffectComponent& fireEffect)
{
	f32 deltaTime = (f32)Time::DeltaTime();

	fireEffect.fireTimer -= deltaTime;
	if (fireEffect.fireTimer > 0.0f && PlayerManager::Get().GetThisPlayer() == fireEffect.playerEntityID)
	{
		hpComponent.hp -= fireEffect.fireDamagePerSecond * deltaTime;
		EntityManager::Get().Collect<ThisPlayer, InputController>().Do(
			[&](ThisPlayer&, InputController& inputC)
			{
				inputC.damageDoneToEnemies += fireEffect.fireDamagePerSecond * deltaTime;
			});
		hpComponent.damageThisFrame = true;

	}
	else
	{
		EntityManager::Get().DeferredEntityDestruction(fireEffect.particleEntity);
		EntityManager::Get().RemoveComponent<FireEffectComponent>(e);
	}
}
