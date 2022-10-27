#include "AgentBehaviorSystems.h"
#include "AgentManager.h"

using namespace DOG;

void AgentSeekPlayerSystem::OnUpdate(entity e, AgentSeekPlayerComponent& seek, AgentIdComponent& agent, TransformComponent& transform)
{
	EntityManager& eMan = EntityManager::Get();
	constexpr float SEEK_RADIUS_SQUARED = 256.0f;
	struct PlayerDist
	{
		entity entityID = NULL_ENTITY;
		i8 id = 0;
		f32 sqDist = 0.0f;
		Vector3 pos{ 0,0,0 };
	};

	PlayerDist player;
	Vector3 agentPos(transform.GetPosition());

	eMan.Collect<ThisPlayer, TransformComponent, NetworkPlayerComponent>().Do(
		[&](entity id, ThisPlayer&, TransformComponent& transC, NetworkPlayerComponent& netPlayer) {
			player.entityID = id;
			player.id = netPlayer.playerId;
			player.pos = transC.GetPosition();
			player.sqDist = Vector3::DistanceSquared(player.pos, agentPos);
		});

	eMan.Collect<OnlinePlayer, TransformComponent, NetworkPlayerComponent>().Do(
		[&](entity id, OnlinePlayer&, TransformComponent& transC, NetworkPlayerComponent& netPlayer) {
			f32 sqDist = Vector3::DistanceSquared(transC.GetPosition(), agentPos);
			if (sqDist < player.sqDist)
			{
				player.entityID = id;
				player.id = netPlayer.playerId;
				player.pos = transC.GetPosition();
				player.sqDist = sqDist;
			}
		});

	if (seek.aggro || player.sqDist < SEEK_RADIUS_SQUARED)
	{
		// update target
		bool newTarget = player.entityID != seek.entityID;

		seek.entityID = player.entityID;
		seek.direction = player.pos - agentPos;
		seek.direction.Normalize();
		seek.squaredDistance = player.sqDist;

		// add network signal
		if (newTarget)
		{
			NetworkAgentSeekPlayer* netSeek;
			if (!eMan.HasComponent<NetworkAgentSeekPlayer>(e))
				netSeek = &eMan.AddComponent<NetworkAgentSeekPlayer>(e);
			else
				netSeek = &eMan.GetComponent<NetworkAgentSeekPlayer>(e);

			(*netSeek).playerID = player.id;
			(*netSeek).agentID = agent.id;
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

		(*netSeek).playerID = -1;
		(*netSeek).agentID = agent.id;
	}
}


void AgentMovementSystem::OnUpdate(entity e, AgentMovementComponent& movement, 
	AgentPathfinderComponent& pathfinder, AgentSeekPlayerComponent& seek, TransformComponent& trans)
{
	if (seek.entityID == DOG::NULL_ENTITY)
	{
		// what to do when no player in sight
		//if (trans.GetPosition() != pathfinder.targetPos)
		//{
		//	trans.worldMatrix = Matrix::CreateLookAt(trans.GetPosition(), pathfinder.targetPos, Vector3(0, 1, 0)).Invert();
		//	trans.SetPosition(trans.GetPosition() + movement.forward * static_cast<f32>(movement.speed * Time::DeltaTime()));
		//	std::cout << "(" << trans.GetPosition().x << ", " << trans.GetPosition().y << ", " << trans.GetPosition().z << ")" << std::endl;
		//}
		//else
		//{
		//	pathfinder.targetPos = movement.station + AgentManager::GenerateRandomVector3(agent.id);
		//	movement.forward = pathfinder.targetPos - trans.GetPosition();
		//	movement.forward.Normalize();
		//	std::cout << "Setting position (" << pathfinder.targetPos.x << ", " << pathfinder.targetPos.y << ", " << pathfinder.targetPos.z << ")" << std::endl;
		//}
	}
	else
	{
		pathfinder.targetPos = EntityManager::Get().GetComponent<TransformComponent>(seek.entityID).GetPosition();
		pathfinder.targetPos += (-seek.direction * 2);
		trans.worldMatrix = Matrix::CreateLookAt(trans.GetPosition(), pathfinder.targetPos, Vector3(0, 1, 0)).Invert();
		movement.forward = seek.direction;
		// TODO: transfer actual movement responsibility to Pathfinder
		trans.SetPosition(trans.GetPosition() + movement.forward * static_cast<f32>(movement.currentSpeed * Time::DeltaTime()));
		
		if (!EntityManager::Get().HasComponent<AgentAttackComponent>(e))
			EntityManager::Get().AddComponent<AgentAttackComponent>(e);
	}
}

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
			if (attack.radiusSquared < seek.squaredDistance)
			{ 
				PlayerStatsComponent& player = EntityManager::Get().GetComponent<PlayerStatsComponent>(seek.entityID);
				player.health -= attack.damage;
				attack.elapsedTime = 0.0f;
				seek.aggro = false;
			}
		}
		else
		{
			attack.elapsedTime = static_cast<f32>(attack.elapsedTime + Time::DeltaTime());
		}
	}
}

