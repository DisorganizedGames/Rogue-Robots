#include "Scene.h"

using namespace DOG;

EntityManager& Scene::s_entityManager = EntityManager::Get();

Scene::Scene(SceneComponent::Type scene) : m_sceneType(scene) {}

Scene::~Scene()
{
	s_entityManager.Collect<SceneComponent>().Do([&](entity e, SceneComponent& sceneC)
		{
			if (sceneC.scene == m_sceneType)
			{
				s_entityManager.DeferredEntityDestruction(e);
			}
		});
}

entity Scene::CreateEntity() const noexcept
{
	entity e = s_entityManager.CreateEntity();
	s_entityManager.AddComponent<SceneComponent>(e, m_sceneType);
	return e;
}



void Scene::AddEntity(DOG::entity e) const noexcept
{
	assert(!s_entityManager.HasComponent<SceneComponent>(e));
	s_entityManager.AddComponent<SceneComponent>(e, m_sceneType);
}

void Scene::AddEntities(const std::vector<DOG::entity>& entities) const noexcept
{
	for (auto& e : entities)
		AddEntity(e);
}

SceneComponent::Type Scene::GetSceneType() const noexcept
{
	return m_sceneType;
}

void Scene::CreateTrampolinePickup(DirectX::SimpleMath::Vector3 position)
{
	static u32 trampolineNetworkID = 0u;
	u32 trampolineID = AssetManager::Get().LoadModelAsset("Assets/Models/Temporary_Assets/Trampoline.glb");

	entity trampolineEntity = CreateEntity();
	AddComponent<ActiveItemComponent>(trampolineEntity).type = ActiveItemComponent::Type::Trampoline;
	AddComponent<PickupComponent>(trampolineEntity).itemName = "Trampoline";
	AddComponent<ModelComponent>(trampolineEntity, trampolineID);
	AddComponent<TransformComponent>(trampolineEntity, position).SetScale({ 0.3f, 0.3f, 0.3f });
	AddComponent<ShadowReceiverComponent>(trampolineEntity);
	auto& ni = AddComponent<NetworkId>(trampolineEntity);
	ni.entityTypeId = EntityTypes::Trampoline;
	ni.id = trampolineNetworkID++;

	LuaMain::GetScriptManager()->AddScript(trampolineEntity, "Pickupable.lua");

	auto& lerpAnimator = AddComponent<PickupLerpAnimateComponent>(trampolineEntity);
	lerpAnimator.baseOrigin = s_entityManager.GetComponent<TransformComponent>(trampolineEntity).GetPosition().y;
	lerpAnimator.baseTarget = lerpAnimator.baseOrigin + 2.0f;
	lerpAnimator.currentOrigin = lerpAnimator.baseOrigin;
}

void Scene::CreateMissilePickup(DirectX::SimpleMath::Vector3 position)
{
	static u32 missileNetworkID = 0u;
	u32 missileID = AssetManager::Get().LoadModelAsset("Assets/Models/Ammunition/missile.glb");

	entity missileEntity = CreateEntity();
	auto& bc = AddComponent<BarrelComponent>(missileEntity);
	bc.type = BarrelComponent::Type::Missile;
	bc.maximumAmmoCapacityForType = 5;
	bc.ammoPerPickup = 1;
	AddComponent<PickupComponent>(missileEntity).itemName = "Homing missile";
	AddComponent<ModelComponent>(missileEntity, missileID);
	AddComponent<TransformComponent>(missileEntity, position).SetScale({ 0.8f, 0.8f, 0.8f });
	AddComponent<ShadowReceiverComponent>(missileEntity);
	auto& ni = AddComponent<NetworkId>(missileEntity);
	ni.entityTypeId = EntityTypes::MissileBarrel;
	ni.id = missileNetworkID++;

	LuaMain::GetScriptManager()->AddScript(missileEntity, "Pickupable.lua");

	auto& lerpAnimator = AddComponent<PickupLerpAnimateComponent>(missileEntity);
	lerpAnimator.baseOrigin = s_entityManager.GetComponent<TransformComponent>(missileEntity).GetPosition().y;
	lerpAnimator.baseTarget = lerpAnimator.baseOrigin + 2.0f;
	lerpAnimator.currentOrigin = lerpAnimator.baseOrigin;
}

