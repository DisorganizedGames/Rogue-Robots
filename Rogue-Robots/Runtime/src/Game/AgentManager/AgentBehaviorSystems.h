#pragma once
#include <DOGEngine.h>
#include "../GameComponent.h"

#include "DirectXMath.h"
#include <DirectXTK/SimpleMath.h>

class AgentSeekPlayerSystem: public DOG::ISystem
{
	using Vector3 = DirectX::SimpleMath::Vector3;
	using Matrix = DirectX::SimpleMath::Matrix;
public:
	SYSTEM_CLASS(AgentSeekPlayerComponent, AgentIdComponent, DOG::TransformComponent);
	ON_UPDATE_ID(AgentSeekPlayerComponent, AgentIdComponent, DOG::TransformComponent);
	void OnUpdate(DOG::entity e, AgentSeekPlayerComponent& seek, AgentIdComponent& agent, DOG::TransformComponent& transform);
};

class AgentMovementSystem : public DOG::ISystem
{
	using Vector3 = DirectX::SimpleMath::Vector3;
	using Matrix = DirectX::SimpleMath::Matrix;
public:
	SYSTEM_CLASS(AgentMovementComponent, AgentPathfinderComponent, AgentSeekPlayerComponent, DOG::TransformComponent);
	ON_UPDATE_ID(AgentMovementComponent, AgentPathfinderComponent, AgentSeekPlayerComponent, DOG::TransformComponent);
	void OnUpdate(DOG::entity e, AgentMovementComponent& movement, AgentPathfinderComponent& pathfinder, 
		AgentSeekPlayerComponent& seek, DOG::TransformComponent& trans);
};

class AgentAttackSystem : public DOG::ISystem
{
	using Vector3 = DirectX::SimpleMath::Vector3;
	using Matrix = DirectX::SimpleMath::Matrix;
public:
	SYSTEM_CLASS(AgentAttackComponent, AgentSeekPlayerComponent);
	ON_UPDATE_ID(AgentAttackComponent, AgentSeekPlayerComponent);
	void OnUpdate(DOG::entity e, AgentAttackComponent& attack, AgentSeekPlayerComponent& seek);
};
