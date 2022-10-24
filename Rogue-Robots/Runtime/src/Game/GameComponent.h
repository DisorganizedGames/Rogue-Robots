#pragma once
#include <DOGEngine.h>
using namespace DirectX::SimpleMath;



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

	u32 roomId;
};

struct BulletComponent
{
	i8 playerId;
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
	bool switchBarrelComp = false;
	bool activateActiveItem = false;
};

struct DoorComponent
{
	u32 roomId = u32(-1);
	bool isOpening = false;
	f32 openValue = 0.f;
};

struct PassiveItemComponent {
	enum class Type
	{
		Template
	};

	Type type;
};

struct CreateAndDestroyEntityComponent
{
	u32 entityTypeId = 0;
	u32 id = 0;
	bool alive = true;
	i8 playerId = 0; 
	Vector3 position;
};
