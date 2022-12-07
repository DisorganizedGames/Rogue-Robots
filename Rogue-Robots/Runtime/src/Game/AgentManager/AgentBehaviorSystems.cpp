#include "AgentBehaviorSystems.h"
#include "../DOGEngine/src/Graphics/Rendering/PostProcess.h"
#include "Game/PlayerManager/PlayerManager.h"
#include "Network/Network.h"
using namespace DOG;
using namespace DirectX::SimpleMath;

/**************************************************
*			Early Update Systems
***************************************************/

void AgentBehaviorTreeSystem::OnEarlyUpdate(entity agent, AgentIdComponent&, BehaviorTreeComponent& btc)
{
	ASSERT(btc.currentRunningNode, "Current running node is nullptr!");

	btc.currentRunningNode->Process(agent);
}

void AgentDistanceToPlayersSystem::OnEarlyUpdate(entity agent, BTDistanceToPlayerComponent&, AgentTargetMetricsComponent& atmc,
	AgentIdComponent& aidc, TransformComponent& tc, BehaviorTreeComponent& btc)
{
	//System checks the (non-squared) distance to every living player.
	//Also resets the currently held player data since it is no longer up to date (This system is the first system to run every frame for agents except the BT-system):
	const f32 AUDIO_RANGE = 15.0f;
	bool atLeastOneWithinAudioRange = false;
	
	atmc.playerData.clear();
	atmc.playerData.reserve(PlayerManager::Get().GetNrOfPlayers());

	f32 maxVision = AgentManager::Get().GetAgentStats(aidc.type).visionDistance;
	bool atLeastOneWithinRange = false;
	EntityManager::Get().Collect<PlayerAliveComponent, TransformComponent>().Do([&](entity playerID, PlayerAliveComponent, TransformComponent& ptc)
		{
			atmc.playerData.emplace_back(
				playerID, 
				Vector3::Distance(tc.GetPosition(), ptc.GetPosition()),
				ptc.GetPosition()
			);
			if (EntityManager::Get().HasComponent<ThisPlayer>(playerID))
			{
				//auto rayCastResult = PhysicsEngine::RayCast(tc.GetPosition(), ptc.GetPosition());

				//bool hitIsPlayer = (rayCastResult->entityHit == playerID);
				//if (hitIsPlayer)
				//{
				if (atmc.playerData.back().distanceFromAgent < 50.0f)
				{
					EntityManager::Get().GetComponent<PointLightComponent>(agent).dirty = true;
				}
			}
			atLeastOneWithinRange = atLeastOneWithinRange || (atmc.playerData.back().distanceFromAgent <= maxVision);

			atLeastOneWithinAudioRange = atLeastOneWithinAudioRange || (atmc.playerData.back().distanceFromAgent <= AUDIO_RANGE);
		});

	//Only one per group is played
	EntityManager::Get().Collect<AgentOnStandbyAudioComponent, AgentIdComponent>().Do([&](AgentOnStandbyAudioComponent, AgentIdComponent agentId)
		{
			AgentManager& am = AgentManager::Get();
			u32 myGroup = am.GroupID(agentId.id);
			u32 agentGroup = am.GroupID(aidc.id);

			if (agentId.id == aidc.id)
				return;

			atLeastOneWithinAudioRange = (myGroup != agentGroup) && atLeastOneWithinAudioRange;
		});

	//Add audiocomponent if wihtin range or else remove it
	entity e = agent;
	if (atLeastOneWithinAudioRange)
	{
		if (!EntityManager::Get().HasComponent<AgentAggroComponent>(e))
		{
			if (!EntityManager::Get().HasComponent<AgentOnStandbyAudioComponent>(e))
			{
				auto& agentOnStandbyComponent = EntityManager::Get().AddComponent<AgentOnStandbyAudioComponent>(e);
				agentOnStandbyComponent.agentOnStandbyAudioEntity = EntityManager::Get().CreateEntity();

				entity audioEntity = agentOnStandbyComponent.agentOnStandbyAudioEntity;
				EntityManager::Get().AddComponent<TransformComponent>(audioEntity);
				EntityManager::Get().AddComponent<ChildComponent>(audioEntity).parent = e;
				EntityManager::Get().AddComponent<SceneComponent>(audioEntity, EntityManager::Get().GetComponent<SceneComponent>(e).scene);
				EntityManager::Get().AddComponent<DOG::AudioComponent>(audioEntity);
			}

			auto& onStandbyAudio = EntityManager::Get().GetComponent<DOG::AudioComponent>(EntityManager::Get().GetComponent<AgentOnStandbyAudioComponent>(e).agentOnStandbyAudioEntity);
			if (!onStandbyAudio.playing)
			{
				onStandbyAudio.assetID = AssetManager::Get().LoadAudio("Assets/Audio/Enemy/OnStandby.wav");
				onStandbyAudio.shouldPlay = true;
				onStandbyAudio.volume = 1.0f;
				onStandbyAudio.is3D = true;
				onStandbyAudio.loop = true;
			}
		}
		else
		{
			if (EntityManager::Get().HasComponent<AgentOnStandbyAudioComponent>(e))
			{
				auto& onStandbyAudio = EntityManager::Get().GetComponent<DOG::AudioComponent>(EntityManager::Get().GetComponent<AgentOnStandbyAudioComponent>(e).agentOnStandbyAudioEntity);
				onStandbyAudio.shouldStop = true;
			}
		}
	}
	else
	{
		if (EntityManager::Get().HasComponent<AgentOnStandbyAudioComponent>(e))
		{
			auto& agentOnStandbyComponent = EntityManager::Get().GetComponent<AgentOnStandbyAudioComponent>(e);

			EntityManager::Get().DeferredEntityDestruction(agentOnStandbyComponent.agentOnStandbyAudioEntity);

			EntityManager::Get().RemoveComponent<AgentOnStandbyAudioComponent>(e);
		}
	}

	if (atLeastOneWithinRange)
		LEAF(btc.currentRunningNode)->Succeed(agent);
	else
		LEAF(btc.currentRunningNode)->Fail(agent);
}

