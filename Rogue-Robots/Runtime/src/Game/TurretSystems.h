#pragma once
#include <DOGEngine.h>
#include "GameComponent.h"



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
	SYSTEM_CLASS(TurretTargetingComponent, TurretBasicShootingComponent, DOG::TransformComponent);
	ON_UPDATE_ID(TurretTargetingComponent, TurretBasicShootingComponent, DOG::TransformComponent);
	void OnUpdate(DOG::entity e, TurretTargetingComponent& targeter, TurretBasicShootingComponent& ammo, DOG::TransformComponent& transform);
private:
};

class TurretProjectileSystem : public DOG::ISystem
{
public:
	SYSTEM_CLASS(TurretProjectileComponent, DOG::PointLightComponent);
	ON_UPDATE_ID(TurretProjectileComponent, DOG::PointLightComponent);
	void OnUpdate(DOG::entity e, TurretProjectileComponent& projectile, DOG::PointLightComponent& pointLight);
private:
};