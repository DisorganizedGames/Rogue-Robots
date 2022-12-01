#include "AgentBehaviorSystems.h"
#include "AgentManager.h"
#include "../DOGEngine/src/Graphics/Rendering/PostProcess.h"
#include "Game/PlayerManager/PlayerManager.h"

using namespace DOG;


/**************************************************
*			Early Update Systems
***************************************************/



void AgentSeekPlayerSystem::OnEarlyUpdate(entity e, AgentSeekPlayerComponent& seek, AgentIdComponent& agent, TransformComponent& transform)
{
	constexpr f32 SEEK_RADIUS_METERS = 5.0f;
	constexpr f32 SEEK_RADIUS_SQUARED = SEEK_RADIUS_METERS * SEEK_RADIUS_METERS;
	EntityManager& eMan = EntityManager::Get();
	struct PlayerDist
	{ 
		entity entityID = NULL_ENTITY;
		i8 id = 0;
		f32 sqDist = std::numeric_limits<f32>::max();
		Vector3 pos{ 0,0,0 };
	};

	PlayerDist player;
	Vector3 agentPos(transform.GetPosition());

	eMan.Collect<PlayerAliveComponent, TransformComponent, NetworkPlayerComponent>().Do(
		[&](entity id,PlayerAliveComponent&, TransformComponent& transC, NetworkPlayerComponent& netPlayer) {
			f32 sqDist = Vector3::DistanceSquared(transC.GetPosition(), agentPos);
			if (sqDist < player.sqDist)
			{
				player.entityID = id;
				player.id = netPlayer.playerId;
				player.pos = transC.GetPosition();
				player.sqDist = sqDist;
			}
		});

	bool aggro = eMan.HasComponent<AgentAggroComponent>(e);

	if (aggro || player.sqDist < SEEK_RADIUS_SQUARED)
	{
		if (!aggro)
			eMan.AddComponent<AgentAggroComponent>(e);


		// update target
		bool newTarget = player.entityID != seek.entityID;

		seek.entityID = player.entityID;
		seek.direction = player.pos - agentPos;
		seek.direction.Normalize();
		seek.squaredDistance = player.sqDist;

		if (seek.entityID != NULL_ENTITY)
		{
			// add or update PathfinderWalkComponent
			if (eMan.HasComponent<PathfinderWalkComponent>(e))
				eMan.GetComponent<PathfinderWalkComponent>(e).goal = eMan.GetComponent<TransformComponent>(seek.entityID).GetPosition();
			else
				eMan.AddComponent<PathfinderWalkComponent>(e).goal = eMan.GetComponent<TransformComponent>(seek.entityID).GetPosition();
		}

		// add network signal
		if (newTarget)
		{
			NetworkAgentSeekPlayer* netSeek;
			if (!eMan.HasComponent<NetworkAgentSeekPlayer>(e))
				netSeek = &eMan.AddComponent<NetworkAgentSeekPlayer>(e);
			else
				netSeek = &eMan.GetComponent<NetworkAgentSeekPlayer>(e);

			netSeek->playerID = player.id;
			netSeek->agentID = agent.id;
		}
	}
	else
	{
		// Lost target
		seek.entityID = DOG::NULL_ENTITY;
		// add network signal
		NetworkAgentSeekPlayer* netSeek;
		if (!eMan.HasComponent<NetworkAgentSeekPlayer>(e))
			netSeek = &eMan.AddComponent<NetworkAgentSeekPlayer>(e);
		else
			netSeek = &eMan.GetComponent<NetworkAgentSeekPlayer>(e);

		// remove PathfinderWalkComponent
		if (eMan.HasComponent<PathfinderWalkComponent>(e))
			eMan.RemoveComponent<PathfinderWalkComponent>(e);


		netSeek->playerID = -1;
		netSeek->agentID = agent.id;
	}
}


/**************************************************
*				Regular Systems
***************************************************/


