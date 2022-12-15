#include "ItemManager.h"
#include "Game/GameLayer.h"
#include "../LoadSplitModels.h"
#include "../PrefabInstantiatorFunctions.h"

using namespace DOG;
using namespace DirectX::SimpleMath;

EntityManager& ItemManager::s_entityManager = EntityManager::Get();
ItemManager ItemManager::s_amInstance;
bool ItemManager::s_notInitialized = true;
u32 ItemManager::s_pickupId = 0u;

/*******************************
		Public Methods
*******************************/


u32 ItemManager::CreateItem(EntityTypes itemType, Vector3 position, u32 id)
{
	switch (itemType)
	{
	case EntityTypes::IncreaseMaxHp:
		return CreateMaxHealthBoostPickup(position, id);
		break;
	case EntityTypes::Trampoline:
		return CreateTrampolinePickup(position, id);
		break;
	case EntityTypes::Turret:
		return CreateTurretPickup(position, id);
	case EntityTypes::GrenadeBarrel:
		return CreateGrenadePickup(position, id);
		break;
	case EntityTypes::MissileBarrel:
		return CreateMissilePickup(position, id);
		break;
	case EntityTypes::LaserBarrel:
		return CreateLaserPickup(position, id);
		break;
	case EntityTypes::FrostMagazineModification:
		return CreateFrostModificationPickup(position, id);
		break;
	case EntityTypes::FireMagazineModification:
		return CreateFireModificationPickup(position, id);
		break;
	case EntityTypes::FullAutoMisc:
		return CreateFullAutoPickup(position, id);
		break;
	case EntityTypes::ChargeShotMisc:
		return CreateChargeShotPickup(position, id);
		break;
	case EntityTypes::IncreaseSpeed:
		return CreateSpeedBoostPickup(position, id);
		break;
	case EntityTypes::IncreaseSpeed2:
		return CreateSpeedBoost2Pickup(position, id);
		break;
	//case EntityTypes::Health:
		//return CreateHealthPickup(position, id);
		//break;
	case EntityTypes::JumpBoost:
		return CreateJumpBoost(position, id);
	//case EntityTypes::Reviver:
	//	return CreateReviverPickup(position, id);
	case EntityTypes::GoalRadar:
		return CreateGoalRadarPickup(position, id);
	case EntityTypes::Syringe:
		return CreateSyringePickup(position, id);
	default:
		break;
	}
	return 0;
}

void ItemManager::CreateItemHost(EntityTypes itemType, Vector3 position)
{
	u32 id = CreateItem(itemType, position);
	if (id > 0)
	{
		entity item = s_entityManager.CreateEntity();
		CreateAndDestroyEntityComponent& newItem = s_entityManager.AddComponent<CreateAndDestroyEntityComponent>(item);
		newItem.alive = true;
		newItem.entityTypeId = itemType;
		newItem.id = id;
		s_entityManager.Collect<ThisPlayer, NetworkPlayerComponent>().Do(
			[&](ThisPlayer&, NetworkPlayerComponent& net) { newItem.playerId = net.playerId; });
		newItem.position = position;
	}
}

void ItemManager::CreateItemClient(CreateAndDestroyEntityComponent cad)
{
	CreateItem(cad.entityTypeId, cad.position, cad.id);
}

void ItemManager::DestroyAllItems()
{
	EntityManager::Get().Collect<NetworkId>().Do([&](entity id, NetworkId&)
		{
			s_entityManager.RemoveComponent<NetworkId>(id);
			s_entityManager.DeferredEntityDestruction(id);
		});
	s_pickupId = 0u;
}

/*******************************
		Private Methods
*******************************/

ItemManager::ItemManager() noexcept
{

}


void ItemManager::Initialize()
{
	// Set status to initialized
	s_notInitialized = false;
	s_pickupId = 0u;
}


