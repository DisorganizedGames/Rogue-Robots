#pragma once
#include <DOGEngine.h>
#include "Game/GameComponent.h"
#include "BehaviorTree.h"
#include "AgentManager.h"

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

class AgentDistanceToPlayersSystem : public DOG::ISystem
{
public:
	SYSTEM_CLASS(BTDistanceToPlayerComponent, AgentTargetMetricsComponent, AgentIdComponent, DOG::TransformComponent, BehaviorTreeComponent);
	ON_EARLY_UPDATE_ID(BTDistanceToPlayerComponent, AgentTargetMetricsComponent, AgentIdComponent, DOG::TransformComponent, BehaviorTreeComponent);
	void OnEarlyUpdate(DOG::entity e, BTDistanceToPlayerComponent&, AgentTargetMetricsComponent& atmc, AgentIdComponent& aidc, DOG::TransformComponent& tc, BehaviorTreeComponent& btc);
};

class AgentLineOfSightToPlayerSystem : public DOG::ISystem
{
public:
	SYSTEM_CLASS(BTLineOfSightToPlayerComponent, AgentTargetMetricsComponent, AgentIdComponent, DOG::TransformComponent, BehaviorTreeComponent);
	ON_EARLY_UPDATE_ID(BTLineOfSightToPlayerComponent, AgentTargetMetricsComponent, AgentIdComponent, DOG::TransformComponent, BehaviorTreeComponent);
	void OnEarlyUpdate(DOG::entity e, BTLineOfSightToPlayerComponent&, AgentTargetMetricsComponent& atmc, AgentIdComponent& aidc, DOG::TransformComponent& tc, BehaviorTreeComponent& btc);
};

class AgentDetectPlayerSystem: public DOG::ISystem
{
	using Vector3 = DirectX::SimpleMath::Vector3;
	using Matrix = DirectX::SimpleMath::Matrix;
public:
	SYSTEM_CLASS(BTDetectPlayerComponent, AgentSeekPlayerComponent, AgentIdComponent, DOG::TransformComponent, AgentTargetMetricsComponent, BehaviorTreeComponent);
	ON_EARLY_UPDATE_ID(BTDetectPlayerComponent, AgentSeekPlayerComponent, AgentIdComponent, DOG::TransformComponent, AgentTargetMetricsComponent, BehaviorTreeComponent);
	void OnEarlyUpdate(DOG::entity e, BTDetectPlayerComponent&, AgentSeekPlayerComponent& seek, 
		AgentIdComponent& agent, DOG::TransformComponent& transform, AgentTargetMetricsComponent& atmc, BehaviorTreeComponent& btc);
	[[nodiscard]] const bool IsPotentialTarget(const AgentTargetMetricsComponent::PlayerData& playerData, const AgentManager::AgentStats& stats) noexcept;
};

class AgentDetectHitSystem : public DOG::ISystem
{
public:
	SYSTEM_CLASS(BTHitDetectComponent, AgentSeekPlayerComponent, AgentIdComponent, DOG::TransformComponent, AgentTargetMetricsComponent, BehaviorTreeComponent);
	ON_EARLY_UPDATE_ID(BTHitDetectComponent, AgentSeekPlayerComponent, AgentIdComponent, DOG::TransformComponent, AgentTargetMetricsComponent, BehaviorTreeComponent);
	void OnEarlyUpdate(DOG::entity agentID, BTHitDetectComponent&, AgentSeekPlayerComponent& seek,
		AgentIdComponent& agent, DOG::TransformComponent& transform, AgentTargetMetricsComponent& atmc, BehaviorTreeComponent& btc);
};

