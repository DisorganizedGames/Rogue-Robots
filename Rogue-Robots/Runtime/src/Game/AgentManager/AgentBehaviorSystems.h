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
		constexpr float seekRadius = 10.0f;
		struct PlayerDist
		{
			DOG::entity entityID;
			i8 id;
			Vector3 pos;
			Vector3 dir;
			float dist;
			PlayerDist(DOG::entity eID, i8 pID, const Vector3& p, const Vector3& a) : entityID(eID), id(pID), pos(p)
			{
				dir = p - a;
				dist = dir.Length();
				dir.Normalize();
			}
			bool operator<(PlayerDist& o) { return dist < o.dist; }
		};

		std::vector<PlayerDist> players;
		players.reserve(4);

		Vector3 agentPos(transform.GetPosition());

		eMan.Collect<DOG::ThisPlayer, DOG::TransformComponent, DOG::NetworkPlayerComponent>().Do(
			[&](DOG::entity id, DOG::ThisPlayer&, DOG::TransformComponent& transC, DOG::NetworkPlayerComponent& netPlayer) {
				PlayerDist dist(id, netPlayer.playerId, transC.GetPosition(), agentPos);
				players.push_back(dist);
			});

		eMan.Collect<DOG::OnlinePlayer, DOG::TransformComponent, DOG::NetworkPlayerComponent>().Do(
			[&](DOG::entity id, DOG::OnlinePlayer&, DOG::TransformComponent& transC, DOG::NetworkPlayerComponent& netPlayer) {
				PlayerDist dist(id, netPlayer.playerId, transC.GetPosition(), agentPos);
				players.push_back(dist);
			});

		std::sort(players.begin(), players.end());	// sort players in acending distance to agent

		PlayerDist& player = players[0];
		if (player.dist < seekRadius)
		{
			// new target
			if (player.id != seek.playerID)
			{
				//std::cout << "Agent " << agent.id << " detected player " << static_cast<int>(player.id) << std::endl;
				seek.playerID = player.id;
				seek.entityID = player.entityID;
				seek.direction = player.dir;
				// add network signal
				if (!eMan.HasComponent<NetworkAgentSeekPlayer>(e))
				{
					NetworkAgentSeekPlayer& netSeek = eMan.AddComponent<NetworkAgentSeekPlayer>(e);
					netSeek.playerID = player.id;
				}
				else
				{
					NetworkAgentSeekPlayer& netSeek = eMan.GetComponent<NetworkAgentSeekPlayer>(e);
					netSeek.playerID = player.id;
				}
			}
		}
		else if (seek.playerID != -1)
		{
			// Lost target
			//std::cout << "Agent " << agent.id << " lost sight of player " << static_cast<int>(seek.playerID) << std::endl;
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
	ON_UPDATE(AgentMovementComponent, AgentPathfinderComponent, AgentSeekPlayerComponent, DOG::TransformComponent);
	void OnUpdate(AgentMovementComponent& movement, AgentPathfinderComponent& pathfinder, AgentSeekPlayerComponent& seek, DOG::TransformComponent& trans)
	{
		if (seek.playerID == -1)
		{
			// do other movement
		}
		else
		{
			//std::cout << "(" << seek.direction.x << ", " << seek.direction.y << ", " << seek.direction.z << ")" << std::endl;
			pathfinder.targetPos = DOG::EntityManager::Get().GetComponent<DOG::TransformComponent>(seek.entityID).GetPosition();
			trans.worldMatrix = Matrix::CreateLookAt(trans.GetPosition(), pathfinder.targetPos, Vector3(0, 1, 0)).Invert();			
			movement.forward = seek.direction;
			trans.SetPosition(trans.GetPosition() + movement.forward * movement.speed * DOG::Time::DeltaTime());
		}
	}
};
