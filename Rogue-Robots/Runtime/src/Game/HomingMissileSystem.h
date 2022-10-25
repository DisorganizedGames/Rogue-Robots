#pragma once
#include <DOGEngine.h>
#include "GameComponent.h"

class HomingMissileSystem : public DOG::ISystem
{
	using Vector3 = DirectX::SimpleMath::Vector3;
public:
	SYSTEM_CLASS(HomingMissileComponent, DOG::TransformComponent);
	ON_UPDATE(HomingMissileComponent, DOG::TransformComponent);
	void OnUpdate(HomingMissileComponent& missile, DOG::TransformComponent& transform)
	{

	}
};