void AgentLineOfSightToPlayerSystem::OnEarlyUpdate(entity agent, BTLineOfSightToPlayerComponent&, AgentTargetMetricsComponent& atmc,
	AgentIdComponent& aidc, TransformComponent& tc, BehaviorTreeComponent& btc)
{
	const AgentManager::AgentStats stats = AgentManager::Get().GetAgentStats(aidc.type);
	
	bool atLeastOneHasLineOfSight = false;
	//This system checks for line of sight to every player. We are still in the gather-data-phase, so all players are analyzed:
	for (auto& pd : atmc.playerData)
	{
		if (pd.distanceFromAgent <= stats.lidarDistance)
		{
			auto rayCastResult = PhysicsEngine::RayCast(tc.GetPosition(), pd.position);
			//Nothing was hit at all:
			if (!rayCastResult)
				continue;
			bool hitIsPlayer = (rayCastResult->entityHit == pd.playerID);
			//The player in question was not hit (line-of-sight does not exist):
			if (!hitIsPlayer)
				continue;

			pd.lineOfSight = AgentTargetMetricsComponent::LineOfSight::Partial;
			atLeastOneHasLineOfSight = true;
		}
		else if (pd.distanceFromAgent <= stats.visionDistance || EntityManager::Get().HasComponent<AgentAlertComponent>(agent))
		{
			//We are in line-of-sight, POSSIBLY. What remains is to check the dot product between the agent forward vector and 
			//the vector direction from the agent to the player, since the player could still, e.g., be behind the back:
			Vector3 vectorFromAgentToPlayer = (pd.position - tc.GetPosition());
			vectorFromAgentToPlayer.Normalize();
			const float dot = tc.GetForward().Dot(vectorFromAgentToPlayer);

			if (dot > stats.visionConeDotValue)
			{
				auto rayCastResult = PhysicsEngine::RayCast(tc.GetPosition(), pd.position);
				//Nothing was hit at all:
				if (!rayCastResult)
					continue;
				bool hitIsPlayer = (rayCastResult->entityHit == pd.playerID);
				//The player in question was not hit (line-of-sight does not exist):
				if (!hitIsPlayer)
					continue;

				pd.lineOfSight = AgentTargetMetricsComponent::LineOfSight::Full;
				atLeastOneHasLineOfSight = true;
			}
		}
	}

	if (atLeastOneHasLineOfSight)
		LEAF(btc.currentRunningNode)->Succeed(agent);
	else
		LEAF(btc.currentRunningNode)->Fail(agent);
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

	const AgentManager::AgentStats stats = AgentManager::Get().GetAgentStats(agent.type);

	//Find all potential targets:
	std::vector<AgentTargetMetricsComponent::PlayerData> potentialTargets;
	for (auto& pd : atmc.playerData)
	{
		if (IsPotentialTarget(pd, stats))
			potentialTargets.push_back(pd);
	}

	//No potential targets exist, even though some player(s) is/are within minimum range:
	if (potentialTargets.empty())
	{
		seek.entityID = NULL_ENTITY;
		LEAF(btc.currentRunningNode)->Fail(agentID);
		return;
	}

	//We now find the final target:
	AgentTargetMetricsComponent::PlayerData finalTarget = *std::min_element(potentialTargets.begin(), potentialTargets.end());

	seek.entityID = finalTarget.playerID;
	seek.distanceToPlayer = finalTarget.distanceFromAgent;
	seek.direction = transform.GetPosition() - finalTarget.position;

	LEAF(btc.currentRunningNode)->Succeed(agentID);
}

