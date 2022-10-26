#pragma once
#include <DOGEngine.h>
#include "../GameComponent.h"

#include "DirectXMath.h"
#include <DirectXTK/SimpleMath.h>

class AgentSeekPlayerSystem: public DOG::ISystem
{
	using Vector3 = DirectX::SimpleMath::Vector3;
	using Matrix = DirectX::SimpleMath::Matrix;
public:
	SYSTEM_CLASS(AgentSeekPlayerComponent, AgentIdComponent, DOG::TransformComponent);
	ON_UPDATE_ID(AgentSeekPlayerComponent, AgentIdComponent, DOG::TransformComponent);
	void OnUpdate(DOG::entity e, AgentSeekPlayerComponent& seek, AgentIdComponent& agent, DOG::TransformComponent& transform)
	{
		DOG::EntityManager& eMan = DOG::EntityManager::Get();
		constexpr float SEEK_RADIUS = 10.0f;
		struct PlayerDist
		{
			DOG::entity entityID = DOG::NULL_ENTITY;
			i8 id = 0;
			Vector3 pos{ 0,0,0 };
			Vector3 dir{ 0,0,0 };
			float squaredDist = 0.0f;
			PlayerDist() = default;
			PlayerDist(DOG::entity eID, i8 pID, const Vector3& p, const Vector3& a) : entityID(eID), id(pID), pos(p)
			{
				dir = p - a;
				squaredDist = Vector3::DistanceSquared(p, a);
				dir.Normalize();
			}
			bool operator<(PlayerDist& o) { return squaredDist < o.squaredDist; }
		};

		PlayerDist player;

		Vector3 agentPos(transform.GetPosition());

		eMan.Collect<DOG::ThisPlayer, DOG::TransformComponent, DOG::NetworkPlayerComponent>().Do(
			[&](DOG::entity id, DOG::ThisPlayer&, DOG::TransformComponent& transC, DOG::NetworkPlayerComponent& netPlayer) {
				PlayerDist dist(id, netPlayer.playerId, transC.GetPosition(), agentPos);
				player = dist;
			});

		eMan.Collect<DOG::OnlinePlayer, DOG::TransformComponent, DOG::NetworkPlayerComponent>().Do(
			[&](DOG::entity id, DOG::OnlinePlayer&, DOG::TransformComponent& transC, DOG::NetworkPlayerComponent& netPlayer) {
				PlayerDist dist(id, netPlayer.playerId, transC.GetPosition(), agentPos);
				if (dist < player)
					player = dist;
			});

		if (player.squaredDist < SEEK_RADIUS)
		{
			// new target
			if (player.id != seek.playerID)
			{
				seek.playerID = player.id;
				seek.entityID = player.entityID;
				seek.direction = player.dir;
				seek.squaredDistance = player.squaredDist;
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
};

class AgentMovementSystem : public DOG::ISystem
{
	using Vector3 = DirectX::SimpleMath::Vector3;
	using Matrix = DirectX::SimpleMath::Matrix;
public:
	SYSTEM_CLASS(AgentMovementComponent, AgentPathfinderComponent, AgentSeekPlayerComponent, DOG::TransformComponent);
	ON_UPDATE_ID(AgentMovementComponent, AgentPathfinderComponent, AgentSeekPlayerComponent, DOG::TransformComponent);
	void OnUpdate(DOG::entity e, AgentMovementComponent& movement, AgentPathfinderComponent& pathfinder, AgentSeekPlayerComponent& seek, DOG::TransformComponent& trans)
	{
		if (seek.playerID == -1)
		{
			// do other movement
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
};

class AgentAttackSystem : public DOG::ISystem
{
	using Vector3 = DirectX::SimpleMath::Vector3;
	using Matrix = DirectX::SimpleMath::Matrix;
public:
	SYSTEM_CLASS(AgentAttackComponent, AgentSeekPlayerComponent);
	ON_UPDATE_ID(AgentAttackComponent, AgentSeekPlayerComponent);
	void OnUpdate(DOG::entity e, AgentAttackComponent& attack, AgentSeekPlayerComponent& seek)
	{
		if (seek.playerID == -1)
		{
			DOG::EntityManager::Get().RemoveComponent<AgentAttackComponent>(e);
		}
		else if (attack.coolDown < attack.elapsedTime && attack.radiusSquared < seek.squaredDistance)
		{
			PlayerStatsComponent& player = DOG::EntityManager::Get().GetComponent<PlayerStatsComponent>(seek.entityID);
			player.health -= attack.damage;
			attack.elapsedTime = 0.0f;
		}
		else
		{
			attack.elapsedTime = static_cast<f32>(attack.elapsedTime + DOG::Time::DeltaTime());
		}
	}
};
