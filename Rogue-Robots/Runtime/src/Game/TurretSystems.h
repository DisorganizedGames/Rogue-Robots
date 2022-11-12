#pragma once
#include <DOGEngine.h>
#include "GameComponent.h"

struct TurretTargetingComponent
{
	float maxRange = 100.0;
	DOG::entity trackedTarget = DOG::NULL_ENTITY;

	float yawSpeed = 1.0f;
	float pitchSpeed = 0.6f;
};

class TurretTargetingSystem : public DOG::ISystem
{
public:
	SYSTEM_CLASS(TurretTargetingComponent, ChildComponent, DOG::TransformComponent);
	ON_UPDATE(TurretTargetingComponent, ChildComponent, DOG::TransformComponent);
	void OnUpdate(TurretTargetingComponent& targeter, ChildComponent& localTransform, DOG::TransformComponent& globalTransform);
private:
};