u32 ItemManager::CreateTrampolinePickup(Vector3 position, u32 id)
{
	u32 trampolineID = AssetManager::Get().LoadModelAsset("Assets/Models/Pickups/Trampoline.glb");

	entity trampolineEntity = s_entityManager.CreateEntity();
	s_entityManager.AddComponent<ActiveItemComponent>(trampolineEntity).type = ActiveItemComponent::Type::Trampoline;
	s_entityManager.AddComponent<PickupComponent>(trampolineEntity).itemName = "Trampoline";
	s_entityManager.AddComponent<ModelComponent>(trampolineEntity, trampolineID);
	s_entityManager.AddComponent<TransformComponent>(trampolineEntity, position).SetScale({ 0.3f, 0.3f, 0.3f });
	s_entityManager.AddComponent<ShadowReceiverComponent>(trampolineEntity);
	auto& ni = s_entityManager.AddComponent<NetworkId>(trampolineEntity);
	ni.entityTypeId = EntityTypes::Trampoline;
	if (id == 0)
		ni.id = ++s_pickupId;
	else
	{
		ni.id = id;
	}
	LuaMain::GetScriptManager()->AddScript(trampolineEntity, "Pickupable.lua");

	auto& lerpAnimator = s_entityManager.AddComponent<PickupLerpAnimateComponent>(trampolineEntity);
	lerpAnimator.baseOrigin = s_entityManager.GetComponent<TransformComponent>(trampolineEntity).GetPosition().y;
	lerpAnimator.baseTarget = lerpAnimator.baseOrigin + 2.0f;
	lerpAnimator.currentOrigin = lerpAnimator.baseOrigin;
	return ni.id;
}

u32 ItemManager::CreateMissilePickup(DirectX::SimpleMath::Vector3 position,  u32 id)
{
	u32 missileID = AssetManager::Get().LoadModelAsset("Assets/Models/Ammunition/Missile/missile.glb");

	entity missileEntity = s_entityManager.CreateEntity();
	auto& bc = s_entityManager.AddComponent<BarrelComponent>(missileEntity);
	bc.type = BarrelComponent::Type::Missile;
	bc.maximumAmmoCapacityForType = 10;
	bc.ammoPerPickup = 1;
	s_entityManager.AddComponent<PickupComponent>(missileEntity, PickupComponent::Type::BarrelItem).itemName = "Homing missile";
	s_entityManager.AddComponent<ModelComponent>(missileEntity, missileID);
	s_entityManager.AddComponent<TransformComponent>(missileEntity, position).SetScale({ 0.8f, 0.8f, 0.8f });
	s_entityManager.AddComponent<ShadowReceiverComponent>(missileEntity);
	auto& ni = s_entityManager.AddComponent<NetworkId>(missileEntity);
	ni.entityTypeId = EntityTypes::MissileBarrel;
	if (id == 0)
		ni.id = ++s_pickupId;
	else
	{
		ni.id = id;
	}
	LuaMain::GetScriptManager()->AddScript(missileEntity, "Pickupable.lua");

	auto& lerpAnimator = s_entityManager.AddComponent<PickupLerpAnimateComponent>(missileEntity);
	lerpAnimator.baseOrigin = s_entityManager.GetComponent<TransformComponent>(missileEntity).GetPosition().y;
	lerpAnimator.baseTarget = lerpAnimator.baseOrigin + 2.0f;
	lerpAnimator.currentOrigin = lerpAnimator.baseOrigin;
	return ni.id;
}

u32 ItemManager::CreateLaserPickup(Vector3 position, u32 id)
{

	entity laserEntity = SpawnLaserBlob(TransformComponent(position, Vector3::Zero, Vector3(0.5f, 0.5f, 0.5f)), NULL_ENTITY);
	auto& bc = s_entityManager.AddComponent<BarrelComponent>(laserEntity);
	bc.type = BarrelComponent::Type::Laser;
	bc.maximumAmmoCapacityForType = 80;
	bc.ammoPerPickup = 20;
	s_entityManager.AddComponent<PickupComponent>(laserEntity, PickupComponent::Type::BarrelItem).itemName = "Laser";
	auto& ni = s_entityManager.AddComponent<NetworkId>(laserEntity);
	ni.entityTypeId = EntityTypes::LaserBarrel;
	if (id == 0)
		ni.id = ++s_pickupId;
	else
	{
		ni.id = id;
	}

	LuaMain::GetScriptManager()->AddScript(laserEntity, "Pickupable.lua");

	auto& lerpAnimator = s_entityManager.AddComponent<PickupLerpAnimateComponent>(laserEntity);
	lerpAnimator.baseOrigin = s_entityManager.GetComponent<TransformComponent>(laserEntity).GetPosition().y;
	lerpAnimator.baseTarget = lerpAnimator.baseOrigin + 2.0f;
	lerpAnimator.currentOrigin = lerpAnimator.baseOrigin;
	return ni.id;
}