const bool AgentDetectPlayerSystem::IsPotentialTarget(const AgentTargetMetricsComponent::PlayerData& playerData, const AgentManager::AgentStats& stats) noexcept
{
	const bool playerIsWithinMinimumRange = (playerData.distanceFromAgent <= stats.visionConeDotValue);
	const bool playerIsWithinForcedAggroRange = (playerData.distanceFromAgent <= stats.lidarDistance);
	const bool playerIsInFullLOS = (playerData.lineOfSight == AgentTargetMetricsComponent::LineOfSight::Full);
	const bool playerIsInPartialLOS = (playerData.lineOfSight == AgentTargetMetricsComponent::LineOfSight::Partial);

	const bool validAggro1 = (playerIsWithinMinimumRange && playerIsInFullLOS);
	const bool validAggro2 = (playerIsWithinForcedAggroRange && playerIsInPartialLOS);

	if (validAggro1 || validAggro2)
		return true;
	else
		return false;
}

void AgentDetectHitSystem::OnEarlyUpdate(entity agentID, BTHitDetectComponent&, AgentSeekPlayerComponent& seek,
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

	const AgentManager::AgentStats stats = AgentManager::Get().GetAgentStats(agent.type);

	//Find all potential targets:
	std::vector<AgentTargetMetricsComponent::PlayerData> potentialTargets;
	for (auto& pd : atmc.playerData)
	{
		if (pd.lineOfSight != AgentTargetMetricsComponent::LineOfSight::None)
			potentialTargets.push_back(pd);
	}

	//No potential targets exist, even though some player(s) is/are within minimum range:
	if (potentialTargets.empty())
	{
		seek.entityID = NULL_ENTITY;
		LEAF(btc.currentRunningNode)->Fail(agentID);
		return;
	}

	//We now find the final target:
	AgentTargetMetricsComponent::PlayerData finalTarget = *std::min_element(potentialTargets.begin(), potentialTargets.end());

	seek.entityID = finalTarget.playerID;
	seek.distanceToPlayer = finalTarget.distanceFromAgent;
	seek.direction = transform.GetPosition() - finalTarget.position;

	EntityManager& em = EntityManager::Get();

	if (!em.HasComponent<AgentAggroComponent>(agentID))
	{
		em.AddComponent<AgentAggroComponent>(agentID);
		if (!em.HasComponent<PathFindingSync>(agentID))
		{
			em.AddComponent<PathFindingSync>(agentID).id = em.GetComponent<AgentIdComponent>(agentID);
			em.GetComponent<PathFindingSync>(agentID).id.id = em.GetComponent<PathFindingSync>(agentID).id.id | AGGRO_BIT;
		}
	}
	LEAF(btc.currentRunningNode)->Succeed(agentID);
}

