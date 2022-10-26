#include "ExplosionSystems.h"

using Vector3 = DirectX::SimpleMath::Vector3;
using namespace DOG;
void ExplosionSystem::OnUpdate(entity e, TransformComponent& explosionTransform, ExplosionComponent& explosionInfo)
{
	Vector3 explosionPosition = explosionTransform.GetPosition();

	float power = explosionInfo.power;
	float radius = explosionInfo.radius;

	EntityManager::Get().Collect<TransformComponent, RigidbodyComponent>().Do([&](TransformComponent& transform, RigidbodyComponent& rigidbody)
		{
			Vector3 position = transform.GetPosition();
			float distance = Vector3::Distance(position, explosionPosition);
			if (distance > radius)
				return;

			//float squaredDistance = Vector3::DistanceSquared(position, explosionPosition);
			//if (squaredDistance < 1.0f)
			//	squaredDistance = 1.0f;
			//power /= squaredDistance;

			Vector3 direction = (position - explosionPosition);
			direction.Normalize();
			rigidbody.centralImpulse = direction * rigidbody.mass * power;
		});

	EntityManager::Get().RemoveComponent<ExplosionComponent>(e);
}

u32 ExplosionEffectSystem::explosionEffectModelID = 0; 
entity ExplosionEffectSystem::CreateExplosionEffect(entity parentEntity, float radius)
{
	if (explosionEffectModelID == 0)
	{
		constexpr const char* modelName = "Assets/Models/Temporary_Assets/Explosion.glb";
		explosionEffectModelID = AssetManager::Get().LoadModelAsset(modelName);
	}

	entity newEntity = EntityManager::Get().CreateEntity();
	if (EntityManager::Get().HasComponent<SceneComponent>(parentEntity))
		EntityManager::Get().AddComponent<SceneComponent>(newEntity, EntityManager::Get().GetComponent<SceneComponent>(parentEntity).scene);

	Vector3 parentPosition = EntityManager::Get().GetComponent<TransformComponent>(parentEntity).GetPosition();

	EntityManager::Get().AddComponent<TransformComponent>(newEntity, parentPosition, Vector3(.0f, .0f, .0f), Vector3(radius, radius, radius));
	EntityManager::Get().AddComponent<ModelComponent>(newEntity, explosionEffectModelID);
	LuaMain::GetScriptManager()->AddScript(newEntity, "ExplosionEffect.lua");

	return newEntity;
}