void AgentHitDetectionSystem::OnUpdate(entity me, HasEnteredCollisionComponent& collision, AgentSeekPlayerComponent& seek)
{
	EntityManager& eMan = EntityManager::Get();
	AgentHitComponent* hit;
	if (!eMan.HasComponent<AgentHitComponent>(me))
		hit = &eMan.AddComponent<AgentHitComponent>(me);
	else
		hit = &eMan.GetComponent<AgentHitComponent>(me);
	for (u32 i = 0; i < collision.entitiesCount; ++i)
	{
		if (eMan.HasComponent<BulletComponent>(collision.entities[i]))
		{
			BulletComponent& bullet = eMan.GetComponent<BulletComponent>(collision.entities[i]);
			seek.entityID = bullet.playerEntityID;
			seek.aggro = true;
			(*hit).HitBy(collision.entities[i], bullet.playerEntityID, bullet.damage);
		}
	}
}

void AgentHitSystem::OnUpdate(entity e, AgentHitComponent& hit, AgentHPComponent& hp)
{
	for (i8 i = 0; i < hit.count; ++i)
	{
		//std::cout << hit.hits[i].hitByEntity << std::endl;;
		if (EntityManager::Get().HasComponent<ThisPlayer>(hit.hits[i].playerEntity))
		{
			hp.hp -= hit.hits[i].damage;
			hp.damageThisFrame = true;
		}
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
				//Agent speed is set to 1/3 of original for now:
				EntityManager::Get().GetComponent<AgentMovementComponent>(e).currentSpeed /= 3.0f; 
				EntityManager::Get().AddComponent<FrostEffectComponent>(e, fecBullet.frostTimer);
			}
			break;
		}
	}

	/* Signal to player that their bullet hit an enemy */
	for (i8 i = 0; i < std::min((u64)hit.count, hit.hits.size()); ++i)
	{
		auto& bullet = EntityManager::Get().GetComponent<BulletComponent>(hit.hits[i].hitByEntity);
		LuaMain::GetEventSystem()->InvokeEvent("BulletEnemyHit"+std::to_string(bullet.playerEntityID), e);	// should it be entityID of player or hit object?
		//LuaMain::GetEventSystem()->InvokeEvent("BulletEnemyHit"+std::to_string(bullet.playerId), e);		// or is playerId needed, as in previous version?
	}

	EntityManager::Get().RemoveComponent<AgentHitComponent>(e);
}

void AgentDestructSystem::OnUpdate(entity e, AgentHPComponent& hp, AgentIdComponent& agent, TransformComponent& trans)
{
	if (hp.hp <= 0 || trans.GetPosition().y < -10.0f)
	{
		EntityManager& eMan = EntityManager::Get();
		#if defined _DEBUG
		std::cout << "Agent " << eMan.GetComponent<AgentIdComponent>(e).id << " killed! ";
		if (hp.hp <= 0)
			std::cout << "HP: " << hp.hp << std::endl;
		else
			std::cout << "Position: (" << trans.GetPosition().x << ", " << trans.GetPosition().y << ", " << trans.GetPosition().z << ")" << std::endl;
		#endif
		
		// Send network signal to destroy agent
		CreateAndDestroyEntityComponent& kill = eMan.AddComponent<CreateAndDestroyEntityComponent>(e);
		kill.alive = false;
		kill.entityTypeId = agent.type;
		kill.id = agent.id;
		eMan.Collect<ThisPlayer, NetworkPlayerComponent>().Do(
			[&](ThisPlayer&, NetworkPlayerComponent& net) { kill.playerId = net.playerId; });	// what is the purpose of this?
		kill.position = trans.GetPosition();

		eMan.DeferredEntityDestruction(e);
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
