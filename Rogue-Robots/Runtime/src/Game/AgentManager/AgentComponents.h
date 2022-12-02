#pragma once
#include <DOGEngine.h>

enum class EntityTypes;

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
	EntityTypes type;
};

struct AgentMovementComponent
{
	f32 baseSpeed = 10.0f;		// TODO: make "global" based on type
	f32 currentSpeed = baseSpeed;
	DirectX::SimpleMath::Vector3 forward{1, 0, 0};
	DirectX::SimpleMath::Vector3 station{0, 0, 0};
};

struct AgentHPComponent
{
	f32 hp = 150.0f;
	f32 maxHP = 150.0f;
	bool damageThisFrame = false;
};

struct AgentAttackComponent
{
	f32 radiusSquared = 1.5f;
	f32 damage = 10.0f;
	f32 coolDown = 1.0f;
	f64 timeOfLast = 0;
	bool Ready() { return coolDown <= DOG::Time::ElapsedTime() - timeOfLast; }
};

struct AgentSeekPlayerComponent
{
	DOG::entity entityID = DOG::NULL_ENTITY;
	DirectX::SimpleMath::Vector3 direction;
	f32 squaredDistance = std::numeric_limits<float>::max();
};

struct AgentAggroComponent
{
	f64 timeTriggered = DOG::Time::ElapsedTime();
};

struct AgentHitComponent
{
	struct Hit
	{
		DOG::entity hitByEntity{ DOG::NULL_ENTITY };
		DOG::entity playerEntity{ DOG::NULL_ENTITY };
		f32 damage{ 0.0f };
	};
	i8 count = 0;
	std::array<Hit, 20> hits;
	void HitBy(DOG::entity projectile, DOG::entity player, f32 damage) {
		hits[count++ % hits.max_size()] = {projectile, player, damage }; }
	void HitBy(Hit hit) { hits[count++ % hits.max_size()] = hit; }
};

struct AgentCorpse
{

};

/*******************************************

			Shadow Agent Components

********************************************/

struct ShadowAgentSeekPlayerComponent
{
	i8 playerID = -1;
	DOG::entity entityID = DOG::NULL_ENTITY;
};

/*******************************************

			Network Components
 
********************************************/

struct NetworkAgentStats
{
	i32 playerId = 0;
	u32 objectId = 0;
	AgentHPComponent hp;
};

struct NetworkAgentSeekPlayer
{
	i8 playerID = -1;
	u32 agentID = u32(-1);
};
