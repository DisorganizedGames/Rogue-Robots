#pragma once
#include <DOGEngine.h>
#include "../Game/GameComponent.h"

class PathfinderWalkAgentsSystem : public DOG::ISystem
{
	using Vector3 = DirectX::SimpleMath::Vector3;
	using Matrix = DirectX::SimpleMath::Matrix;
public:
	SYSTEM_CLASS(PathfinderWalkComponent, AgentMovementComponent, DOG::RigidbodyComponent, DOG::TransformComponent);
	ON_LATE_UPDATE_ID(PathfinderWalkComponent, AgentMovementComponent, DOG::RigidbodyComponent, DOG::TransformComponent);
	void OnLateUpdate(DOG::entity e, PathfinderWalkComponent& pathfinder, 
		AgentMovementComponent& movement, DOG::RigidbodyComponent& rb, DOG::TransformComponent& trans);
};

class VisualizePathCleanUpSystem : public DOG::ISystem
{
	using Vector3 = DirectX::SimpleMath::Vector3;
	using Matrix = DirectX::SimpleMath::Matrix;
public:
	SYSTEM_CLASS(LaserBeamVFXComponent, VisualizePathComponent);
	ON_LATE_UPDATE_ID(LaserBeamVFXComponent, VisualizePathComponent);
	void OnLateUpdate(DOG::entity e, LaserBeamVFXComponent&, VisualizePathComponent&);
};
