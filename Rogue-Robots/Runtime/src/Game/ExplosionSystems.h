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

class ExplosionEffectSystem : public DOG::ISystem
{
private:
	static u32 explosionEffectModelID;
public:
	//You do not have control over the entity! The system does!
	static DOG::entity CreateExplosionEffect(DOG::entity parentEntity, float radius, const DirectX::SimpleMath::Vector3& color = { 0.8f, 0.f, 0.f }, float growTime = -1.0f, float shrinkTime = -1.0f);

	SYSTEM_CLASS(ExplosionEffectComponent);
	ON_UPDATE_ID(ExplosionEffectComponent);
	void OnUpdate(DOG::entity e, ExplosionEffectComponent& explosionInfo);
};