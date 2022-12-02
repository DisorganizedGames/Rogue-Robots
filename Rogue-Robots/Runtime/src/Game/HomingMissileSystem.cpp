#include "HomingMissileSystem.h"
#include "AgentManager\AgentComponents.h"
using namespace DOG;
using namespace DirectX::SimpleMath;


void HomingMissileSystem::OnUpdate(entity e, HomingMissileComponent& missile, DOG::TransformComponent& transform, DOG::RigidbodyComponent& rigidBody)
{
	if (missile.launched && missile.flightTime < missile.lifeTime)
	{
		Vector3 forward = transform.GetForward();
		float dt = DOG::Time::DeltaTime<DOG::TimeType::Seconds, f32>();
		if (missile.hit)
		{
			if (missile.engineIsIgnited)
			{
				Vector3 target = DirectX::SimpleMath::Vector3::Transform(Vector3(-1, 1, -1), transform);
				target.Normalize();
				rigidBody.angularVelocity = 2 * missile.turnSpeed * target;
				rigidBody.linearVelocity = 0.8f * missile.mainMotorSpeed * forward;
			}
		}
		else if (DOG::EntityManager::Get().Exists(missile.homingTarget) && DOG::EntityManager::Get().HasComponent<DOG::TransformComponent>(missile.homingTarget))
		{
			if (missile.flightTime == 0.0f)
			{
				rigidBody.linearVelocity = missile.startMotorSpeed * forward;
			}
			else if (missile.flightTime < missile.engineStartTime)
			{
			}
			else if (missile.flightTime < missile.attackFlightPhaseStartTime)
			{
				if (!missile.engineIsIgnited)
				{
					StartMissileEngine(e, missile);
				}
				Vector3 targetDir = forward;
				targetDir.y = 0;
				targetDir.Normalize();
				targetDir.y = 1;
				targetDir.Normalize();
				Vector3 t = forward.Cross(targetDir);
				rigidBody.angularVelocity = missile.turnSpeed * t;
				rigidBody.linearVelocity = (missile.mainMotorSpeed / std::max(1.0f, missile.attackFlightPhaseStartTime - missile.flightTime)) * forward;
			}
			else
			{
				missile.armed = true;
				Vector3 target = DOG::EntityManager::Get().GetComponent<DOG::TransformComponent>(missile.homingTarget).GetPosition();
				Vector3 targetDir = target - transform.GetPosition();
				targetDir.Normalize();
				Vector3 t = forward.Cross(targetDir);
				rigidBody.angularVelocity = missile.turnSpeed * t;
				rigidBody.linearVelocity = missile.mainMotorSpeed * forward;
			}
		}
		else
		{
			if (!missile.engineIsIgnited && missile.flightTime > 0.8f * missile.engineStartTime)
			{
				StartMissileEngine(e, missile);
				missile.armed = true;
			}
			if (missile.engineIsIgnited)
			{
				rigidBody.linearVelocity = missile.mainMotorSpeed * forward;
			}
		}
		missile.flightTime += dt;

		if (missile.engineIsIgnited)
		{
			EntityManager::Get().GetComponent<PointLightComponent>(missile.jet).dirty = true;
		}
	}
	else if (missile.engineIsIgnited)
	{
		assert(EntityManager::Get().Exists(missile.jet));
		EntityManager::Get().DeferredEntityDestruction(missile.jet);
		missile.engineIsIgnited = false;
	}
}

HomingMissileSystem::HomingMissileSystem()
{
	// Load the dds textur if it exists
	AssetLoadFlag textureFlag = AssetLoadFlag::Srgb;
	textureFlag |= AssetLoadFlag::GPUMemory;
	if (std::filesystem::exists("Assets/Textures/Flipbook/smoke_4x4.dds"))
		m_smokeTexureAssetID = AssetManager::Get().LoadTexture("Assets/Textures/Flipbook/smoke_4x4.dds", textureFlag);
	else
		m_smokeTexureAssetID = AssetManager::Get().LoadTexture("Assets/Textures/Flipbook/smoke_4x4.png", textureFlag);

	m_missileJetModelAssetID = AssetManager::Get().LoadModelAsset("Assets/Models/Ammunition/Missile/jet.gltf");
}

