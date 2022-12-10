#include "ExplosionSystems.h"

#include "../DOGEngine/src/Core/CustomMaterialManager.h"

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

u32 ExplosionEffectSystem::s_explosionEffectModelID = 0;
u32 ExplosionEffectSystem::s_explosionAudioID[static_cast<size_t>(ExplosionEffectComponent::ExplosionSound::None)] = {};
const char* ExplosionEffectSystem::s_explosionEffectModelPath = "Assets/Models/Ammunition/Explosion.glb";
ExplosionEffectSystem::ExplosionEffectSystem()
{
	s_explosionEffectModelID = AssetManager::Get().LoadModelAsset(s_explosionEffectModelPath);
	s_explosionAudioID[static_cast<size_t>(ExplosionEffectComponent::ExplosionSound::Explosion1)] = AssetManager::Get().LoadAudio("Assets/Audio/GunSounds/Explosion 08a (grenade).wav");
	s_explosionAudioID[static_cast<size_t>(ExplosionEffectComponent::ExplosionSound::Explosion2)] = AssetManager::Get().LoadAudio("Assets/Audio/GunSounds/Explosion 03b.wav");
}

entity ExplosionEffectSystem::CreateExplosionEffect(entity parentEntity, float radius, float growTime, float shrinkTime, float audioVolume, ExplosionEffectComponent::ExplosionSound explosionSound)
{
	if (s_explosionEffectModelID == 0)
	{
		// This is a static function and if the constructor has not loaded the asset we will need to do it.
		s_explosionEffectModelID = AssetManager::Get().LoadModelAsset(s_explosionEffectModelPath);
	}

	entity newEntity = EntityManager::Get().CreateEntity();
	if (EntityManager::Get().HasComponent<SceneComponent>(parentEntity))
		EntityManager::Get().AddComponent<SceneComponent>(newEntity, EntityManager::Get().GetComponent<SceneComponent>(parentEntity).scene);

	Vector3 parentPosition = EntityManager::Get().GetComponent<TransformComponent>(parentEntity).GetPosition();

	EntityManager::Get().AddComponent<TransformComponent>(newEntity, parentPosition, Vector3(.0f, .0f, .0f), Vector3(radius, radius, radius));
	EntityManager::Get().AddComponent<ModelComponent>(newEntity, s_explosionEffectModelID);

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

	if (explosionSound != ExplosionEffectComponent::ExplosionSound::None)
	{
		entity audioEntity = EntityManager::Get().CreateEntity();
		if (auto scene = EntityManager::Get().TryGetComponent<SceneComponent>(parentEntity))
			EntityManager::Get().AddComponent<SceneComponent>(audioEntity, scene->get().scene);
		EntityManager::Get().AddComponent<TransformComponent>(audioEntity, parentPosition);
		auto& audio = EntityManager::Get().AddComponent<AudioComponent>(audioEntity);
		audio.assetID = s_explosionAudioID[static_cast<size_t>(explosionSound)];
		audio.is3D = true;
		audio.shouldPlay = true;
		audio.volume = audioVolume;
		EntityManager::Get().AddComponent<LifetimeComponent>(audioEntity, 5.0f); // Hope that the explosion sound is shorter then 5 seconds.
	}

	AddEffectsToExplosion(parentEntity, newEntity);

	gfx::PostProcess::Get().InstantiateShockWave(parentPosition, Vector3(10, 0.8f, 0.1f), 1);

	return newEntity;
}

void ExplosionEffectSystem::AddEffectsToExplosion(DOG::entity parentEntity, DOG::entity explosionEntity)
{
	std::string materialName = "";
	//Base explosion color
	Vector3 color = { 0.8f, 0.0f, 0.0f };

	if (EntityManager::Get().HasComponent<FrostEffectComponent>(parentEntity))
	{	
		materialName = "FrostExplosionMaterial";
	}
	else if (EntityManager::Get().HasComponent<FireEffectComponent>(parentEntity))
	{
		materialName = "FireExplosionMaterial";
	}
	else
	{
		materialName = "NormalExplosionMaterial";
	}

	if (!materialName.empty() && EntityManager::Get().HasComponent<ModelComponent>(explosionEntity))
	{
		LuaTable materialPrefabsTable = LuaMain::GetGlobal()->GetTable("MaterialPrefabs");
		LuaTable materialTable(materialPrefabsTable.CallFunctionOnTable("GetMaterial", materialName).table);

		ModelComponent& model = EntityManager::Get().GetComponent<ModelComponent>(explosionEntity);

		ModelAsset* modelAsset = AssetManager::Get().GetAsset<ModelAsset>(model.id);

		if (!modelAsset)
		{
			std::cout << "Model has not been loaded in yet! Model ID: " << model.id << "\n";
		}
		else
		{
			MaterialHandle materialHandle;
			materialHandle.handle = static_cast<u64>(materialTable.GetIntFromTable("materialHandle"));

			LuaTable albedoFactor = materialTable.GetTableFromTable("albedoFactor");

			color = { albedoFactor.GetFloatFromTable("x"), albedoFactor.GetFloatFromTable("y"), albedoFactor.GetFloatFromTable("z") };

			MaterialDesc materialDesc{};
			materialDesc.albedoFactor = { albedoFactor.GetFloatFromTable("x"), albedoFactor.GetFloatFromTable("y"), albedoFactor.GetFloatFromTable("z"), 1.0f };
			materialDesc.roughnessFactor = (float)materialTable.GetDoubleFromTable("roughnessFactor");
			materialDesc.metallicFactor = (float)materialTable.GetDoubleFromTable("metallicFactor");

			EntityManager::Get().AddComponent<SubmeshRenderer>(explosionEntity, modelAsset->gfxModel->mesh.mesh, materialHandle, materialDesc);
			EntityManager::Get().RemoveComponent<ModelComponent>(explosionEntity);
		}
	}

	// Add dynamic point light
	auto pdesc = PointLightDesc();
	pdesc.color = color;
	pdesc.strength = 100.f;
	pdesc.radius = 50.f;
	auto& plc = EntityManager::Get().AddComponent<PointLightComponent>(explosionEntity);
	plc.handle = LightManager::Get().AddPointLight(pdesc, LightUpdateFrequency::PerFrame);
	plc.color = pdesc.color;
	plc.strength = pdesc.strength;
	plc.radius = pdesc.radius;
}

void ExplosionEffectSystem::OnUpdate(DOG::entity e, ExplosionEffectComponent& explosionInfo)
{
	CreateExplosionEffect(e, explosionInfo.radius, explosionInfo.growTime, explosionInfo.shrinkTime, explosionInfo.audioVolume, explosionInfo.explosionSound);

	EntityManager::Get().RemoveComponent<ExplosionEffectComponent>(e);
}