u32 ItemManager::CreateGrenadePickup(DirectX::SimpleMath::Vector3 position, u32 id)
{

	u32 grenadeID = AssetManager::Get().LoadModelAsset("Assets/Models/Ammunition/Grenade/Grenade.glb");

	entity grenadeEntity = s_entityManager.CreateEntity();
	auto& bc = s_entityManager.AddComponent<BarrelComponent>(grenadeEntity);
	bc.type = BarrelComponent::Type::Grenade;
	bc.maximumAmmoCapacityForType = 20;
	bc.ammoPerPickup = 2;
	s_entityManager.AddComponent<PickupComponent>(grenadeEntity, PickupComponent::Type::BarrelItem).itemName = "Grenade";
	s_entityManager.AddComponent<ModelComponent>(grenadeEntity, grenadeID);
	s_entityManager.AddComponent<TransformComponent>(grenadeEntity, position).SetScale({ 0.5f, 0.5f, 0.5f });
	s_entityManager.AddComponent<ShadowReceiverComponent>(grenadeEntity);
	auto& ni = s_entityManager.AddComponent<NetworkId>(grenadeEntity);
	ni.entityTypeId = EntityTypes::GrenadeBarrel;
	if (id == 0)
		ni.id = ++s_pickupId;
	else
	{
		ni.id = id;
	}
	LuaMain::GetScriptManager()->AddScript(grenadeEntity, "Pickupable.lua");

	auto& lerpAnimator = s_entityManager.AddComponent<PickupLerpAnimateComponent>(grenadeEntity);
	lerpAnimator.baseOrigin = s_entityManager.GetComponent<TransformComponent>(grenadeEntity).GetPosition().y;
	lerpAnimator.baseTarget = lerpAnimator.baseOrigin + 2.0f;
	lerpAnimator.currentOrigin = lerpAnimator.baseOrigin;
	return ni.id;
}

u32 ItemManager::CreateMaxHealthBoostPickup(DirectX::SimpleMath::Vector3 position, u32 id )
{
	u32 healthBoostID = AssetManager::Get().LoadModelAsset("Assets/Models/Pickups/Medkit.glb");

	entity healthBoostEntity = s_entityManager.CreateEntity();
	s_entityManager.AddComponent<PassiveItemComponent>(healthBoostEntity).type = PassiveItemComponent::Type::MaxHealthBoost;
	s_entityManager.AddComponent<PickupComponent>(healthBoostEntity, PickupComponent::Type::PassiveItem).itemName = "Max HP boost";
	s_entityManager.AddComponent<ModelComponent>(healthBoostEntity, healthBoostID);
	s_entityManager.AddComponent<TransformComponent>(healthBoostEntity, position).SetScale({ 0.8f, 0.8f, 0.8f });
	s_entityManager.AddComponent<ShadowReceiverComponent>(healthBoostEntity);
	auto& ni = s_entityManager.AddComponent<NetworkId>(healthBoostEntity);
	ni.entityTypeId = EntityTypes::IncreaseMaxHp;
	if (id == 0)
		ni.id = ++s_pickupId;
	else
	{
		ni.id = id;
	}

	LuaMain::GetScriptManager()->AddScript(healthBoostEntity, "Pickupable.lua");

	auto& lerpAnimator = s_entityManager.AddComponent<PickupLerpAnimateComponent>(healthBoostEntity);
	lerpAnimator.baseOrigin = s_entityManager.GetComponent<TransformComponent>(healthBoostEntity).GetPosition().y;
	lerpAnimator.baseTarget = lerpAnimator.baseOrigin + 2.0f;
	lerpAnimator.currentOrigin = lerpAnimator.baseOrigin;
	return ni.id;
}

