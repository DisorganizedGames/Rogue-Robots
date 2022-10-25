#pragma once
#include <DOGEngine.h>
#include "GameComponent.h"

class HomingMissileSystem : public DOG::ISystem
{
	using Vector3 = DirectX::SimpleMath::Vector3;
public:
	SYSTEM_CLASS(HomingMissileComponent, DOG::TransformComponent, DOG::RigidbodyComponent);
	ON_UPDATE(HomingMissileComponent, DOG::TransformComponent, DOG::RigidbodyComponent);
	void OnUpdate(HomingMissileComponent& missile, DOG::TransformComponent& transform, DOG::RigidbodyComponent& rigidBody)
	{
		if (missile.launched && missile.engineBurnTime > 0)
		{
			missile.engineBurnTime -= DOG::Time::DeltaTime<DOG::TimeType::Seconds, f32>();

			Vector3 forwad = -transform.worldMatrix.Forward();
			if (DOG::EntityManager::Get().Exists(missile.homingTarget) && DOG::EntityManager::Get().HasComponent<DOG::TransformComponent>(missile.homingTarget))
			{
				Vector3 target = DOG::EntityManager::Get().GetComponent<DOG::TransformComponent>(missile.homingTarget).GetPosition();
				Vector3 targetDir = target - transform.GetPosition();
				targetDir.Normalize();
				Vector3 t = forwad.Cross(targetDir);
				rigidBody.angularVelocity = missile.turnSpeed * t;
				rigidBody.linearVelocity = missile.speed * forwad;
			}
			else if (missile.homeInOnPosition)
			{
				Vector3 targetDir =  missile.targetPosition - transform.GetPosition();
				targetDir.Normalize();
				Vector3 t = forwad.Cross(targetDir);
				rigidBody.angularVelocity = missile.turnSpeed * t;
				rigidBody.linearVelocity = missile.speed * forwad;
			}
			else
			{
				rigidBody.linearVelocity = missile.speed * forwad;
			}
		}
	}
};