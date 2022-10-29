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

class AgentAggroSystem : public DOG::ISystem
{
	using Vector3 = DirectX::SimpleMath::Vector3;
	using Matrix = DirectX::SimpleMath::Matrix;
public:
	SYSTEM_CLASS(AgentAggroComponent);
	ON_UPDATE_ID(AgentAggroComponent);
	void OnUpdate(DOG::entity e, AgentAggroComponent& aggro);
};

class AgentHitDetectionSystem : public DOG::ISystem
{
	using Vector3 = DirectX::SimpleMath::Vector3;
	using Matrix = DirectX::SimpleMath::Matrix;
public:
	SYSTEM_CLASS(DOG::HasEnteredCollisionComponent, AgentSeekPlayerComponent);
	ON_UPDATE_ID(DOG::HasEnteredCollisionComponent, AgentSeekPlayerComponent);
	void OnUpdate(DOG::entity me, DOG::HasEnteredCollisionComponent& collision, AgentSeekPlayerComponent& seek);
};

class AgentHitSystem : public DOG::ISystem
{
	using Vector3 = DirectX::SimpleMath::Vector3;
	using Matrix = DirectX::SimpleMath::Matrix;
public:
	SYSTEM_CLASS(AgentHitComponent, AgentHPComponent);
	ON_UPDATE_ID(AgentHitComponent, AgentHPComponent);
	void OnUpdate(DOG::entity e, AgentHitComponent& hit, AgentHPComponent& hp);
};

class AgentDestructSystem : public DOG::ISystem
{
	using Vector3 = DirectX::SimpleMath::Vector3;
	using Matrix = DirectX::SimpleMath::Matrix;
public:
	SYSTEM_CLASS(AgentHPComponent, AgentIdComponent, DOG::TransformComponent);
	ON_UPDATE_ID(AgentHPComponent, AgentIdComponent, DOG::TransformComponent);
	void OnUpdate(DOG::entity e, AgentHPComponent& hp, AgentIdComponent& agent, DOG::TransformComponent& trans);
};

class AgentFrostTimerSystem : public DOG::ISystem
{
public:
	SYSTEM_CLASS(AgentMovementComponent, FrostEffectComponent);
	ON_UPDATE_ID(AgentMovementComponent, FrostEffectComponent);
	void OnUpdate(DOG::entity e, AgentMovementComponent& movement, FrostEffectComponent& frostEffect);
};

/***********************************************

			Shadow Agent Systems

***********************************************/


class ShadowAgentSeekPlayerSystem : public DOG::ISystem
{
	using Vector3 = DirectX::SimpleMath::Vector3;
	using Matrix = DirectX::SimpleMath::Matrix;
public:
	SYSTEM_CLASS(ShadowAgentSeekPlayerComponent);
	ON_UPDATE(ShadowAgentSeekPlayerComponent);
	void OnUpdate(ShadowAgentSeekPlayerComponent& seek);
};

