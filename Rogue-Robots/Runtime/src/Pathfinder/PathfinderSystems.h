#pragma once
#include <DOGEngine.h>
#include "../Game/GameComponent.h"

class PathfinderWalkSystem : public DOG::ISystem
{
	using Vector3 = DirectX::SimpleMath::Vector3;
	using Matrix = DirectX::SimpleMath::Matrix;
public:
	SYSTEM_CLASS(PathfinderWalkComponent, DOG::TransformComponent);
	ON_UPDATE(PathfinderWalkComponent, DOG::TransformComponent);
	void OnUpdate(PathfinderWalkComponent& pfc, DOG::TransformComponent& trans);
};

class VisualizePathCleanUpSystem : public DOG::ISystem
{
	using Vector3 = DirectX::SimpleMath::Vector3;
	using Matrix = DirectX::SimpleMath::Matrix;
public:
	SYSTEM_CLASS(LaserBeamVFXComponent, VisualizePathComponent);
	ON_UPDATE_ID(LaserBeamVFXComponent, VisualizePathComponent);
	void OnUpdate(DOG::entity e, LaserBeamVFXComponent&, VisualizePathComponent&);
};