void AgentAttackSystem::OnUpdate(entity e, AgentAttackComponent& attack, AgentSeekPlayerComponent& seek)
{
	if (seek.entityID != DOG::NULL_ENTITY && EntityManager::Get().HasComponent<ThisPlayer>(seek.entityID))
	{
		if (seek.entityID == DOG::NULL_ENTITY)
		{
			EntityManager::Get().RemoveComponent<AgentAttackComponent>(e);
		}
		else if (attack.coolDown <= attack.elapsedTime)
		{
			// TODO: update a PlayerHitComponent instead and let a PlayerManager handle HP adjustments for players
			if (seek.squaredDistance < attack.radiusSquared)
			{ 
				PlayerStatsComponent& player = EntityManager::Get().GetComponent<PlayerStatsComponent>(seek.entityID);
				player.health -= attack.damage;
				attack.elapsedTime = 0.0f;


				if (EntityManager::Get().HasComponent<PlayerAliveComponent>(seek.entityID))
				{
					// Add visual effect
					const auto& pos1 = EntityManager::Get().GetComponent<TransformComponent>(seek.entityID).GetPosition();
					const auto& pos2 = EntityManager::Get().GetComponent<TransformComponent>(e).GetPosition();
					auto dir = pos2 - pos1;
					dir.Normalize();

					DOG::gfx::PostProcess::Get().InstantiateDamageDisk({ dir.x, dir.z }, 2.f, 1.5f);
				}
			}
		}
		else
		{
			attack.elapsedTime = static_cast<f32>(attack.elapsedTime + Time::DeltaTime());
		}
	}
}

void AgentHitDetectionSystem::OnUpdate(entity e, HasEnteredCollisionComponent& collision, AgentSeekPlayerComponent& seek)
{
	EntityManager& eMan = EntityManager::Get();
	AgentHitComponent* hit;
	if (!eMan.HasComponent<AgentHitComponent>(e))
		hit = &eMan.AddComponent<AgentHitComponent>(e);
	else
		hit = &eMan.GetComponent<AgentHitComponent>(e);
	for (u32 i = 0; i < collision.entitiesCount; ++i)
	{
		if (eMan.HasComponent<BulletComponent>(collision.entities[i]))
		{
			entity bulletEntity = collision.entities[i];

			BulletComponent& bullet = eMan.GetComponent<BulletComponent>(bulletEntity);
			seek.entityID = bullet.playerEntityID;
			(*hit).HitBy(bulletEntity, bullet.playerEntityID, bullet.damage);
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
			
		}
	}
}

void AgentHitSystem::OnUpdate(entity e, AgentHitComponent& hit, AgentHPComponent& hp)
{
	for (i8 i = 0; i < hit.count; ++i)
	{

		if (EntityManager::Get().HasComponent<ThisPlayer>(hit.hits[i].playerEntity))
		{
			hp.hp -= hit.hits[i].damage;
			hp.damageThisFrame = true;
		}
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
		// Send network signal to destroy agents
		if (PlayerManager::Get().IsThisMultiplayer())
		{
			if (!hp.damageThisFrame)
				AgentManager::Get().DestroyLocalAgent(e);
		}
		else
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

void AgentAggroSystem::OnUpdate(DOG::entity e, AgentAggroComponent& aggro, AgentIdComponent& agent)
{
	constexpr float minutes = 1.3f;
	constexpr f64 maxAggroTime = minutes * 60.0;

	EntityManager& em = EntityManager::Get();
	AgentManager& am = AgentManager::Get();

	u32 myGroup = am.GroupID(agent.id);

	if ((DOG::Time::ElapsedTime() - aggro.timeTriggered) > maxAggroTime)
	{
		em.RemoveComponent<AgentAggroComponent>(e);
		em.RemoveComponentIfExists<AgentAttackComponent>(e);
	}
	else
	{
		// go to attack mode
		if (!em.HasComponent<AgentAttackComponent>(e))
			em.AddComponent<AgentAttackComponent>(e);

		em.Collect<AgentIdComponent>().Do(
			[&](entity o, AgentIdComponent& other)
			{
				u32 otherGroup = am.GroupID(other.id);
				if (myGroup == otherGroup && !em.HasComponent<AgentAggroComponent>(o))
					em.AddComponent<AgentAggroComponent>(o);
			}
		);
	}
}


/**************************************************
*			Late Update Systems
***************************************************/


void AgentMovementSystem::OnLateUpdate(AgentMovementComponent& movement, PathfinderWalkComponent& pfc, RigidbodyComponent& rb, TransformComponent& trans)
{

	if (pfc.path.size() != 0)
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