u32 ItemManager::CreateFrostModificationPickup(DirectX::SimpleMath::Vector3 position, u32 id)
{
	u32 frostModID = AssetManager::Get().LoadModelAsset("Assets/Models/ModularRifle/Frost.gltf");

	entity frostModEntity = s_entityManager.CreateEntity();
	s_entityManager.AddComponent<MagazineModificationComponent>(frostModEntity).type = MagazineModificationComponent::Type::Frost;
	s_entityManager.AddComponent<PickupComponent>(frostModEntity, PickupComponent::Type::MagazineModificationItem).itemName = "Frost modification";
	s_entityManager.AddComponent<ModelComponent>(frostModEntity, frostModID);
	s_entityManager.AddComponent<TransformComponent>(frostModEntity, position).SetScale({ 0.3f, 0.3f, 0.3f });
	s_entityManager.AddComponent<ShadowReceiverComponent>(frostModEntity);
	auto& ni = s_entityManager.AddComponent<NetworkId>(frostModEntity);
	ni.entityTypeId = EntityTypes::FrostMagazineModification;
	if (id == 0)
		ni.id = ++s_pickupId;
	else
	{
		ni.id = id;
	}

	LuaMain::GetScriptManager()->AddScript(frostModEntity, "Pickupable.lua");

	auto& lerpAnimator = s_entityManager.AddComponent<PickupLerpAnimateComponent>(frostModEntity);
	lerpAnimator.baseOrigin = s_entityManager.GetComponent<TransformComponent>(frostModEntity).GetPosition().y;
	lerpAnimator.baseTarget = lerpAnimator.baseOrigin + 2.0f;
	lerpAnimator.currentOrigin = lerpAnimator.baseOrigin;
	return ni.id;
}

u32 ItemManager::CreateFireModificationPickup(DirectX::SimpleMath::Vector3 position, u32 id)
{
	u32 fireModID = AssetManager::Get().LoadModelAsset("Assets/Models/ModularRifle/Fire.gltf");

	entity fireModEntity = s_entityManager.CreateEntity();
	s_entityManager.AddComponent<MagazineModificationComponent>(fireModEntity).type = MagazineModificationComponent::Type::Frost;
	s_entityManager.AddComponent<PickupComponent>(fireModEntity, PickupComponent::Type::MagazineModificationItem).itemName = "Fire modification";
	s_entityManager.AddComponent<ModelComponent>(fireModEntity, fireModID);
	s_entityManager.AddComponent<TransformComponent>(fireModEntity, position).SetScale({ 0.3f, 0.3f, 0.3f });
	s_entityManager.AddComponent<ShadowReceiverComponent>(fireModEntity);
	auto& ni = s_entityManager.AddComponent<NetworkId>(fireModEntity);
	ni.entityTypeId = EntityTypes::FireMagazineModification;
	if (id == 0)
		ni.id = ++s_pickupId;
	else
	{
		ni.id = id;
	}

	LuaMain::GetScriptManager()->AddScript(fireModEntity, "Pickupable.lua");

	auto& lerpAnimator = s_entityManager.AddComponent<PickupLerpAnimateComponent>(fireModEntity);
	lerpAnimator.baseOrigin = s_entityManager.GetComponent<TransformComponent>(fireModEntity).GetPosition().y;
	lerpAnimator.baseTarget = lerpAnimator.baseOrigin + 2.0f;
	lerpAnimator.currentOrigin = lerpAnimator.baseOrigin;
	return ni.id;
}