void HomingMissileSystem::StartMissileEngine(entity e, HomingMissileComponent& missile) const
{
	auto& em = EntityManager::Get();

	missile.jet = em.CreateEntity();
	em.AddComponent<TransformComponent>(missile.jet);
	auto& localTransform = em.AddComponent<ChildComponent>(missile.jet);
	localTransform.parent = e;
	localTransform.localTransform.SetPosition({ 0,0, -0.7f }).SetRotation({ -DirectX::XM_PIDIV2, 0, 0 }).SetScale({ 1.3f, 1.3f, 1.3f });
	em.AddComponent<ModelComponent>(missile.jet, m_missileJetModelAssetID);
	missile.engineIsIgnited = true;


	LightHandle pointLight = LightManager::Get().AddPointLight(PointLightDesc(), LightUpdateFrequency::PerFrame);
	em.AddComponent<PointLightComponent>(missile.jet, pointLight, Vector3(1.0f, 0.6f, 0.1f), 30.f, 50.0f);

	entity jetParticleEmitter = em.CreateEntity();
	if (auto scene = em.TryGetComponent<SceneComponent>(e))
		em.AddComponent<SceneComponent>(jetParticleEmitter, scene->get().scene);

	em.AddComponent<TransformComponent>(jetParticleEmitter);
	auto& jtr = em.AddComponent<ChildComponent>(jetParticleEmitter);
	jtr.parent = missile.jet;
	jtr.localTransform.SetPosition({ 0, 0, 0 });

	em.AddComponent<ConeSpawnComponent>(jetParticleEmitter) = { .angle = DirectX::XM_PI / 6, .speed = 5.f };
	em.AddComponent<NoGravityBehaviorComponent>(jetParticleEmitter);
	em.AddComponent<ParticleEmitterComponent>(jetParticleEmitter) = {
		.spawnRate = 160,
		.particleSize = 0.08f,
		.particleLifetime = 0.09f,
		.startColor = {4, 1.2f, 0.4f, 1},
		.endColor = {2, 0.8f, 0.2f, 0}
	};

	entity smokeParticleEmitter = em.CreateEntity();
	if (auto scene = em.TryGetComponent<SceneComponent>(e))
		em.AddComponent<SceneComponent>(smokeParticleEmitter, scene->get().scene);

	em.AddComponent<TransformComponent>(smokeParticleEmitter);
	auto& str = em.AddComponent<ChildComponent>(smokeParticleEmitter);
	str.parent = missile.jet;
	str.localTransform.SetPosition({ 0, 0, 0 });

	auto texture = AssetManager::Get().GetAsset<TextureAsset>(m_smokeTexureAssetID);
	if (texture == nullptr) return;
	em.AddComponent<ConeSpawnComponent>(smokeParticleEmitter) = { .angle = DirectX::XM_PI / 8, .speed = 1.f };
	em.AddComponent<NoGravityBehaviorComponent>(smokeParticleEmitter);
	em.AddComponent<ParticleEmitterComponent>(smokeParticleEmitter) = {
		.spawnRate = 128,
		.particleSize = 0.25f,
		.particleLifetime = 1.2f,
		.textureHandle = texture->textureViewRawHandle,
		.textureSegmentsX = 4,
		.textureSegmentsY = 4,
		.startColor = {1,1,1,1},
		.endColor = {1, 1, 1, 0}
	};
}


