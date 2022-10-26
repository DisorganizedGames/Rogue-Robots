#pragma once
#include <DOGEngine.h>
#include "GameComponent.h"

class ExplosionSystem : public DOG::ISystem
{
public:
	SYSTEM_CLASS(DOG::TransformComponent, ExplosionComponent);
	ON_UPDATE_ID(DOG::TransformComponent, ExplosionComponent);
	void OnUpdate(DOG::entity e, DOG::TransformComponent& explosionTransform, ExplosionComponent& explosionInfo);
};

class ExplosionEffectSystem
{
private:
	static u32 explosionEffectModelID;
public:
	//You do not have control over the entity! The script does!
	static DOG::entity CreateExplosionEffect(DOG::entity parentEntity, float radius);
};