#pragma once
#include <DOGEngine.h>

enum class AgentBehavior
{
	Default,
	MoveTo,
	Attack,
	PopBehaviorFromStack = Default,
};

struct AgentStatsComponent
{
	float hp;
	float maxHP;
	float speed;
	//...

	u32 roomId;
};

struct AgentIdComponent
{
	u32 id;
};

struct AgentMovementComponent
{
	float speed = 15;
	DirectX::SimpleMath::Vector3 forward{1, 0, 0};
};

struct AgentHPComponent
{
	float hp;
	float maxHP;
};

struct AgentPathfinderComponent
{
	DirectX::SimpleMath::Vector3 targetPos;
	// TODO: PlannedPath
};

struct AgentSeekPlayerComponent
{
	i8 playerID = -1;
	DOG::entity entityID = 0;
	DirectX::SimpleMath::Vector3 direction;
};


/*******************************************

			Network Components
 
********************************************/

struct NetworkAgentStats
{
	int playerId;
	u32 objectId;
	AgentStatsComponent stats;
};

struct NetworkAgentBehaviorSync
{
	u32 agentID;
	AgentBehavior behavior;
};

struct NetworkAgentSeekPlayer
{
	i8 playerID = -1;
};
