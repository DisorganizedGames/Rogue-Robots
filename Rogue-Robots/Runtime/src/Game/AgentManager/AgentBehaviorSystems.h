#pragma once
#include <DOGEngine.h>
#include "AgentComponents.h"



class AgentSeekPlayerSystem: public DOG::ISystem
{
	using Vector3 = DirectX::SimpleMath::Vector3;
public:
	SYSTEM_CLASS(AgentSeekPlayerComponent, AgentIdComponent);
	ON_UPDATE_ID(AgentSeekPlayerComponent, AgentIdComponent);
	void OnUpdate(DOG::entity e, AgentSeekPlayerComponent& seek, AgentIdComponent& agent)
	{
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

		EntityManager::Get().Collect<ThisPlayer, TransformComponent, NetworkPlayerComponent>().Do(
			[&](entity id, ThisPlayer&, TransformComponent& transC, NetworkPlayerComponent& netPlayer) {
				players.push_back(PlayerDist(id, netPlayer.playerId, transC.GetPosition(), (agentPos - transC.GetPosition()).Length()));
			});

		EntityManager::Get().Collect<OnlinePlayer, TransformComponent, NetworkPlayerComponent>().Do(
			[&](entity id, OnlinePlayer&, TransformComponent& transC, NetworkPlayerComponent& netPlayer) {
				players.push_back(PlayerDist(id, netPlayer.playerId, transC.GetPosition(), (agentPos - transC.GetPosition()).Length()));
			});

		std::sort(players.begin(), players.end());

		PlayerDist& player = players[0];
		if (player.dist < seekRadius)
		{
			seek.playerID = player.playerID;
			seek.playerPos = player.pos;
		}
		else
			seek.playerID = -1;
	}
};

//class AgentBehaviorMoveTo : public DOG::ISystem
//{
//	using Vector3 = DirectX::SimpleMath::Vector3;
//public:
//	SYSTEM_CLASS(AgentBehaviorMoveToComponent, AgentMovementComponent, DOG::TransformComponent);
//	ON_UPDATE(AgentBehaviorMoveToComponent, AgentMovementComponent, DOG::TransformComponent);
//	void OnUpdate(AgentBehaviorMoveToComponent& behavior, AgentMovementComponent& movement, DOG::TransformComponent& transform)
//	{
//
//	}
//};
