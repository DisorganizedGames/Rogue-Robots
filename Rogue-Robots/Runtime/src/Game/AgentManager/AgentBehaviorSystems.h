#pragma once
#include <DOGEngine.h>
#include "Game/GameComponent.h"
#include "BehaviorTree.h"

/**************************************************
*			Early Update Systems
***************************************************/

class AgentBehaviorTreeSystem : public DOG::ISystem
{
public:
	SYSTEM_CLASS(AgentIdComponent, BehaviorTreeComponent);
	ON_EARLY_UPDATE_ID(AgentIdComponent, BehaviorTreeComponent);
	void OnEarlyUpdate(DOG::entity agent, AgentIdComponent&, BehaviorTreeComponent& btc);
};

class AgentDetectPlayerSystem: public DOG::ISystem
{
	using Vector3 = DirectX::SimpleMath::Vector3;
	using Matrix = DirectX::SimpleMath::Matrix;
public:
	SYSTEM_CLASS(BTDetectPlayerComponent, AgentSeekPlayerComponent, AgentIdComponent, DOG::TransformComponent);
	ON_EARLY_UPDATE_ID(BTDetectPlayerComponent, AgentSeekPlayerComponent, AgentIdComponent, DOG::TransformComponent);
	void OnEarlyUpdate(DOG::entity e, BTDetectPlayerComponent&, AgentSeekPlayerComponent& seek, AgentIdComponent& agent, DOG::TransformComponent& transform);
};


/**************************************************
*				Regular Systems
***************************************************/

class AgentAttackSystem : public DOG::ISystem
{
	using Vector3 = DirectX::SimpleMath::Vector3;
	using Matrix = DirectX::SimpleMath::Matrix;
public:
	SYSTEM_CLASS(BTAttackComponent, BehaviorTreeComponent, AgentAttackComponent, AgentSeekPlayerComponent);
	ON_UPDATE_ID(BTAttackComponent, BehaviorTreeComponent, AgentAttackComponent, AgentSeekPlayerComponent);
	void OnUpdate(DOG::entity e, BTAttackComponent&, BehaviorTreeComponent& btc, 
		AgentAttackComponent& attack, AgentSeekPlayerComponent& seek);
};

class AgentAggroSystem : public DOG::ISystem
{
	using Vector3 = DirectX::SimpleMath::Vector3;
	using Matrix = DirectX::SimpleMath::Matrix;
public:
	SYSTEM_CLASS(BTAggroComponent, AgentAggroComponent, AgentIdComponent);
	ON_UPDATE_ID(BTAggroComponent, AgentAggroComponent, AgentIdComponent);
	void OnUpdate(DOG::entity e, BTAggroComponent&, AgentAggroComponent& aggro, AgentIdComponent& agent);
};

class AgentHitDetectionSystem : public DOG::ISystem
{
	using Vector3 = DirectX::SimpleMath::Vector3;
	using Matrix = DirectX::SimpleMath::Matrix;
public:
	SYSTEM_CLASS(/*BTHitDetectComponent, */DOG::HasEnteredCollisionComponent, AgentSeekPlayerComponent);
	ON_UPDATE_ID(/*BTHitDetectComponent, */DOG::HasEnteredCollisionComponent, AgentSeekPlayerComponent);
	void OnUpdate(DOG::entity me, /*BTHitDetectComponent&,*/ DOG::HasEnteredCollisionComponent& collision, AgentSeekPlayerComponent& seek);
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
	SYSTEM_CLASS(AgentHPComponent, DOG::TransformComponent);
	ON_UPDATE_ID(AgentHPComponent, DOG::TransformComponent);
	void OnUpdate(DOG::entity e, AgentHPComponent& hp, DOG::TransformComponent& trans);
};

class AgentFrostTimerSystem : public DOG::ISystem
{
public:
	SYSTEM_CLASS(AgentMovementComponent, FrostEffectComponent);
	ON_UPDATE_ID(AgentMovementComponent, FrostEffectComponent);
	void OnUpdate(DOG::entity e, AgentMovementComponent& movement, FrostEffectComponent& frostEffect);
};

class AgentFireTimerSystem : public DOG::ISystem
{
public:
	SYSTEM_CLASS(AgentHPComponent, FireEffectComponent);
	ON_UPDATE_ID(AgentHPComponent, FireEffectComponent);
	void OnUpdate(DOG::entity e, AgentHPComponent& hpComponent, FireEffectComponent& fireEffect);
};


/**************************************************
*			Late Update Systems
***************************************************/


class AgentMovementSystem : public DOG::ISystem
{
	using Vector3 = DirectX::SimpleMath::Vector3;
	using Matrix = DirectX::SimpleMath::Matrix;
public:
	SYSTEM_CLASS(BTMoveToPlayerComponent, BehaviorTreeComponent, AgentMovementComponent, PathfinderWalkComponent, DOG::RigidbodyComponent, DOG::TransformComponent);
	ON_LATE_UPDATE_ID(BTMoveToPlayerComponent, BehaviorTreeComponent, AgentMovementComponent, PathfinderWalkComponent, DOG::RigidbodyComponent, DOG::TransformComponent);
	void OnLateUpdate(DOG::entity e, BTMoveToPlayerComponent&, BehaviorTreeComponent& btc, 
		AgentMovementComponent& movement, PathfinderWalkComponent& pfc,
		DOG::RigidbodyComponent& rb, DOG::TransformComponent& trans);
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


/***********************************************

			Late Update Agent Systems

***********************************************/

class LateAgentDestructCleanupSystem : public DOG::ISystem
{
	using Vector3 = DirectX::SimpleMath::Vector3;
	using Matrix = DirectX::SimpleMath::Matrix;
	using DeferredDeletionComponent = DOG::DeferredDeletionComponent;
public:
	SYSTEM_CLASS(AgentIdComponent, DeferredDeletionComponent);
	ON_LATE_UPDATE(AgentIdComponent, DeferredDeletionComponent);
	void OnLateUpdate(AgentIdComponent& agent, DeferredDeletionComponent&);
};

