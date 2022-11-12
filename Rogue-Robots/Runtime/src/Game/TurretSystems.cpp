#include "TurretSystems.h"
#include "AgentManager/AgentComponents.h"

using namespace DOG;
using namespace DirectX;
using namespace DirectX::SimpleMath;

void TurretTargetingSystem::OnUpdate(TurretTargetingComponent& targeter, ChildComponent& localTransform, DOG::TransformComponent& globalTransform)
{
	auto& em = EntityManager::Get();
	targeter.trackedTarget = NULL_ENTITY;

	// Fins closest agent
	float mxDistSquared = targeter.maxRange * targeter.maxRange;
	//em.Collect<AgentAggroComponent, TransformComponent>().Do([&](entity agent, AgentAggroComponent& agentAggro, TransformComponent& agentTransform)
	em.Collect<AgentHPComponent, TransformComponent>().Do([&](entity agent, AgentHPComponent& agentAggro, TransformComponent& agentTransform)
		{
			Vector3 agentPosW = agentTransform.GetPosition();
			Vector3 turretPosW = globalTransform.GetPosition();
			float d2 = SimpleMath::Vector3::DistanceSquared(turretPosW, agentPosW);
			if (d2 < mxDistSquared)
			{
				mxDistSquared = d2;
				targeter.trackedTarget = agent;
			}
		});


	// Turn turret against the target 
	//if (targeter.trackedTarget != NULL_ENTITY)
	{
		//Vector3 targetPosL = Vector3::Transform(em.GetComponent<TransformComponent>(targeter.trackedTarget).GetPosition(), globalTransform.worldMatrix.Invert());
		Vector3 targetPosL = Vector3::Transform(em.GetComponent<TransformComponent>(GetPlayer()).GetPosition(), globalTransform.worldMatrix.Invert());
		f32 dt = Time::DeltaTime<TimeType::Seconds, f32>();

		if (targetPosL.x > 0)
			localTransform.localTransform.RotateW({ 0, dt * targeter.yawSpeed, 0});
		else if (targetPosL.x < 0)
			localTransform.localTransform.RotateW({ 0, dt * -targeter.yawSpeed, 0 });

		if (targetPosL.y > 0)
			localTransform.localTransform.RotateL({ dt * -targeter.pitchSpeed , 0, 0 });
		else if (targetPosL.y < 0)
			localTransform.localTransform.RotateL({ dt * targeter.pitchSpeed, 0, 0 });
	}
}