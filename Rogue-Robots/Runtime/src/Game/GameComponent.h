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

struct ModularBlockComponent : public DOG::Component<ModularBlockComponent>
{
	//
};