#include "TurretSystems.h"
#include "AgentManager/AgentComponents.h"
#include "PrefabInstantiatorFunctions.h"

using namespace DOG;
using namespace DirectX;
using namespace DirectX::SimpleMath;

void TurretTargetingSystem::OnUpdate(TurretTargetingComponent& targeter, ChildComponent& localTransform, DOG::TransformComponent& globalTransform)
{
	auto& em = EntityManager::Get();
	targeter.trackedTarget = NULL_ENTITY;

	Vector3 xzFwdL = localTransform.localTransform.GetForward();
	xzFwdL.y = 0;
	xzFwdL.Normalize();
	float yaw = acosf(xzFwdL.z);
	if (xzFwdL.x > 0) yaw = -yaw;

	Vector3 turretPosW = globalTransform.GetPosition();
	Vector3 targetDirL; // In turrets local space
	float maxDistSquared = targeter.maxRange * targeter.maxRange;
	auto&& findBetterTarget = [&](entity target, Vector3 targetPosW) {

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

				// Detect if there is a wall or something in the way
				if (auto hit = PhysicsEngine::RayCast(turretPosW, targetPosW); hit && em.Exists(hit->entityHit))
				{
					if (SimpleMath::Vector3::DistanceSquared(turretPosW, hit->hitPosition) < d2)
					{
						if (!em.HasComponent<AgentAggroComponent>(hit->entityHit)) return;
					}
				}

				targetDirL.Normalize();
				maxDistSquared = d2;
				targeter.trackedTarget = target;
			}
		}
	};


	// Finds closest agent
	em.Collect<AgentAggroComponent, TransformComponent>().Do([&](entity agent, AgentAggroComponent&, TransformComponent& agentTransform)
		{
			findBetterTarget(agent, agentTransform.GetPosition());
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

		// If turret aims roughly at the target then set shoot to true.
		targeter.shoot = targetDirL.z > 0.95f;
	}
}




TurretShootingSystem::TurretShootingSystem()
{
	m_turretShootSound = AssetManager::Get().LoadAudio("Assets/Audio/Items/TurretShoot.wav");
}

void TurretShootingSystem::OnUpdate(entity e, TurretTargetingComponent& targeter, TurretBasicShootingComponent& turretShooter, DOG::TransformComponent& transform)
{
	f64 dt = Time::DeltaTime();
	if (targeter.shoot && turretShooter.ammoCount > 0)
	{
		while (turretShooter.lastDischargeTimer > turretShooter.timeStep)
		{
			AudioComponent& audio = EntityManager::Get().GetComponent<AudioComponent>(e);
			if (!audio.playing)
			{
				audio.assetID = m_turretShootSound;
				audio.shouldPlay = true;
			}

			SpawnTurretProjectile(transform, turretShooter.projectileSpeed, turretShooter.damage, turretShooter.projectileLifeTime, e, turretShooter.owningPlayer);
			turretShooter.lastDischargeTimer -= turretShooter.timeStep;
			if (--turretShooter.ammoCount <= 0) break;
		}
		turretShooter.lastDischargeTimer += dt;
	}
	targeter.shoot = false;
}

// Thanks unity src code
float MoveTowards(float current, float target, float maxDelta)
{
	return abs(target - current) <= maxDelta ? target : current + std::copysignf(maxDelta, target - current);
}

void TurretProjectileSystem::OnUpdate(DOG::entity e, TurretProjectileComponent& projectile, DOG::PointLightComponent& pointLight)
{
	f32 dt = static_cast<f32>(Time::DeltaTime());
	projectile.lifeTime += dt;
	if (projectile.lifeTime < projectile.maxLifeTime)
	{
		pointLight.strength = MoveTowards(pointLight.strength, 0, dt);
		pointLight.radius = MoveTowards(pointLight.radius, 0, dt);
		pointLight.dirty = true;
	}
	else
	{
		EntityManager::Get().DeferredEntityDestruction(e);
	}
}

void TurretProjectileHitSystem::OnUpdate(DOG::entity e, TurretProjectileComponent&, BulletComponent& bullet, DOG::TransformComponent& transform, DOG::HasEnteredCollisionComponent&)
{
	auto& em = EntityManager::Get();
	// Create a particle emitter for bullet hit effect
	auto bulletPos = transform.GetPosition();
	auto& bulletScene = em.GetComponent<SceneComponent>(e);

	auto playerPosition = em.GetComponent<TransformComponent>(bullet.playerEntityID).GetPosition();
	auto towardPlayer = (bulletPos - playerPosition);
	towardPlayer.Normalize();

	entity hitParticleEffect = em.CreateEntity();
	em.AddComponent<SceneComponent>(hitParticleEffect, bulletScene.scene);
	em.AddComponent<TransformComponent>(hitParticleEffect, bulletPos)
		.RotateForwardTo(towardPlayer)
		.RotateL(Vector3(DirectX::XM_PIDIV2, 0, 0));

	em.AddComponent<LifetimeComponent>(hitParticleEffect, 0.01f);
	em.AddComponent<ParticleEmitterComponent>(hitParticleEffect) = {
		.spawnRate = 64.f,
		.particleSize = 0.08f,
		.particleLifetime = 0.5f,
		.startColor = 4.0f * DirectX::SimpleMath::Vector4(0.8f, 0.5f, 0.001f, 1),
		.endColor = 2.0f * DirectX::SimpleMath::Vector4(0.5f, 0.8f, 0.2f, 1),
	};
	em.AddComponent<ConeSpawnComponent>(hitParticleEffect) = { .angle = DirectX::XM_PI / 3, .speed = 4.f };
}