void AgentGetPathSystem::OnEarlyUpdate(entity e, BTGetPathComponent&, AgentSeekPlayerComponent& seek, BehaviorTreeComponent& btc)
{
	EntityManager& em = EntityManager::Get();
	em.AddOrGetComponent<PathfinderWalkComponent>(e).goal = em.GetComponent<TransformComponent>(seek.entityID).GetPosition();
	
	LEAF(btc.currentRunningNode)->Succeed(e);
}


/**************************************************
*				Regular Systems
***************************************************/

void AgentAttackSystem::OnUpdate(entity e, BTAttackComponent&, BehaviorTreeComponent& btc, 
	AgentAttackComponent& attack, AgentSeekPlayerComponent& seek)
{
	if (seek.HasTarget() && attack.Ready()
		&& seek.distanceToPlayer <= 1.5f)	// TODO: hard coded - change to dynamic
	{
		PlayerManager::Get().HurtIfThisPlayer(seek.entityID, attack.damage, e);

		if (!EntityManager::Get().HasComponent<AgentAttackAudioComponent>(e))
		{
			auto& attackAudio = EntityManager::Get().AddComponent<AgentAttackAudioComponent>(e);
			attackAudio.agentAttackAudioComponent = EntityManager::Get().CreateEntity();

			EntityManager::Get().AddComponent<TransformComponent>(attackAudio.agentAttackAudioComponent);
			EntityManager::Get().AddComponent<ChildComponent>(attackAudio.agentAttackAudioComponent).parent = e;

			EntityManager::Get().AddComponent<DOG::AudioComponent>(attackAudio.agentAttackAudioComponent);
		}

		auto& attackAudio = EntityManager::Get().GetComponent<AgentAttackAudioComponent>(e);
		auto& audio = EntityManager::Get().GetComponent<DOG::AudioComponent>(attackAudio.agentAttackAudioComponent);
		audio.shouldPlay = true;
		audio.is3D = true;
		audio.assetID = AssetManager::Get().LoadAudio("Assets/Audio/Enemy/Attack.wav");

		// Reset cooldown
		attack.timeOfLast = Time::ElapsedTime();

		LEAF(btc.currentRunningNode)->Succeed(e);
	}
	else
		LEAF(btc.currentRunningNode)->Fail(e);
}

void AgentJumpAtPlayerSystem::OnUpdate(entity e, BTJumpAtPlayerComponent&, BehaviorTreeComponent& btc,
	AgentAttackComponent& attack, AgentSeekPlayerComponent& seek)
{
	if (seek.HasTarget() && attack.Ready()
		&& 1.5f < seek.distanceToPlayer && seek.distanceToPlayer < 7.f)  // TODO: hard coded - change to dynamic
	{
		EntityManager& em = EntityManager::Get();
		AgentMovementComponent& movement = em.GetComponent<AgentMovementComponent>(e);
		RigidbodyComponent& rb = em.GetComponent<RigidbodyComponent>(e);
		TransformComponent& trans = em.GetComponent<TransformComponent>(e);
		Vector3 agentPos = trans.GetPosition();
		Vector3 playerPos = em.GetComponent<TransformComponent>(seek.entityID).GetPosition();
		movement.forward = playerPos - agentPos;
		movement.forward.Normalize();

		Vector3 up = Vector3::Up;
		Vector3 right = up.Cross(movement.forward);

		right.Normalize();
		up = movement.forward.Cross(right);

		DirectX::SimpleMath::Matrix r(right, up, movement.forward);
		trans.SetRotation(r);


		constexpr f32 SKID_FACTOR = 0.1f;
		movement.forward += rb.linearVelocity * SKID_FACTOR;
		movement.forward.Normalize();
		movement.forward *= movement.currentSpeed;
		rb.linearVelocity = movement.forward;

		LEAF(btc.currentRunningNode)->Succeed(e);
	}
	else
		LEAF(btc.currentRunningNode)->Fail(e);
}