u32 ItemManager::CreateTurretPickup(Vector3 position, u32 id)
{
	u32 turretBaseModelID = AssetManager::Get().LoadModelAsset("Assets/Models/Pickups/turretBase.glb");
	u32 turretHeadModelID = AssetManager::Get().LoadModelAsset("Assets/Models/Pickups/turret2.glb");

	entity turretPickUpEntity = s_entityManager.CreateEntity();
	s_entityManager.AddComponent<ActiveItemComponent>(turretPickUpEntity).type = ActiveItemComponent::Type::Turret;
	s_entityManager.AddComponent<PickupComponent>(turretPickUpEntity, PickupComponent::Type::ActiveItem).itemName = "Turret";
	s_entityManager.AddComponent<ModelComponent>(turretPickUpEntity, turretBaseModelID);
	s_entityManager.AddComponent<TransformComponent>(turretPickUpEntity, position).SetScale({ 0.4f, 0.4f, 0.4f });
	s_entityManager.AddComponent<ShadowReceiverComponent>(turretPickUpEntity);
	
	auto& ni = s_entityManager.AddComponent<NetworkId>(turretPickUpEntity);
	ni.entityTypeId = EntityTypes::Turret;
	if (id == 0)
		ni.id = ++s_pickupId;
	else
	{
		ni.id = id;
	}

	LuaMain::GetScriptManager()->AddScript(turretPickUpEntity, "Pickupable.lua");

	auto& lerpAnimator = s_entityManager.AddComponent<PickupLerpAnimateComponent>(turretPickUpEntity);
	lerpAnimator.baseOrigin = s_entityManager.GetComponent<TransformComponent>(turretPickUpEntity).GetPosition().y;
	lerpAnimator.baseTarget = lerpAnimator.baseOrigin + 2.0f;
	lerpAnimator.currentOrigin = lerpAnimator.baseOrigin;

	entity turretHeadpEntity = s_entityManager.CreateEntity();

	s_entityManager.AddComponent<OutlineBabyComponent>(turretPickUpEntity).child = turretHeadpEntity;

	s_entityManager.AddComponent<ModelComponent>(turretHeadpEntity, turretHeadModelID);
	s_entityManager.AddComponent<TransformComponent>(turretHeadpEntity);
	s_entityManager.AddComponent<ShadowReceiverComponent>(turretHeadpEntity);
	auto& node = s_entityManager.AddComponent<ChildComponent>(turretHeadpEntity);
	node.parent = turretPickUpEntity;
	node.localTransform.SetPosition({ 0, 1, 0 });
	return ni.id;
}

u32 ItemManager::CreateSpeedBoostPickup(DirectX::SimpleMath::Vector3 position, u32 id)
{
	u32 speedBoostID = AssetManager::Get().LoadModelAsset("Assets/Models/Pickups/speedPassive.glb");

	entity speedBoostEntity = s_entityManager.CreateEntity();
	s_entityManager.AddComponent<PassiveItemComponent>(speedBoostEntity).type = PassiveItemComponent::Type::SpeedBoost;
	s_entityManager.AddComponent<PickupComponent>(speedBoostEntity, PickupComponent::Type::PassiveItem).itemName = "Speed Boost";
	s_entityManager.AddComponent<ModelComponent>(speedBoostEntity, speedBoostID);
	s_entityManager.AddComponent<TransformComponent>(speedBoostEntity, position).SetScale({ 0.5f, 0.5f, 0.5f });
	s_entityManager.AddComponent<ShadowReceiverComponent>(speedBoostEntity);
	auto& ni = s_entityManager.AddComponent<NetworkId>(speedBoostEntity);
	ni.entityTypeId = EntityTypes::IncreaseSpeed;
	if (id == 0)
		ni.id = ++s_pickupId;
	else
	{
		ni.id = id;
	}

	LuaMain::GetScriptManager()->AddScript(speedBoostEntity, "Pickupable.lua");

	auto& lerpAnimator = s_entityManager.AddComponent<PickupLerpAnimateComponent>(speedBoostEntity);
	lerpAnimator.baseOrigin = s_entityManager.GetComponent<TransformComponent>(speedBoostEntity).GetPosition().y;
	lerpAnimator.baseTarget = lerpAnimator.baseOrigin + 2.0f;
	lerpAnimator.currentOrigin = lerpAnimator.baseOrigin;
	return ni.id;
}

