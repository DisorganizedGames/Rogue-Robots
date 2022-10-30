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
	f32 health = maxHealth;
	f32 speed = 10.f;
	f32 lifeSteal = 0.f;
	//...
};

struct BulletComponent
{
	DOG::entity playerEntityID;
	f32 damage;
};

struct HomingMissileComponent
{
	HomingMissileComponent() = default;
	HomingMissileComponent(DOG::entity target) : homingTarget(target) {}
	float speed = 30;
	float turnSpeed = 5;
	float engineBurnTime = 1.8f;
	float explosionRadius = 10.0f;
	float dmg = 300.0f;
	DOG::entity homingTarget = DOG::NULL_ENTITY;
	DOG::entity playerEntityID = DOG::NULL_ENTITY;
	bool homeInOnPosition = false;
	DirectX::SimpleMath::Vector3 targetPosition;
	bool launched = true;
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
	bool reload = false;
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
	u32 id = u32(-1);
	bool alive = true;
	i8 playerId = -1; 
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


/**********************************************************

			Temporary Components for MVP

***********************************************************/

struct PlayersRoom0Component {};

struct PlayersRoom1Component {};

struct PlayersRoom2Component {};

struct PlayersRoom3Component {};
