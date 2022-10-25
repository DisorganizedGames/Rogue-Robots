#pragma once
#include <DOGEngine.h>
#include "../GameComponent.h"



class AgentSeekPlayerSystem: public DOG::ISystem
{
	using Vector3 = DirectX::SimpleMath::Vector3;
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
			int id;
			Vector3 pos;
			float dist;
			PlayerDist(DOG::entity eID, int pID, Vector3 p, float d) : entityID(eID), id(pID), pos(p), dist(d) {}
			bool operator<(PlayerDist& o) { return dist < o.dist; }
		};

		std::vector<PlayerDist> players;
		players.reserve(4);

		Vector3 agentPos(transform.GetPosition());

		eMan.Collect<DOG::ThisPlayer, DOG::TransformComponent, DOG::NetworkPlayerComponent>().Do(
			[&](DOG::entity id, DOG::ThisPlayer&, DOG::TransformComponent& transC, DOG::NetworkPlayerComponent& netPlayer) {
				players.push_back(PlayerDist(id, netPlayer.playerId, transC.GetPosition(), (agentPos - transC.GetPosition()).Length()));
			});

		eMan.Collect<DOG::OnlinePlayer, DOG::TransformComponent, DOG::NetworkPlayerComponent>().Do(
			[&](DOG::entity id, DOG::OnlinePlayer&, DOG::TransformComponent& transC, DOG::NetworkPlayerComponent& netPlayer) {
				players.push_back(PlayerDist(id, netPlayer.playerId, transC.GetPosition(), (agentPos - transC.GetPosition()).Length()));
			});

		std::sort(players.begin(), players.end());	// sort players in acending distance to agent

		PlayerDist& player = players[0];
		if (player.dist < seekRadius)
		{
			// new target
			if (player.id != seek.playerID)
			{
				std::cout << "Agent " << agent.id << " detected player " << player.id << std::endl;
				seek.playerID = player.id;
				seek.entityID = player.entityID;
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
			std::cout << "Agent " << agent.id << " lost sight of player " << seek.playerID << std::endl;
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

class AgentMoveToSystem : public DOG::ISystem
{
	using Vector3 = DirectX::SimpleMath::Vector3;
public:
	SYSTEM_CLASS(AgentMovementComponent, AgentPathfinderComponent, AgentSeekPlayerComponent);
	ON_UPDATE(AgentMovementComponent, AgentPathfinderComponent, AgentSeekPlayerComponent);
	void OnUpdate(AgentMovementComponent& movement, AgentPathfinderComponent& pathfinder, AgentSeekPlayerComponent& seek)
	{
		if (seek.playerID == -1)
		{
			// do other movement
		}
		else
		{
			pathfinder.targetPos = DOG::EntityManager::Get().GetComponent<DOG::TransformComponent>(seek.entityID).GetPosition();
		}
	}
};