u32 ItemManager::CreateSpeedBoost2Pickup(DirectX::SimpleMath::Vector3 position, u32 id)
{

	u32 speedBoostID = AssetManager::Get().LoadModelAsset("Assets/Models/Pickups/speedPassive2.glb");

	entity speedBoostEntity = s_entityManager.CreateEntity();
	s_entityManager.AddComponent<PassiveItemComponent>(speedBoostEntity).type = PassiveItemComponent::Type::SpeedBoost2;
	s_entityManager.AddComponent<PickupComponent>(speedBoostEntity, PickupComponent::Type::PassiveItem).itemName = "Speed Boost X2";
	s_entityManager.AddComponent<ModelComponent>(speedBoostEntity, speedBoostID);
	s_entityManager.AddComponent<TransformComponent>(speedBoostEntity, position).SetScale({ 0.5f, 0.5f, 0.5f });
	s_entityManager.AddComponent<ShadowReceiverComponent>(speedBoostEntity);
	auto& ni = s_entityManager.AddComponent<NetworkId>(speedBoostEntity);
	ni.entityTypeId = EntityTypes::IncreaseSpeed2;
	if (id == 0)
		ni.id = ++s_pickupId;
	else
	{
		ni.id = id;
	}

	LuaMain::GetScriptManager()->AddScript(speedBoostEntity, "Pickupable.lua");

	auto& lerpAnimator = s_entityManager.AddComponent<PickupLerpAnimateComponent>(speedBoostEntity);
	lerpAnimator.baseOrigin = s_entityManager.GetComponent<TransformComponent>(speedBoostEntity).GetPosition().y;
	lerpAnimator.baseTarget = lerpAnimator.baseOrigin + 2.0f;
	lerpAnimator.currentOrigin = lerpAnimator.baseOrigin;
	return ni.id;
}

u32 ItemManager::CreateJumpBoost(DirectX::SimpleMath::Vector3 position, u32 id)
{
	u32 jumpBoostId = AssetManager::Get().LoadModelAsset("Assets/Models/Pickups/JumpBoost.glb");

	entity pEntity = s_entityManager.CreateEntity();
	s_entityManager.AddComponent<PassiveItemComponent>(pEntity).type = PassiveItemComponent::Type::JumpBoost;
	s_entityManager.AddComponent<PickupComponent>(pEntity, PickupComponent::Type::PassiveItem).itemName = "JumpBoost";
	s_entityManager.AddComponent<ModelComponent>(pEntity, jumpBoostId);
	s_entityManager.AddComponent<TransformComponent>(pEntity, position).SetScale({ 0.3f, 0.3f, 0.3f });
	s_entityManager.AddComponent<ShadowReceiverComponent>(pEntity);
	auto& ni = s_entityManager.AddComponent<NetworkId>(pEntity);
	ni.entityTypeId = EntityTypes::JumpBoost;
	if (id == 0)
		ni.id = ++s_pickupId;
	else
	{
		ni.id = id;
	}

	LuaMain::GetScriptManager()->AddScript(pEntity, "Pickupable.lua");

	auto& lerpAnimator = s_entityManager.AddComponent<PickupLerpAnimateComponent>(pEntity);
	lerpAnimator.baseOrigin = s_entityManager.GetComponent<TransformComponent>(pEntity).GetPosition().y;
	lerpAnimator.baseTarget = lerpAnimator.baseOrigin + 2.0f;
	lerpAnimator.currentOrigin = lerpAnimator.baseOrigin;
	return ni.id;
}