void HomingMissileImpacteSystem::OnUpdate(entity e, HomingMissileComponent& missile, DOG::HasEnteredCollisionComponent& collision, DOG::TransformComponent& transform)
{
	auto& em = EntityManager::Get();
	if (missile.launched && collision.entitiesCount > 0)
	{
		// Instantly arm the missile if directly hit an enemy
		for (u32 i = 0; i < collision.entitiesCount; i++)
		{
			if (em.Exists(collision.entities[i]) && em.HasComponent<AgentIdComponent>(collision.entities[i]))
			{
				missile.armed = true;
				break;
			}
		}


		if (missile.armed || missile.hit > 3)
		{
			em.Collect<AgentIdComponent, DOG::TransformComponent>().Do([&](entity agent, AgentIdComponent&, DOG::TransformComponent& tr)
				{
					float distSquared = Vector3::DistanceSquared(transform.GetPosition(), tr.GetPosition());
					if (distSquared < missile.explosionRadius * missile.explosionRadius)
					{
						AgentHitComponent* hit;
						if (em.HasComponent<AgentHitComponent>(agent))
							hit = &em.GetComponent<AgentHitComponent>(agent);
						else
							hit = &em.AddComponent<AgentHitComponent>(agent);
						hit->HitBy({ e, missile.playerEntityID , missile.dmg / (1.0f + distSquared) });
					}
				});

			// Friendly fire
			em.Collect<PlayerStatsComponent, DOG::TransformComponent, PlayerAliveComponent, ThisPlayer>().Do([&](entity player, PlayerStatsComponent& stats, DOG::TransformComponent& playerTransform, PlayerAliveComponent&, ThisPlayer&)
				{
					float distSquared = Vector3::DistanceSquared(transform.GetPosition(), playerTransform.GetPosition());
					if (distSquared < missile.explosionRadius * missile.explosionRadius)
					{
						stats.health -= (missile.dmg / (1.0f + distSquared)) / 10.0f;
					}
				});

			em.AddComponent<ExplosionEffectComponent>(e, 0.8f * missile.explosionRadius);
			em.AddComponent<ExplosionComponent>(e, 2 * missile.explosionRadius, 0.8f * missile.explosionRadius);
			em.DeferredEntityDestruction(e);

			if (s_useSmokeExplosion)
			{
				entity topEmitter = em.CreateEntity();
				entity bottomEmitter = em.CreateEntity();
				if (auto scene = em.TryGetComponent<SceneComponent>(e))
				{
					em.AddComponent<SceneComponent>(topEmitter, scene->get().scene);
					em.AddComponent<SceneComponent>(bottomEmitter, scene->get().scene);
				}

				auto& tr = em.AddComponent<TransformComponent>(topEmitter, transform.GetPosition());
				em.AddComponent<TransformComponent>(bottomEmitter) = tr;


				em.AddComponent<LifetimeComponent>(topEmitter, 0.5f);
				em.AddComponent<LifetimeComponent>(bottomEmitter, 0.7f);


				auto texture = AssetManager::Get().GetAsset<TextureAsset>(m_smokeTexureAssetID);
				if (texture)
				{
					em.AddComponent<CylinderSpawnComponent>(topEmitter, 5, 5);
					em.AddComponent<GravityPointBehaviorComponent>(topEmitter, transform.GetPosition(), -4);
					em.AddComponent<ParticleEmitterComponent>(topEmitter) = {
						.spawnRate = 1200,
						.particleSize = 0.2f,
						.particleLifetime = 0.8f,
						.textureHandle = texture->textureViewRawHandle,
						.textureSegmentsX = 4,
						.textureSegmentsY = 4,
						.startColor = {1, 0.4f, 0.2f, 1},
						.endColor = {0.8, 0.3f, 0.1f, 0},
					};


					em.AddComponent<CylinderSpawnComponent>(bottomEmitter, 5, 1.2);
					em.AddComponent<GravityPointBehaviorComponent>(bottomEmitter, transform.GetPosition(), -70);
					em.AddComponent<ParticleEmitterComponent>(bottomEmitter) = {
						.spawnRate = 2000,
						.particleSize = 0.2f,
						.particleLifetime = 0.6f,
						.textureHandle = texture->textureViewRawHandle,
						.textureSegmentsX = 4,
						.textureSegmentsY = 4,
						.startColor = {0.8, 0.3f, 0.15f, 1},
						.endColor = {0.6, 0.2f, 0.1f, 0},
					};
				}
			}
		}
		else
		{
			missile.flightTime -= 1.5f; // Make the missile fly for a bit longer, just for fun.
		}
		missile.hit++;
	}
}

HomingMissileImpacteSystem::HomingMissileImpacteSystem()
{
	// Load the dds textur if it exists
	AssetLoadFlag textureFlag = AssetLoadFlag::Srgb;
	textureFlag |= AssetLoadFlag::GPUMemory;
	if (std::filesystem::exists("Assets/Textures/Flipbook/smoke_4x4.dds"))
		m_smokeTexureAssetID = AssetManager::Get().LoadTexture("Assets/Textures/Flipbook/smoke_4x4.dds", textureFlag);
	else
		m_smokeTexureAssetID = AssetManager::Get().LoadTexture("Assets/Textures/Flipbook/smoke_4x4.png", textureFlag);
}

void HomingMissileTargetingSystem::OnUpdate(HomingMissileComponent& missile, DOG::TransformComponent& transform)
{
	float minDistSquared = 1000000;
	entity target = NULL_ENTITY;
	if (!missile.homing)
		return;

	if (missile.homingTarget == NULL_ENTITY)
	{
		EntityManager::Get().Collect<AgentHPComponent, DOG::TransformComponent>().Do([&](entity e, AgentHPComponent&, DOG::TransformComponent& agentTransform)
			{
				float distSquared = Vector3::DistanceSquared(agentTransform.GetPosition(), transform.GetPosition());
				if (distSquared < minDistSquared)
				{
					if (auto hit = PhysicsEngine::RayCast(transform.GetPosition(), agentTransform.GetPosition()); hit && hit->entityHit == e)
					{
						minDistSquared = distSquared;
						target = e;
					}
				}
			});
		missile.homingTarget = target;
	}
}
