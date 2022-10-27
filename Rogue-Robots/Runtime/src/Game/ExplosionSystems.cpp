#include "ExplosionSystems.h"
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
entity ExplosionEffectSystem::CreateExplosionEffect(entity parentEntity, float radius, float growTime, float shrinkTime)
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

	// Add dynamic point light
	auto pdesc = PointLightDesc();
	pdesc.color = { 0.8f, 0.f, 0.f };
	pdesc.strength = 100.f;
	auto& plc = EntityManager::Get().AddComponent<PointLightComponent>(newEntity);
	plc.handle = LightManager::Get().AddPointLight(pdesc, LightUpdateFrequency::PerFrame);
	plc.color = pdesc.color;
	plc.strength = pdesc.strength;

	//Set values for explosions on the script
	LuaMain::GetScriptManager()->AddScript(newEntity, "ExplosionEffect.lua");
	if (growTime != -1.0f || shrinkTime != -1.0f)
	{
		ScriptData scriptData = LuaMain::GetScriptManager()->GetScript(newEntity, "ExplosionEffect.lua");
		LuaTable scriptTable(scriptData.scriptTable, true);


		if (growTime != -1.0f)
			scriptTable.AddFloatToTable("growTime", growTime);
		if (shrinkTime != -1.0f)
			scriptTable.AddFloatToTable("shrinkTime", shrinkTime);
	}

	return newEntity;
}

void ExplosionEffectSystem::OnUpdate(DOG::entity e, ExplosionEffectComponent& explosionInfo)
{
	CreateExplosionEffect(e, explosionInfo.radius, explosionInfo.growTime, explosionInfo.shrinkTime);

	EntityManager::Get().RemoveComponent<ExplosionEffectComponent>(e);
}