class AgentGetPathSystem : public DOG::ISystem
{
public:
	SYSTEM_CLASS(BTGetPathComponent, AgentSeekPlayerComponent, BehaviorTreeComponent);
	ON_EARLY_UPDATE_ID(BTGetPathComponent, AgentSeekPlayerComponent, BehaviorTreeComponent);
	void OnEarlyUpdate(DOG::entity e, BTGetPathComponent&, AgentSeekPlayerComponent& seek, BehaviorTreeComponent& btc);
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

class AgentJumpAtPlayerSystem : public DOG::ISystem
{
	using Vector3 = DirectX::SimpleMath::Vector3;
	using Matrix = DirectX::SimpleMath::Matrix;
public:
	SYSTEM_CLASS(BTJumpAtPlayerComponent, BehaviorTreeComponent, AgentAttackComponent, AgentSeekPlayerComponent);
	ON_UPDATE_ID(BTJumpAtPlayerComponent, BehaviorTreeComponent, AgentAttackComponent, AgentSeekPlayerComponent);
	void OnUpdate(DOG::entity e, BTJumpAtPlayerComponent&, BehaviorTreeComponent& btc,
		AgentAttackComponent& attack, AgentSeekPlayerComponent& seek);
};

class AgentPullBackSystem : public DOG::ISystem
{
	using Vector3 = DirectX::SimpleMath::Vector3;
	using Matrix = DirectX::SimpleMath::Matrix;
public:
	SYSTEM_CLASS(BTPullBackComponent, BehaviorTreeComponent, AgentAttackComponent, AgentSeekPlayerComponent);
	ON_UPDATE_ID(BTPullBackComponent, BehaviorTreeComponent, AgentAttackComponent, AgentSeekPlayerComponent);
	void OnUpdate(DOG::entity e, BTPullBackComponent&, BehaviorTreeComponent& btc,
		AgentAttackComponent& attack, AgentSeekPlayerComponent& seek);
};

class AgentDodgeSystem : public DOG::ISystem
{
	using Vector3 = DirectX::SimpleMath::Vector3;
	using Matrix = DirectX::SimpleMath::Matrix;
public:
	SYSTEM_CLASS(BTDodgeComponent, BehaviorTreeComponent, AgentAttackComponent, AgentSeekPlayerComponent);
	ON_UPDATE_ID(BTDodgeComponent, BehaviorTreeComponent, AgentAttackComponent, AgentSeekPlayerComponent);
	void OnUpdate(DOG::entity e, BTDodgeComponent&, BehaviorTreeComponent& btc,
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
	SYSTEM_CLASS(DOG::HasEnteredCollisionComponent, AgentSeekPlayerComponent);
	ON_UPDATE_ID(DOG::HasEnteredCollisionComponent, AgentSeekPlayerComponent);
	void OnUpdate(DOG::entity me, DOG::HasEnteredCollisionComponent& collision, AgentSeekPlayerComponent& seek);
};

class AgentHitSystem : public DOG::ISystem
{
	using Vector3 = DirectX::SimpleMath::Vector3;
	using Matrix = DirectX::SimpleMath::Matrix;

private:
	std::vector<u32> m_hitSounds;

public:
	AgentHitSystem();

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
	SYSTEM_CLASS(AgentMovementComponent, FrostEffectComponent, AgentIdComponent);
	ON_UPDATE_ID(AgentMovementComponent, FrostEffectComponent, AgentIdComponent);
	void OnUpdate(DOG::entity e, AgentMovementComponent& movement, FrostEffectComponent& frostEffect, AgentIdComponent& idc);
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
private:
	std::vector<u32> m_walkingSounds;

public:
	AgentMovementSystem();
	
	SYSTEM_CLASS(BTMoveToPlayerComponent, BehaviorTreeComponent, AgentMovementComponent, AgentSeekPlayerComponent, PathfinderWalkComponent, DOG::RigidbodyComponent, DOG::TransformComponent);
	ON_LATE_UPDATE_ID(BTMoveToPlayerComponent, BehaviorTreeComponent, AgentMovementComponent, AgentSeekPlayerComponent, PathfinderWalkComponent, DOG::RigidbodyComponent, DOG::TransformComponent);
	void OnLateUpdate(DOG::entity e, BTMoveToPlayerComponent&, BehaviorTreeComponent& btc, 
		AgentMovementComponent& movement, AgentSeekPlayerComponent& seek, PathfinderWalkComponent& pfc,
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

