#pragma once
#include <DOGEngine.h>
#include "GameComponent.h"

class HomingMissileSystem : public DOG::ISystem
{
	using Vector3 = DirectX::SimpleMath::Vector3;
public:
	SYSTEM_CLASS(HomingMissileComponent, DOG::TransformComponent, DOG::RigidbodyComponent);
	ON_UPDATE_ID(HomingMissileComponent, DOG::TransformComponent, DOG::RigidbodyComponent);
	void OnUpdate(DOG::entity e, HomingMissileComponent& missile, DOG::TransformComponent& transform, DOG::RigidbodyComponent& rigidBody);

	HomingMissileSystem();

private:
	void StartMissileEngine(DOG::entity e, HomingMissileComponent& missile) const;

	u32 m_smokeTexureAssetID{ 0 };
	u32 m_missileJetModelAssetID{ 0 };
};

class HomingMissileImpacteSystem : public DOG::ISystem
{
public:
	SYSTEM_CLASS(HomingMissileComponent, DOG::HasEnteredCollisionComponent, DOG::TransformComponent);
	ON_UPDATE_ID(HomingMissileComponent, DOG::HasEnteredCollisionComponent, DOG::TransformComponent);
	void OnUpdate(DOG::entity e, HomingMissileComponent& missile, DOG::HasEnteredCollisionComponent& collision, DOG::TransformComponent& transform);

	HomingMissileImpacteSystem();
	inline static bool s_useSmokeExplosion = true;
private:
	u32 m_smokeTexureAssetID{ 0 };
};

class HomingMissileTargetingSystem : public DOG::ISystem
{
public:
	SYSTEM_CLASS(HomingMissileComponent, DOG::TransformComponent);
	ON_UPDATE(HomingMissileComponent, DOG::TransformComponent);
	void OnUpdate(HomingMissileComponent& missile, DOG::TransformComponent& transform);
};