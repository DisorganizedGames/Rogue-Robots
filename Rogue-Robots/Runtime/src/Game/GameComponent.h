#pragma once
#include <DOGEngine.h>

struct GunComponent : public DOG::Component<GunComponent>
{
	//ScriptData gunScript;
	//TempScript* gunScript;
};

struct PlayerStatsComponent : public DOG::Component<PlayerStatsComponent>
{
	float health;
	float maxHealth;
	float speed;
	//...
};

