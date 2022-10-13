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

struct InputController
{
	bool forward = false;
	bool left = false;
	bool right = false;
	bool backwards = false;
	bool up = false;
	bool down = false;
	bool shoot = false;
	bool jump = false;
	bool switchComp = false;
	bool activateActiveItem = false;
};
