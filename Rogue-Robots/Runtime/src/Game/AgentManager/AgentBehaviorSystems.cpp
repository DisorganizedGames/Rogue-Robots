#include "AgentBehaviorSystems.h"

using namespace DOG;

void AgentSeekPlayerSystem::OnUpdate(entity e, AgentSeekPlayerComponent& seek, AgentIdComponent& agent, TransformComponent& transform)
{
	EntityManager& eMan = EntityManager::Get();
	bool host = true;
	eMan.Collect<ThisPlayer, NetworkPlayerComponent>().Do([&](ThisPlayer&, NetworkPlayerComponent& net)
		{
			host = net.playerId == 0;
		});
	if (host)
	{
		constexpr float SEEK_RADIUS_SQUARED = 64.0f;
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

		if (player.sqDist < SEEK_RADIUS_SQUARED)
		{
			// new target
			if (player.id != seek.playerID)
			{
				seek.playerID = player.id;
				seek.entityID = player.entityID;
				seek.direction = player.pos - agentPos;
				seek.direction.Normalize();
				seek.squaredDistance = player.sqDist;
				// add network signal
				if (!eMan.HasComponent<NetworkAgentSeekPlayer>(e))
				{
					NetworkAgentSeekPlayer& netSeek = eMan.AddComponent<NetworkAgentSeekPlayer>(e);
					netSeek.playerID = player.id;
					netSeek.agentID = agent.id;
				}
				else
				{
					NetworkAgentSeekPlayer& netSeek = eMan.GetComponent<NetworkAgentSeekPlayer>(e);
					netSeek.playerID = player.id;
					netSeek.agentID = agent.id;
				}
			}
		}
		else if (seek.playerID != -1)
		{
			// Lost target
			seek.playerID = -1;
			// add network signal
			if (!eMan.HasComponent<NetworkAgentSeekPlayer>(e))
			{
				NetworkAgentSeekPlayer& netSeek = eMan.AddComponent<NetworkAgentSeekPlayer>(e);
				netSeek.playerID = -1;
			}
			else
			{
				NetworkAgentSeekPlayer& netSeek = eMan.GetComponent<NetworkAgentSeekPlayer>(e);
				netSeek.playerID = -1;
			}
		}
	}
}


void AgentMovementSystem::OnUpdate(DOG::entity e, AgentMovementComponent& movement, AgentPathfinderComponent& pathfinder, AgentSeekPlayerComponent& seek, DOG::TransformComponent& trans)
{
	if (seek.playerID == -1)
	{
		// what to do when no player in sight
	}
	else
	{
		pathfinder.targetPos = DOG::EntityManager::Get().GetComponent<DOG::TransformComponent>(seek.entityID).GetPosition();
		pathfinder.targetPos += (-seek.direction * 2);
		trans.worldMatrix = Matrix::CreateLookAt(trans.GetPosition(), pathfinder.targetPos, Vector3(0, 1, 0)).Invert();
		movement.forward = seek.direction;
		trans.SetPosition(trans.GetPosition() + movement.forward * static_cast<f32>(movement.speed * DOG::Time::DeltaTime()));
		if (!DOG::EntityManager::Get().HasComponent<AgentAttackComponent>(e))
			DOG::EntityManager::Get().AddComponent<AgentAttackComponent>(e);
	}
}

void AgentAttackSystem::OnUpdate(entity e, AgentAttackComponent& attack, AgentSeekPlayerComponent& seek)
{
	// Do everything except...
	// ...do damage if target is ThisPlayer
	if (EntityManager::Get().HasComponent<ThisPlayer>(seek.entityID))
	{
		if (seek.playerID == -1)
		{
			EntityManager::Get().RemoveComponent<AgentAttackComponent>(e);
		}
		else if (attack.coolDown < attack.elapsedTime && attack.radiusSquared < seek.squaredDistance)
		{
			PlayerStatsComponent& player = EntityManager::Get().GetComponent<PlayerStatsComponent>(seek.entityID);
			player.health -= attack.damage;
			attack.elapsedTime = 0.0f;
		}
		else
		{
			attack.elapsedTime = static_cast<f32>(attack.elapsedTime + Time::DeltaTime());
		}
	}
}

void AgentHitSystem::OnUpdate(entity e, AgentHitComponent& hit, AgentHPComponent& hp)
{
	for (i8 i = 0; i < hit.count; ++i)
		hp.hp -= 100;
	if (hit.count >= hit.entityID.max_size())
		std::cout << "Number of hits: " << (int)hit.count << std::endl;
	EntityManager::Get().RemoveComponent<AgentHitComponent>(e);
}

void AgentDestructSystem::OnUpdate(DOG::entity e, AgentHPComponent& hp)
{
	bool host = true;
	EntityManager::Get().Collect<ThisPlayer, NetworkPlayerComponent>().Do(
		[&](ThisPlayer&, NetworkPlayerComponent& net)
		{
			host = net.playerId == 0;
		});
	if (host && hp.hp <= 0)
	{
		std::cout << "Agent " << e << " killed!" << std::endl;
		EntityManager::Get().DeferredEntityDestruction(e);
	}
}
