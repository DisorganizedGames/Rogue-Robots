#pragma once
#include <DOGEngine.h>
#include "AgentManager/AgentComponents.h"
#include "Pathfinder/PathfinderComponents.h"
#include "EntitesTypes.h"
#include "AnimationIDs.h"
#include "Core/QueryHelpers.h" // included here to leak it everywhere.

struct PlayerControllerComponent
{
	f32 azimuthal = DirectX::XM_PIDIV2;
	f32 polar = DirectX::XM_PIDIV2;

	f32 mouseSensitivity = 1 / 2000.f;

	DOG::entity cameraEntity = DOG::NULL_ENTITY;
	DOG::entity debugCamera = DOG::NULL_ENTITY;
	DOG::entity spectatorCamera = DOG::NULL_ENTITY;

	bool moveView = true;
	bool jumping = false;
};

struct GunComponent
{
	//ScriptData gunScript;
	//TempScript* gunScript;
};

struct PlayerStatsComponent
{
	f32 maxHealth = 100.f;
	f32 health = maxHealth;
	f32 speed = 5.5f;
	f32 lifeSteal = 0.f;
	f32 jumpSpeed = 11.0f;
	//...
};

struct PlayerAliveComponent
{
};

struct SceneComponent
{
	enum class Type
	{
		Global = 0,
		TestScene,
		OldDefaultScene,
		LightScene,
		ParticleScene,
		PCGLevelScene,
	};
	SceneComponent(Type scene) : scene(scene) {}
	Type scene;
};


struct BulletComponent
{
	DOG::entity playerEntityID;
	f32 damage;
};

struct HomingMissileComponent
{
	float startMotorSpeed = 5.0f;
	float mainMotorSpeed = 20;
	float turnSpeed = 4.0f;
	float flightTime = 0.0f;
	float engineStartTime = 0.1f;
	float attackFlightPhaseStartTime = engineStartTime + 0.1f;
	float lifeTime = attackFlightPhaseStartTime + 2.0f;
	float explosionRadius = 10.0f;
	float dmg = 500.0f;
	bool armed = false;
	bool homing = true;
	int hit = 0;
	DOG::entity homingTarget = DOG::NULL_ENTITY;
	DOG::entity playerEntityID = DOG::NULL_ENTITY;
	DOG::entity jet = DOG::NULL_ENTITY;
	bool launched = true;
	bool engineIsIgnited = false;
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
	bool toggleDebug = false;
	bool toggleMoveView = false;
	bool flashlight = true;
	bool revive = false;
	bool throwGlowStick = false;
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
		SpeedBoost2,
		JumpBoost,
	};

	Type type;
};

//The active item that currently resides in inventory
struct ActiveItemComponent
{
	enum class Type{ Trampoline = 0, Turret, Reviver, GoalRadar, Syringe };

	Type type;
};

//ID component for ALL pick ups
struct PickupComponent
{
	const char* itemName;

	enum class Type{ ActiveItem = 0, PassiveItem, BarrelItem, MagazineModificationItem, MiscItem };

	Type type;
};

struct EligibleForPickupComponent
{
	DOG::entity player;
};

struct BarrelComponent
{
	enum class Type { Bullet = 0, Grenade, Missile, Laser };

	Type type;
	u32 maximumAmmoCapacityForType;
	u32 ammoPerPickup;
	u32 currentAmmoCount;
};

struct MagazineModificationComponent
{
	enum class Type { None = 0, Frost = 1 };

	Type type;
};

struct MiscComponent
{
	enum class Type { Basic = 0, FullAuto, ChargeShot };

	Type type;
};

struct LerpToPlayerComponent
{
	DOG::entity player;
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

struct InteractionQueryComponent
{
	//ID
};

struct ChildComponent
{
	DOG::entity parent = DOG::NULL_ENTITY;
	DOG::TransformComponent localTransform;
	bool nodeHasBeenUpdated = false;
};

struct ChildToBoneComponent
{
	DOG::entity boneParent = DOG::NULL_ENTITY;
	DOG::TransformComponent localTransform;
	bool nodeHasBeenUpdated = false;
};

struct WeaponLightComponent
{

};

struct DespawnComponent
{
	DespawnComponent(f64 despawnTime)
	{
		despawnTimer = despawnTime + DOG::Time::ElapsedTime();
	};
	f64 despawnTimer = 0.0f;
};

struct SphereComponent
{
	float radius;
};

struct TurretTargetingComponent
{
	float maxRange = 50.0;
	DOG::entity trackedTarget = DOG::NULL_ENTITY;

	float yawSpeed = 1.5f;
	float pitchSpeed = 0.6f;

	float yawLimit = DirectX::XM_PI / 2;
	float pitchLimit = DirectX::XM_PI / 12;

	bool shoot = false;
};

struct TurretBasicShootingComponent
{
	DOG::entity owningPlayer = DOG::NULL_ENTITY;
	u32 ammoCount = 200;
	f32 projectileSpeed = 100;
	f64 timeStep = 0.2f;
	f64 lastDischargeTimer = timeStep;
	f32 damage = 50;
	f32 projectileLifeTime = 3;
};

struct TurretProjectileComponent
{
	float maxLifeTime = 3.0f;
	float lifeTime = 0;
};

struct DeathUITimerComponent
{
	float duration;
	float timeLeft;
};

struct SpectatorComponent
{
	DOG::entity playerBeingSpectated;
	const char* playerName;
	std::vector<DOG::entity> playerSpectatorQueue;
};

struct ReviveTimerComponent
{
	//Acts as both ID and timer data holder.
	float duration{5.0f};
	float timeLeft{ duration };
	DOG::entity playerBeingRevived{DOG::NULL_ENTITY};
};

struct LifetimeComponent
{
	f32 lifetime = 0.f;
	f32 age = 0.f;

	LifetimeComponent(f32 lifetime) : lifetime(lifetime) {}
};

//ModularBlocks

struct SpawnBlockComponent
{
};

struct ExitBlockComponent
{
};

struct GoalRadarComponent
{
	f32 timeVisible{ 0.f };
};

struct FloorBlockComponent
{
};

struct CheckForLightsComponent
{
};

struct LaserBeamVFXComponent
{
	DirectX::SimpleMath::Vector3 startPos;
	DirectX::SimpleMath::Vector3 endPos;
	DirectX::SimpleMath::Vector3 color;
};

struct LaserBeamComponent
{
	DirectX::SimpleMath::Vector3 startPos;
	DirectX::SimpleMath::Vector3 direction;
	f32 maxRange = 100;
	DirectX::SimpleMath::Vector3 color;
	f32 damage;
	DOG::entity owningPlayer = DOG::NULL_ENTITY;
};

struct LaserBulletComponent
{
	DirectX::SimpleMath::Vector3 color;
	int bounces = 0;
};

struct LaserBarrelComponent
{
	bool shoot = false;
	f32 ammo = 30.0f; // This value is time in seconds you can shoot.
	f32 damagePerSecond = 240.0f;
	LaserBeamComponent laserToShoot;
};

struct GlowStickComponent
{
	static constexpr u32 globalGlowStickLimit = 100;
	f32 spawnTime = 0;
};

struct GlowStickThrowerComponent
{
	bool waitForNewKeyDown = false;
};

struct DeferredSetIgnoreCollisionCheckComponent
{
	bool value = false;
	f32 countDown = 3.0f;
	DOG::entity other = DOG::NULL_ENTITY;
};