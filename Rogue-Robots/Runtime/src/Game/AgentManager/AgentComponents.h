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
	f64 speed = 15.0f;
	DirectX::SimpleMath::Vector3 forward{1, 0, 0};
};

struct AgentHPComponent
{
	f32 hp = 100.0f;
	f32 maxHP = 100.0f;
};

struct AgentAttackComponent
{
	f32 radiusSquared = 1.5f;
	f32 damage = 10.0f;
	f32 coolDown = 1.0f;
	f32 elapsedTime = coolDown;
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
	f32 squaredDistance = 0.0f;
};

struct AgentHitComponent
{
	i8 count = 0;
	std::array<DOG::entity, 5> entityID{ DOG::NULL_ENTITY };
	void HitBy(DOG::entity e) { entityID[count++ % entityID.max_size()] = e; }
};

/*******************************************

			Network Components
 
********************************************/

struct NetworkAgentStats
{
	i32 playerId;
	u32 objectId;
	AgentStatsComponent stats;
};

struct NetworkAgentSeekPlayer
{
	i8 playerID = -1;
	u32 agentID = u32(-1);
};
