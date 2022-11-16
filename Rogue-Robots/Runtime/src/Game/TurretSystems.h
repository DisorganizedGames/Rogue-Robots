#pragma once
#include <DOGEngine.h>
#include "GameComponent.h"

struct TurretTargetingComponent
{
	float maxRange = 100.0;
	DOG::entity trackedTarget = DOG::NULL_ENTITY;

	float yawSpeed = 1.0f;
	float pitchSpeed = 0.6f;

	float yawLimit = DirectX::XM_PI / 2;
	float pitchLimit = DirectX::XM_PI / 12;

	bool shoot = false;
};

struct TurretAmmoComponent
{
	u32 ammoCount = 500;
	f32 projectileSpeed = 100;
	f64 timeStep = 0.2f;
	f64 lastDischargeTimer = timeStep;
};

class TurretTargetingSystem : public DOG::ISystem
{
public:
	SYSTEM_CLASS(TurretTargetingComponent, ChildComponent, DOG::TransformComponent);
	ON_UPDATE(TurretTargetingComponent, ChildComponent, DOG::TransformComponent);
	void OnUpdate(TurretTargetingComponent& targeter, ChildComponent& localTransform, DOG::TransformComponent& globalTransform);
private:
};

class TurretShootingSystem : public DOG::ISystem
{
public:
	SYSTEM_CLASS(TurretTargetingComponent, TurretAmmoComponent, DOG::TransformComponent);
	ON_UPDATE_ID(TurretTargetingComponent, TurretAmmoComponent, DOG::TransformComponent);
	void OnUpdate(DOG::entity e, TurretTargetingComponent& targeter, TurretAmmoComponent& ammo, DOG::TransformComponent& transform);
private:
};