void Scene::CreateGrenadePickup(DirectX::SimpleMath::Vector3 position)
{
	static u32 grenadeNetworkID = 0u;

	u32 grenadeID = AssetManager::Get().LoadModelAsset("Assets/Models/Ammunition/Grenade/Grenade.glb");

	entity grenadeEntity = CreateEntity();
	auto& bc = AddComponent<BarrelComponent>(grenadeEntity);
	bc.type = BarrelComponent::Type::Grenade;
	bc.maximumAmmoCapacityForType = 10;
	bc.ammoPerPickup = 2;
	AddComponent<PickupComponent>(grenadeEntity).itemName = "Grenade";
	AddComponent<ModelComponent>(grenadeEntity, grenadeID);
	AddComponent<TransformComponent>(grenadeEntity, position).SetScale({ 0.5f, 0.5f, 0.5f });
	AddComponent<ShadowReceiverComponent>(grenadeEntity);
	auto& ni = AddComponent<NetworkId>(grenadeEntity);
	ni.entityTypeId = EntityTypes::GrenadeBarrel;
	ni.id = grenadeNetworkID++;

	LuaMain::GetScriptManager()->AddScript(grenadeEntity, "Pickupable.lua");

	auto& lerpAnimator = AddComponent<PickupLerpAnimateComponent>(grenadeEntity);
	lerpAnimator.baseOrigin = s_entityManager.GetComponent<TransformComponent>(grenadeEntity).GetPosition().y;
	lerpAnimator.baseTarget = lerpAnimator.baseOrigin + 2.0f;
	lerpAnimator.currentOrigin = lerpAnimator.baseOrigin;
}

void Scene::CreateMaxHealthBoostPickup(DirectX::SimpleMath::Vector3 position)
{
	static u32 healtBoostNetworkdID = 0u;

	u32 healthBoostID = AssetManager::Get().LoadModelAsset("Assets/Models/Temporary_Assets/Medkit.glb");

	entity healthBoostEntity = CreateEntity();
	AddComponent<PassiveItemComponent>(healthBoostEntity).type = PassiveItemComponent::Type::MaxHealthBoost;
	AddComponent<PickupComponent>(healthBoostEntity).itemName = "Max HP boost";
	AddComponent<ModelComponent>(healthBoostEntity, healthBoostID);
	AddComponent<TransformComponent>(healthBoostEntity, position).SetScale({ 0.5f, 0.5f, 0.5f });
	AddComponent<ShadowReceiverComponent>(healthBoostEntity);
	auto& ni = AddComponent<NetworkId>(healthBoostEntity);
	ni.entityTypeId = EntityTypes::IncreaseMaxHp;
	ni.id = healtBoostNetworkdID++;

	LuaMain::GetScriptManager()->AddScript(healthBoostEntity, "Pickupable.lua");

	auto& lerpAnimator = AddComponent<PickupLerpAnimateComponent>(healthBoostEntity);
	lerpAnimator.baseOrigin = s_entityManager.GetComponent<TransformComponent>(healthBoostEntity).GetPosition().y;
	lerpAnimator.baseTarget = lerpAnimator.baseOrigin + 2.0f;
	lerpAnimator.currentOrigin = lerpAnimator.baseOrigin;
}

void Scene::CreateFrostModificationPickup(DirectX::SimpleMath::Vector3 position)
{
	static u32 frostModNetworkID = 0u;

	u32 frostModID = AssetManager::Get().LoadModelAsset("Assets/Models/Temporary_Assets/blue_cube.glb");

	entity frostModEntity = CreateEntity();
	AddComponent<MagazineModificationComponent>(frostModEntity).type = MagazineModificationComponent::Type::Frost;
	AddComponent<PickupComponent>(frostModEntity).itemName = "Frost modification";
	AddComponent<ModelComponent>(frostModEntity, frostModID);
	AddComponent<TransformComponent>(frostModEntity, position).SetScale({ 0.3f, 0.3f, 0.3f });
	AddComponent<ShadowReceiverComponent>(frostModEntity);
	auto& ni = AddComponent<NetworkId>(frostModEntity);
	ni.entityTypeId = EntityTypes::FrostMagazineModification;
	ni.id = frostModNetworkID++;

	LuaMain::GetScriptManager()->AddScript(frostModEntity, "Pickupable.lua");

	auto& lerpAnimator = AddComponent<PickupLerpAnimateComponent>(frostModEntity);
	lerpAnimator.baseOrigin = s_entityManager.GetComponent<TransformComponent>(frostModEntity).GetPosition().y;
	lerpAnimator.baseTarget = lerpAnimator.baseOrigin + 2.0f;
	lerpAnimator.currentOrigin = lerpAnimator.baseOrigin;
}