void AgentPullBackSystem::OnUpdate(entity e, BTPullBackComponent&, BehaviorTreeComponent& btc,
	AgentAttackComponent& attack, AgentSeekPlayerComponent& seek)
{
	if (seek.HasTarget() && !attack.Ready()
		&& seek.distanceToPlayer < 7.f)  // TODO: hard coded - change to dynamic
	{
		EntityManager& em = EntityManager::Get();
		AgentMovementComponent& movement = em.GetComponent<AgentMovementComponent>(e);
		RigidbodyComponent& rb = em.GetComponent<RigidbodyComponent>(e);
		TransformComponent& trans = em.GetComponent<TransformComponent>(e);
		Vector3 agentPos = trans.GetPosition();
		Vector3 playerPos = em.GetComponent<TransformComponent>(seek.entityID).GetPosition();
		movement.forward = playerPos - agentPos;
		movement.forward.Normalize();

		Vector3 up = Vector3::Up;
		Vector3 right = up.Cross(movement.forward);

		right.Normalize();
		up = movement.forward.Cross(right);

		DirectX::SimpleMath::Matrix r(right, up, movement.forward);
		trans.SetRotation(r);


		constexpr f32 SKID_FACTOR = 0.1f;
		movement.forward += rb.linearVelocity * SKID_FACTOR;
		movement.forward.Normalize();
		movement.forward *= movement.currentSpeed;
		Vector3 back = movement.forward;
		back.x *= -1;
		back.z *= -1;
		rb.linearVelocity = back;

		LEAF(btc.currentRunningNode)->Succeed(e);
	}
	else
		LEAF(btc.currentRunningNode)->Fail(e);
}

void AgentDodgeSystem::OnUpdate(entity e, BTDodgeComponent&, BehaviorTreeComponent& btc,
	AgentAttackComponent& attack, AgentSeekPlayerComponent& seek)
{
	if (seek.HasTarget() && seek.distanceToPlayer <= attack.radius && attack.Ready())
	{
		PlayerManager::Get().HurtIfThisPlayer(seek.entityID, attack.damage, e);

		// Reset cooldown
		attack.timeOfLast = Time::ElapsedTime();

		LEAF(btc.currentRunningNode)->Succeed(e);
	}
	else
		LEAF(btc.currentRunningNode)->Fail(e);
}

