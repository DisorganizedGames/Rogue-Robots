#pragma once
#include <DOGEngine.h>

struct GunComponent
{
	//ScriptData gunScript;
	//TempScript* gunScript;
};

struct PlayerStatsComponent
{
	float health;
	float maxHealth;
	float speed;
	//...
};


struct AgentStatsComponent
{
	float hp;
	float maxHP;
	float speed;
	//...
};

struct BulletComponent
{

};
struct NetworkAgentStats
{
	int playerId;
	u32 objectId;
	AgentStatsComponent stats;
};
