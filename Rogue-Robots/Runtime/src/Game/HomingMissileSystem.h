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
		if (missile.launched && missile.fuel > 0)
		{
			float ve = DOG::PhysicsEngine::standardGravity * missile.specificImpules;
			float force = missile.fuelConsumtion * ve;
			rigidBody.centralForce = force * -transform.worldMatrix.Forward();
			
			float fuelBurnt = std::min(missile.fuel, missile.fuelConsumtion * DOG::Time::DeltaTime<DOG::TimeType::Seconds, f32>());
			missile.fuel -= fuelBurnt;
			rigidBody.mass -= fuelBurnt;
		}
	}
};