u32 ItemManager::CreateFullAutoPickup(Vector3 position, u32 id)
{
	u32 modelID = AssetManager::Get().LoadModelAsset("Assets/Models/ModularRifle/FullAuto.gltf");

	entity fullAutoEntity = s_entityManager.CreateEntity();
	s_entityManager.AddComponent<MiscComponent>(fullAutoEntity).type = MiscComponent::Type::FullAuto;
	s_entityManager.AddComponent<PickupComponent>(fullAutoEntity, PickupComponent::Type::MiscItem).itemName = "Full Auto";
	s_entityManager.AddComponent<ModelComponent>(fullAutoEntity, modelID);
	s_entityManager.AddComponent<TransformComponent>(fullAutoEntity, position).SetScale({ 0.3f, 0.3f, 0.3f });
	s_entityManager.AddComponent<ShadowReceiverComponent>(fullAutoEntity);
	auto& ni = s_entityManager.AddComponent<NetworkId>(fullAutoEntity);
	ni.entityTypeId = EntityTypes::FullAutoMisc;
	if (id == 0)
		ni.id = ++s_pickupId;
	else
	{
		ni.id = id;
	}

	LuaMain::GetScriptManager()->AddScript(fullAutoEntity, "Pickupable.lua");

	auto& lerpAnimator = s_entityManager.AddComponent<PickupLerpAnimateComponent>(fullAutoEntity);
	lerpAnimator.baseOrigin = s_entityManager.GetComponent<TransformComponent>(fullAutoEntity).GetPosition().y;
	lerpAnimator.baseTarget = lerpAnimator.baseOrigin + 2.0f;
	lerpAnimator.currentOrigin = lerpAnimator.baseOrigin;
	return ni.id;
}

u32 ItemManager::CreateChargeShotPickup(Vector3 position, u32 id)
{
	u32 modelID = AssetManager::Get().LoadModelAsset("Assets/Models/ModularRifle/ChargeShot.gltf");

	entity chargeShotEntity = s_entityManager.CreateEntity();
	s_entityManager.AddComponent<MiscComponent>(chargeShotEntity).type = MiscComponent::Type::ChargeShot;
	s_entityManager.AddComponent<PickupComponent>(chargeShotEntity, PickupComponent::Type::MiscItem).itemName = "Charge Shot";
	s_entityManager.AddComponent<ModelComponent>(chargeShotEntity, modelID);
	s_entityManager.AddComponent<TransformComponent>(chargeShotEntity, position).SetScale({ 0.3f, 0.3f, 0.3f });
	s_entityManager.AddComponent<ShadowReceiverComponent>(chargeShotEntity);
	auto& ni = s_entityManager.AddComponent<NetworkId>(chargeShotEntity);
	ni.entityTypeId = EntityTypes::ChargeShotMisc;
	if (id == 0)
		ni.id = ++s_pickupId;
	else
	{
		ni.id = id;
	}

	LuaMain::GetScriptManager()->AddScript(chargeShotEntity, "Pickupable.lua");

	auto& lerpAnimator = s_entityManager.AddComponent<PickupLerpAnimateComponent>(chargeShotEntity);
	lerpAnimator.baseOrigin = s_entityManager.GetComponent<TransformComponent>(chargeShotEntity).GetPosition().y;
	lerpAnimator.baseTarget = lerpAnimator.baseOrigin + 2.0f;
	lerpAnimator.currentOrigin = lerpAnimator.baseOrigin;
	return ni.id;
}

u32 ItemManager::CreateReviverPickup(Vector3 position, u32 id)
{
	u32 modelID = AssetManager::Get().LoadModelAsset("Assets/Models/Pickups/Reviver.glb");

	entity reviverEntity = s_entityManager.CreateEntity();
	s_entityManager.AddComponent<ActiveItemComponent>(reviverEntity).type = ActiveItemComponent::Type::Reviver;
	s_entityManager.AddComponent<PickupComponent>(reviverEntity, PickupComponent::Type::ActiveItem).itemName = "Reviver";
	s_entityManager.AddComponent<ModelComponent>(reviverEntity, modelID);
	s_entityManager.AddComponent<TransformComponent>(reviverEntity, position).SetScale({ 0.3f, 0.3f, 0.3f });
	s_entityManager.AddComponent<ShadowReceiverComponent>(reviverEntity);
	auto& ni = s_entityManager.AddComponent<NetworkId>(reviverEntity);
	ni.entityTypeId = EntityTypes::Reviver;
	if (id == 0)
		ni.id = ++s_pickupId;
	else
	{
		ni.id = id;
	}

	LuaMain::GetScriptManager()->AddScript(reviverEntity, "Pickupable.lua");

	auto& lerpAnimator = s_entityManager.AddComponent<PickupLerpAnimateComponent>(reviverEntity);
	lerpAnimator.baseOrigin = s_entityManager.GetComponent<TransformComponent>(reviverEntity).GetPosition().y;
	lerpAnimator.baseTarget = lerpAnimator.baseOrigin + 2.0f;
	lerpAnimator.currentOrigin = lerpAnimator.baseOrigin;
	return ni.id;
}