void AgentHitDetectionSystem::OnUpdate(entity e, HasEnteredCollisionComponent& collision, AgentSeekPlayerComponent& seek)
{
	EntityManager& eMan = EntityManager::Get();

	auto& hit = eMan.AddOrGetComponent<AgentHitComponent>(e);


	bool hitByPlayer = false;
	for (u32 i = 0; i < collision.entitiesCount; ++i)
	{
		if (eMan.HasComponent<BulletComponent>(collision.entities[i]))
		{
			entity bulletEntity = collision.entities[i];

			BulletComponent& bullet = eMan.GetComponent<BulletComponent>(bulletEntity);
			seek.entityID = bullet.playerEntityID;
			hit.HitBy(bulletEntity, bullet.playerEntityID, bullet.damage);

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
}

AgentHitSystem::AgentHitSystem()
{
	m_hitSounds.push_back(AssetManager::Get().LoadAudio("Assets/Audio/Enemy/Damage1.wav"));
	m_hitSounds.push_back(AssetManager::Get().LoadAudio("Assets/Audio/Enemy/Damage2.wav"));
	m_hitSounds.push_back(AssetManager::Get().LoadAudio("Assets/Audio/Enemy/Damage4.wav"));
}

void AgentHitSystem::OnUpdate(entity e, AgentHitComponent& hit, AgentHPComponent& hp)
{
	bool hitByPlayer = false;
	for (i8 i = 0; i < hit.count; ++i)
	{
		if (!EntityManager::Get().HasComponent<AgentHitAudioComponent>(e) || !EntityManager::Get().Exists(EntityManager::Get().GetComponent<AgentHitAudioComponent>(e).agentHitAudioEntity))
		{
			auto& hitAudio = EntityManager::Get().AddOrReplaceComponent<AgentHitAudioComponent>(e);
			hitAudio.agentHitAudioEntity = EntityManager::Get().CreateEntity();
			EntityManager::Get().AddComponent<TransformComponent>(hitAudio.agentHitAudioEntity);
			EntityManager::Get().AddComponent<ChildComponent>(hitAudio.agentHitAudioEntity).parent = e;

			EntityManager::Get().AddComponent<DOG::AudioComponent>(hitAudio.agentHitAudioEntity);
		}

		auto& hitAudio = EntityManager::Get().GetComponent<AgentHitAudioComponent>(e);
		auto& audio = EntityManager::Get().GetComponent<DOG::AudioComponent>(hitAudio.agentHitAudioEntity);
		if (!audio.playing)
		{
			audio.shouldPlay = true;
			audio.assetID = m_hitSounds[rand() % m_hitSounds.size()];
			audio.is3D = true;
			audio.volume = 1.0f;
			EntityManager::Get().AddOrReplaceComponent<LifetimeComponent>(hitAudio.agentHitAudioEntity, 10.0f);
		}

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

			if (!audio.playing)
			{
				audio.is3D = false;
				audio.volume = 0.25f;
			}
		}
	}
	if (hitByPlayer)
	{
		if (!EntityManager::Get().HasComponent<AgentAlertComponent>(e))
			EntityManager::Get().AddComponent<AgentAlertComponent>(e);
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
				auto& frostEffect = EntityManager::Get().AddComponent<FrostEffectComponent>(e, fecBullet.frostTimer);
				frostEffect.frostAudioEntity = EntityManager::Get().CreateEntity();
				EntityManager::Get().AddComponent<TransformComponent>(frostEffect.frostAudioEntity);
				EntityManager::Get().AddComponent<ChildComponent>(frostEffect.frostAudioEntity).parent = e;
				auto& audio = EntityManager::Get().AddComponent<DOG::AudioComponent>(frostEffect.frostAudioEntity);
				audio.assetID = AssetManager::Get().LoadAudio("Assets/Audio/Enemy/Frost.wav");
				audio.is3D = true;
				audio.loop = true;
				audio.shouldPlay = true;
				audio.volume = 2.5f;
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
				auto& audio = EntityManager::Get().AddComponent<DOG::AudioComponent>(fire.particleEntity);
				audio.assetID = AssetManager::Get().LoadAudio("Assets/Audio/Enemy/Fire.wav");
				audio.is3D = true;
				audio.loop = true;
				audio.shouldPlay = true;
				audio.volume = 1.8f;
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

void AgentFrostTimerSystem::OnUpdate(entity e, AgentMovementComponent& movement, FrostEffectComponent& frostEffect, AgentIdComponent& idc)
{
	frostEffect.frostTimer -= (float)Time::DeltaTime();
	if (frostEffect.frostTimer <= 0.0f)
	{
		movement.currentSpeed = AgentManager::Get().GetAgentStats(idc.type).baseSpeed;
		EntityManager::Get().DeferredEntityDestruction(frostEffect.frostAudioEntity);
		EntityManager::Get().RemoveComponent<FrostEffectComponent>(e);
	}
}

void AgentAggroSystem::OnUpdate(entity e, BTAggroComponent&, AgentAggroComponent& aggro, AgentIdComponent& agent)
{
	constexpr float minutes = 1.3f;
	constexpr f64 maxAggroTime = minutes * 60.0;

	EntityManager& em = EntityManager::Get();
	AgentManager& am = AgentManager::Get();

	if (!em.HasComponent<AgentAlertComponent>(e))
		em.AddComponent<AgentAlertComponent>(e);

	u32 myGroup = am.GroupID(agent.id);

	auto& btc = em.GetComponent<BehaviorTreeComponent>(e);
	if ((Time::ElapsedTime() - aggro.timeTriggered) > maxAggroTime)
	{
		em.RemoveComponent<AgentAggroComponent>(e);
		em.RemoveComponentIfExists<AgentAggroAudioComponent>(e);
		if (!em.HasComponent<PathFindingSync>(e))
		{
			em.AddComponent<PathFindingSync>(e).id = em.GetComponent<AgentIdComponent>(e);
			em.GetComponent<PathFindingSync>(e).id.id = em.GetComponent<PathFindingSync>(e).id.id | AGGRO_BIT;
		}
		em.RemoveComponentIfExists<AgentAttackComponent>(e);
		LEAF(btc.currentRunningNode)->Fail(e);
	}
	else
	{
		// go to attack mode
		if (!em.HasComponent<AgentAttackComponent>(e))
			em.AddComponent<AgentAttackComponent>(e);

		// Give aggro component to the group
		bool signaledAnotherAgent = false;
		em.Collect<AgentIdComponent>().Do(
			[&](entity o, AgentIdComponent& other)
			{
				u32 otherGroup = am.GroupID(other.id);
				if (myGroup == otherGroup && !em.HasComponent<AgentAlertComponent>(o))
				{
					em.AddComponent<AgentAlertComponent>(o);
					if (!em.HasComponent<AgentAggroComponent>(o))
						em.AddComponent<AgentAggroComponent>(o);
					if (!em.HasComponent<PathFindingSync>(o))
					{
						em.AddComponent<PathFindingSync>(o).id = EntityManager::Get().GetComponent<AgentIdComponent>(o);
						em.GetComponent<PathFindingSync>(o).id.id = EntityManager::Get().GetComponent<PathFindingSync>(o).id.id | AGGRO_BIT;
					}
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

		if (!EntityManager::Get().HasComponent<AgentAggroAudioComponent>(e))
		{
			auto& aggroAudioComponent = EntityManager::Get().AddComponent<AgentAggroAudioComponent>(e);
			aggroAudioComponent.agentAggroAudioEntity = EntityManager::Get().CreateEntity();

			entity aggroAudioEntity = aggroAudioComponent.agentAggroAudioEntity;
			EntityManager::Get().AddComponent<TransformComponent>(aggroAudioEntity);
			EntityManager::Get().AddComponent<ChildComponent>(aggroAudioEntity).parent = e;
			EntityManager::Get().AddComponent<LifetimeComponent>(aggroAudioEntity, 5.0f);
			auto& aggroAudio = EntityManager::Get().AddComponent<DOG::AudioComponent>(aggroAudioEntity);
			aggroAudio.assetID = AssetManager::Get().LoadAudio("Assets/Audio/Enemy/Aggro.wav");
			aggroAudio.shouldPlay = true;
			aggroAudio.volume = 0.6f;
		}
	}
}


/**************************************************
*			Late Update Systems
***************************************************/


AgentMovementSystem::AgentMovementSystem()
{
	m_walkingSounds.push_back(AssetManager::Get().LoadAudio("Assets/Audio/Enemy/Walking_1.wav"));
	m_walkingSounds.push_back(AssetManager::Get().LoadAudio("Assets/Audio/Enemy/Walking_2.wav"));
	m_walkingSounds.push_back(AssetManager::Get().LoadAudio("Assets/Audio/Enemy/Walking_3.wav"));
	m_walkingSounds.push_back(AssetManager::Get().LoadAudio("Assets/Audio/Enemy/Walking_4.wav"));
	m_walkingSounds.push_back(AssetManager::Get().LoadAudio("Assets/Audio/Enemy/Walking_5.wav"));
	m_walkingSounds.push_back(AssetManager::Get().LoadAudio("Assets/Audio/Enemy/Walking_6.wav"));
}

void AgentMovementSystem::OnLateUpdate(entity e, BTMoveToPlayerComponent&, BehaviorTreeComponent& btc, 
	AgentMovementComponent& movement, AgentSeekPlayerComponent& seek, PathfinderWalkComponent& pfc, 
	RigidbodyComponent& rb, TransformComponent& trans)
{
	if (pfc.path.size() == 0)
		LEAF(btc.currentRunningNode)->Fail(e);
	else if (seek.entityID != NULL_ENTITY && 5.0f < seek.distanceToPlayer)	// TODO: hardcoded 5 for now - change to dynamic value
	{
		movement.forward = pfc.path[0] - trans.GetPosition();
		movement.forward.y = 0.0f;
		movement.forward.Normalize();

		// Old solution was to make view matrix and then invert it.
		//trans.worldMatrix = DirectX::XMMatrixLookAtLH(trans.GetPosition(), pfc.path[0], Vector3::Up);
		//trans.worldMatrix = trans.worldMatrix.Invert();		// could find a better solution...

		Vector3 up = Vector3::Up;
		Vector3 right = up.Cross(movement.forward);
		
		// The forward vector has y = 0. => the up vector will always be {0, 1, 0}, so we can skipp the last cross product.
		//right.Normalize();
		//up = forward.Cross(right);

		DirectX::SimpleMath::Matrix r(right, up, movement.forward);
		trans.SetRotation(r);

		
		constexpr f32 SKID_FACTOR = 0.1f;
		movement.forward.x += rb.linearVelocity.x * SKID_FACTOR;
		movement.forward.y = 0.0f;
		movement.forward.z += rb.linearVelocity.z * SKID_FACTOR;
		movement.forward.Normalize();
		movement.forward *= movement.currentSpeed;
		rb.linearVelocity.x = movement.forward.x;
		rb.linearVelocity.z = movement.forward.z;

		LEAF(btc.currentRunningNode)->Succeed(e);

		if (!EntityManager::Get().HasComponent<DOG::AudioComponent>(e))
		{
			EntityManager::Get().AddComponent<DOG::AudioComponent>(e).is3D = true;
		}
		
		auto& audio = EntityManager::Get().GetComponent<DOG::AudioComponent>(e);
		if (!audio.playing)
		{
			int walkingSoundIndex = rand() % m_walkingSounds.size();
			audio.assetID = m_walkingSounds[walkingSoundIndex];
			audio.shouldPlay = true;
			audio.volume = 0.5f;
		}
	}
	else
	{
		constexpr Vector3 DEACCELERATE{ .3f, 1.f, .3f };
		constexpr f32 THRESHOLD = 0.3f;

		// stop or reduce speed
		if (rb.linearVelocity.x + rb.linearVelocity.z < THRESHOLD)
		{
			rb.linearVelocity.x = 0;
			rb.linearVelocity.z = 0;
		}
		else
			rb.linearVelocity *= DEACCELERATE;
	}
}

/***********************************************

			Shadow Agent Systems

***********************************************/


void ShadowAgentSeekPlayerSystem::OnUpdate(ShadowAgentSeekPlayerComponent& seek)
{
	if (seek.playerID == -1)
		seek.entityID = NULL_ENTITY;
	else
		EntityManager::Get().Collect<NetworkPlayerComponent>().Do([&](entity e, NetworkPlayerComponent& net)
			{
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

void AgentFireTimerSystem::OnUpdate(entity e, AgentHPComponent& hpComponent, FireEffectComponent& fireEffect)
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
