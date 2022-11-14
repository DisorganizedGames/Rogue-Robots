#include "TurretSystems.h"
#include "AgentManager/AgentComponents.h"

using namespace DOG;
using namespace DirectX;
using namespace DirectX::SimpleMath;

void TurretTargetingSystem::OnUpdate(TurretTargetingComponent& targeter, ChildComponent& localTransform, DOG::TransformComponent& globalTransform)
{
	auto& em = EntityManager::Get();
	targeter.trackedTarget = NULL_ENTITY;

	Vector3 fwd = localTransform.localTransform.GetForward();
	fwd.y = 0;
	fwd.Normalize();
	float yaw = acosf(fwd.z);
	if (fwd.x > 0) yaw = -yaw;

	Vector3 turretPosW = globalTransform.GetPosition();
	Vector3 targetDirL; // In turrets local space
	float maxDistSquared = targeter.maxRange * targeter.maxRange;
	auto&& findBetterTarger = [&](entity target, Vector3 targetPosW) {

		float d2 = SimpleMath::Vector3::DistanceSquared(turretPosW, targetPosW);
		if (d2 < maxDistSquared)
		{
			// If transformed to the turrets space, the target position == direction to target.
			targetDirL = Vector3::Transform(targetPosW, globalTransform.worldMatrix.Invert());
			Vector3 xzTargetDirL = targetDirL;
			xzTargetDirL.y = 0;
			xzTargetDirL.Normalize();
			float targetYawAngle = acosf(xzTargetDirL.z);
			if (xzTargetDirL.x > 0) targetYawAngle = -targetYawAngle;

			if (abs(yaw + targetYawAngle) <= targeter.yawLimit)
			{
				targetDirL.Normalize();
				maxDistSquared = d2;
				targeter.trackedTarget = target;
			}
		}
	};


	// Finds closest agent
	em.Collect<AgentAggroComponent, TransformComponent>().Do([&](entity agent, AgentAggroComponent&, TransformComponent& agentTransform)
		{
			findBetterTarger(agent, agentTransform.GetPosition());
		});

	// Turn turret against the target 
	if (targeter.trackedTarget != NULL_ENTITY)
	{
		f32 dt = Time::DeltaTime<TimeType::Seconds, f32>();
		float deltaYaw = targetDirL.x > 0 ? dt * targeter.yawSpeed : -dt * targeter.yawSpeed;
		if (targeter.yawLimit < XM_PI)
			deltaYaw = targetDirL.x > 0 ? deltaYaw = std::min(targeter.yawLimit + yaw, deltaYaw) : std::max(yaw - targeter.yawLimit, deltaYaw);

		localTransform.localTransform.RotateW({0, deltaYaw, 0});

		float pitch = asinf(localTransform.localTransform.GetForward().y);
		float deltaPitch = targetDirL.y > 0 ? std::max(pitch - targeter.pitchLimit, dt * -targeter.pitchSpeed) : std::min(pitch + targeter.pitchLimit, dt * targeter.pitchSpeed);
		localTransform.localTransform.RotateL({ deltaPitch, 0, 0 });
	}
}