u32 ItemManager::CreateGoalRadarPickup(Vector3 position, u32 id)
{
	u32 goalRadarID = AssetManager::Get().LoadModelAsset("Assets/Models/ModularRifle/radar.gltf");

	entity goalRadarEntity = s_entityManager.CreateEntity();
	s_entityManager.AddComponent<ActiveItemComponent>(goalRadarEntity).type = ActiveItemComponent::Type::GoalRadar;
	s_entityManager.AddComponent<PickupComponent>(goalRadarEntity, PickupComponent::Type::ActiveItem).itemName = "Goal Radar";
	s_entityManager.AddComponent<ModelComponent>(goalRadarEntity, goalRadarID);
	s_entityManager.AddComponent<TransformComponent>(goalRadarEntity, position).SetScale({ 1.f, 1.f, 1.f });
	s_entityManager.AddComponent<ShadowReceiverComponent>(goalRadarEntity);
	auto& ni = s_entityManager.AddComponent<NetworkId>(goalRadarEntity);
	ni.entityTypeId = EntityTypes::GoalRadar;
	if (id == 0)
		ni.id = ++s_pickupId;
	else
	{
		ni.id = id;
	}
	LuaMain::GetScriptManager()->AddScript(goalRadarEntity, "Pickupable.lua");

	auto& lerpAnimator = s_entityManager.AddComponent<PickupLerpAnimateComponent>(goalRadarEntity);
	lerpAnimator.baseOrigin = s_entityManager.GetComponent<TransformComponent>(goalRadarEntity).GetPosition().y;
	lerpAnimator.baseTarget = lerpAnimator.baseOrigin + 2.0f;
	lerpAnimator.currentOrigin = lerpAnimator.baseOrigin;
	return ni.id;
}

u32 ItemManager::CreateSyringePickup(Vector3 position, u32 id)
{
	u32 modelID = AssetManager::Get().LoadModelAsset("Assets/Models/Pickups/Syringe.glb");

	entity entity = s_entityManager.CreateEntity();
	s_entityManager.AddComponent<ActiveItemComponent>(entity).type = ActiveItemComponent::Type::Syringe;
	s_entityManager.AddComponent<PickupComponent>(entity, PickupComponent::Type::ActiveItem).itemName = "Syringe";
	s_entityManager.AddComponent<ModelComponent>(entity, modelID);
	s_entityManager.AddComponent<TransformComponent>(entity, position).SetScale({ 0.2f, 0.2f, 0.2f });
	s_entityManager.AddComponent<ShadowReceiverComponent>(entity);
	auto& ni = s_entityManager.AddComponent<NetworkId>(entity);
	ni.entityTypeId = EntityTypes::Syringe;
	if (id == 0)
		ni.id = ++s_pickupId;
	else
	{
		ni.id = id;
	}

	LuaMain::GetScriptManager()->AddScript(entity, "Pickupable.lua");

	auto& lerpAnimator = s_entityManager.AddComponent<PickupLerpAnimateComponent>(entity);
	lerpAnimator.baseOrigin = s_entityManager.GetComponent<TransformComponent>(entity).GetPosition().y;
	lerpAnimator.baseTarget = lerpAnimator.baseOrigin + 2.0f;
	lerpAnimator.currentOrigin = lerpAnimator.baseOrigin;
	return ni.id;
}
