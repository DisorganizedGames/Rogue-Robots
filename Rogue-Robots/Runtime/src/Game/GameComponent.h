#pragma once
#include <DOGEngine.h>
#include "AgentManager/AgentComponents.h"
#include "EntitesTypes.h"



struct GunComponent
{
	//ScriptData gunScript;
	//TempScript* gunScript;
};

struct PlayerStatsComponent
{
	f32 maxHealth = 100.f;
	f32 health = 10.f;//maxHealth;
	f32 speed = 10.f;
	f32 lifeSteal = 0.f;
	//...
};

struct BulletComponent
{
	i8 playerId;
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
	bool switchMagazineComp = false;
	bool activateActiveItem = false;
};

struct DoorComponent
{
	u32 roomId = u32(-1);
	f32 openDisplacementY = 5.f;
	bool isOpening = false;
	f32 openValue = 0.f;
};

struct PassiveItemComponent {
	enum class Type
	{
		Template,
		MaxHealthBoost,
		SpeedBoost,
		LifeSteal,
	};

	Type type;
};

struct CreateAndDestroyEntityComponent
{
	EntityTypes entityTypeId = EntityTypes::Default;
	u32 id = 0;
	bool alive = true;
	i8 playerId = 0; 
	DirectX::SimpleMath::Vector3 position;
};

struct FrostEffectComponent
{
	//??
	f32 frostTimer = 0.0f;
};

struct ExplosionComponent
{
	ExplosionComponent(float explosionPower, float explosionRadius) noexcept
	{
		power = explosionPower;
		radius = explosionRadius;
	};
	float power;
	float radius;
};

//You do not have control over the entity! The system does!
struct ExplosionEffectComponent
{
	ExplosionEffectComponent(float explosionEffectRadius) noexcept
	{
		radius = explosionEffectRadius;
	};
	float radius;
	float growTime = -1.0f; 
	float shrinkTime = -1.0f;
};
