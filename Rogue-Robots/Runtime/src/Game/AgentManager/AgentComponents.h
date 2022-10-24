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
	DOG::SceneType inScene;
};

struct AgentMovementComponent
{
	float speed;
	DirectX::SimpleMath::Vector3 forward;
};

struct AgentHPComponent
{
	float hp;
	float maxHP;
};

struct AgentPathfinderComponent
{
	DirectX::SimpleMath::Vector3 currentPos;
	DirectX::SimpleMath::Vector3 targetPos;
	// TODO: PlannedPath
};

struct AgentSeekPlayerComponent
{
	AgentSeekPlayerComponent() : playerID(-1), playerPos(Vector3{0,0,0})
	int playerID;
	Vector3 playerPos;
};

struct AgentBehaviorComponent
{
	AgentBehaviorComponent() : top(-1), stack{0} {}
	AgentBehavior Doing() { return top < -1 ? AgentBehavior::Default : stack[top]; }
	bool Push(AgentBehavior b) { return (top + 1) < stack.max_size() ? (stack[++top] = b) == b : false; }
	void Pop() { top < 0 ? top = -1 : --top; }
	char top;
	std::array<AgentBehavior, 5> stack;
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
