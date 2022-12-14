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

struct AgentAlertComponent
{
	//ID
};

struct AgentIdComponent
{
	u32 id;
	EntityTypes type;
};

struct AgentMovementComponent
{
	f32 currentSpeed;
	DirectX::SimpleMath::Vector3 forward{1, 0, 0};
	DirectX::SimpleMath::Vector3 station{0, 0, 0};
};

struct AgentHPComponent
{
	f32 hp = 175.0f;
	f32 maxHP = 175.0f;
	bool damageThisFrame = false;
};

struct AgentAttackComponent
{
	f32 radius = 1.5f;
	f32 damage = 5.5f;
	f32 coolDown = 1.0f;
	f64 timeOfLast = 0;
	bool Ready() { return coolDown <= DOG::Time::ElapsedTime() - timeOfLast; }
};

struct AgentTargetMetricsComponent
{
	enum class LineOfSight { Full = 0u, Partial, None };
	struct PlayerData
	{
		DOG::entity playerID = DOG::NULL_ENTITY;
		f32 distanceFromAgent = std::numeric_limits<f32>::max();
		DirectX::SimpleMath::Vector3 position{ 0,0,0 };
		LineOfSight lineOfSight{ LineOfSight::None };
		bool operator<(const PlayerData& other) 
		{ 
			return lineOfSight < other.lineOfSight
				|| lineOfSight == other.lineOfSight && distanceFromAgent < other.distanceFromAgent;
		}
		bool operator<=(const f32 dist) { return distanceFromAgent <= dist; }
	};
	std::vector<PlayerData> playerData;
};

struct AgentSeekPlayerComponent
{
	DOG::entity entityID = DOG::NULL_ENTITY;
	DirectX::SimpleMath::Vector3 direction;
	f32 distanceToPlayer = std::numeric_limits<float>::max();
	bool HasTarget() { return entityID != DOG::NULL_ENTITY; }
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

struct AgentHitAudioComponent
{
	DOG::entity agentHitAudioEntity = DOG::NULL_ENTITY;
};

struct AgentAttackAudioComponent
{
	DOG::entity agentAttackAudioComponent = DOG::NULL_ENTITY;
};

struct AgentAggroAudioComponent
{
	DOG::entity agentAggroAudioEntity = DOG::NULL_ENTITY;
};

struct AgentOnStandbyAudioComponent
{
	DOG::entity agentOnStandbyAudioEntity = DOG::NULL_ENTITY;
};

struct AgentPatrolComponent
{
	f64 timer;
	bool successfullyCreated = true;
	f32 ratio;
	f32 turnSpeed;
	f32 orientation;
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
