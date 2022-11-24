#include "ItemManager.h"
#include "Game/GameLayer.h"
#include "../LoadSplitModels.h"
#include "../PrefabInstantiatorFunctions.h"

using namespace DOG;
using namespace DirectX::SimpleMath;

EntityManager& ItemManager::s_entityManager = EntityManager::Get();
ItemManager ItemManager::s_amInstance;
bool ItemManager::s_notInitialized = true;

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
}


u32 ItemManager::CreateTrampolinePickup(Vector3 position, u32 id)
{
	static u32 trampolineNetworkID = 0u;
	u32 trampolineID = AssetManager::Get().LoadModelAsset("Assets/Models/Temporary_Assets/Trampoline.glb");

	entity trampolineEntity = s_entityManager.CreateEntity();
	s_entityManager.AddComponent<ActiveItemComponent>(trampolineEntity).type = ActiveItemComponent::Type::Trampoline;
	s_entityManager.AddComponent<PickupComponent>(trampolineEntity).itemName = "Trampoline";
	s_entityManager.AddComponent<ModelComponent>(trampolineEntity, trampolineID);
	s_entityManager.AddComponent<TransformComponent>(trampolineEntity, position).SetScale({ 0.3f, 0.3f, 0.3f });
	s_entityManager.AddComponent<ShadowReceiverComponent>(trampolineEntity);
	auto& ni = s_entityManager.AddComponent<NetworkId>(trampolineEntity);
	ni.entityTypeId = EntityTypes::Trampoline;
	if (id == 0)
		ni.id = ++trampolineNetworkID;
	else
	{
		ni.id = id;
		trampolineNetworkID = id;
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
	static u32 missileNetworkID = 0u;
	u32 missileID = AssetManager::Get().LoadModelAsset("Assets/Models/Ammunition/missile.glb");

	entity missileEntity = s_entityManager.CreateEntity();
	auto& bc = s_entityManager.AddComponent<BarrelComponent>(missileEntity);
	bc.type = BarrelComponent::Type::Missile;
	bc.maximumAmmoCapacityForType = 10;
	bc.ammoPerPickup = 1;
	s_entityManager.AddComponent<PickupComponent>(missileEntity).itemName = "Homing missile";
	s_entityManager.AddComponent<ModelComponent>(missileEntity, missileID);
	s_entityManager.AddComponent<TransformComponent>(missileEntity, position).SetScale({ 0.8f, 0.8f, 0.8f });
	s_entityManager.AddComponent<ShadowReceiverComponent>(missileEntity);
	auto& ni = s_entityManager.AddComponent<NetworkId>(missileEntity);
	ni.entityTypeId = EntityTypes::MissileBarrel;
	if (id == 0)
		ni.id = ++missileNetworkID;
	else
	{
		ni.id = id;
		missileNetworkID = id;
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
	static u32 laserNetworkID = 0u;

	entity laserEntity = SpawnLaserBlob(TransformComponent(position, Vector3::Zero, Vector3(0.5f, 0.5f, 0.5f)), NULL_ENTITY);
	auto& bc = s_entityManager.AddComponent<BarrelComponent>(laserEntity);
	bc.type = BarrelComponent::Type::Laser;
	bc.maximumAmmoCapacityForType = 80;
	bc.ammoPerPickup = 20;
	s_entityManager.AddComponent<PickupComponent>(laserEntity).itemName = "Laser";
	auto& ni = s_entityManager.AddComponent<NetworkId>(laserEntity);
	ni.entityTypeId = EntityTypes::LaserBarrel;
	if (id == 0)
		ni.id = ++laserNetworkID;
	else
		ni.id = id;

	LuaMain::GetScriptManager()->AddScript(laserEntity, "Pickupable.lua");

	auto& lerpAnimator = s_entityManager.AddComponent<PickupLerpAnimateComponent>(laserEntity);
	lerpAnimator.baseOrigin = s_entityManager.GetComponent<TransformComponent>(laserEntity).GetPosition().y;
	lerpAnimator.baseTarget = lerpAnimator.baseOrigin + 2.0f;
	lerpAnimator.currentOrigin = lerpAnimator.baseOrigin;
	return ni.id;
}

u32 ItemManager::CreateGrenadePickup(DirectX::SimpleMath::Vector3 position, u32 id)
{
	static u32 grenadeNetworkID = 0u;

	u32 grenadeID = AssetManager::Get().LoadModelAsset("Assets/Models/Ammunition/Grenade/Grenade.glb");

	entity grenadeEntity = s_entityManager.CreateEntity();
	auto& bc = s_entityManager.AddComponent<BarrelComponent>(grenadeEntity);
	bc.type = BarrelComponent::Type::Grenade;
	bc.maximumAmmoCapacityForType = 20;
	bc.ammoPerPickup = 2;
	s_entityManager.AddComponent<PickupComponent>(grenadeEntity).itemName = "Grenade";
	s_entityManager.AddComponent<ModelComponent>(grenadeEntity, grenadeID);
	s_entityManager.AddComponent<TransformComponent>(grenadeEntity, position).SetScale({ 0.5f, 0.5f, 0.5f });
	s_entityManager.AddComponent<ShadowReceiverComponent>(grenadeEntity);
	auto& ni = s_entityManager.AddComponent<NetworkId>(grenadeEntity);
	ni.entityTypeId = EntityTypes::GrenadeBarrel;
	if (id == 0)
		ni.id = ++grenadeNetworkID;
	else
	{
		ni.id = id;
		grenadeNetworkID = id;
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
	static u32 healtBoostNetworkdID = 0u;

	u32 healthBoostID = AssetManager::Get().LoadModelAsset("Assets/Models/Temporary_Assets/Medkit.glb");

	entity healthBoostEntity = s_entityManager.CreateEntity();
	s_entityManager.AddComponent<PassiveItemComponent>(healthBoostEntity).type = PassiveItemComponent::Type::MaxHealthBoost;
	s_entityManager.AddComponent<PickupComponent>(healthBoostEntity).itemName = "Max HP boost";
	s_entityManager.AddComponent<ModelComponent>(healthBoostEntity, healthBoostID);
	s_entityManager.AddComponent<TransformComponent>(healthBoostEntity, position).SetScale({ 0.8f, 0.8f, 0.8f });
	s_entityManager.AddComponent<ShadowReceiverComponent>(healthBoostEntity);
	auto& ni = s_entityManager.AddComponent<NetworkId>(healthBoostEntity);
	ni.entityTypeId = EntityTypes::IncreaseMaxHp;
	if (id == 0)
		ni.id = ++healtBoostNetworkdID;
	else
	{
		ni.id = id;
		healtBoostNetworkdID = id;
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
	static u32 frostModNetworkID = 0u;

	u32 frostModID = AssetManager::Get().LoadModelAsset("Assets/Models/Temporary_Assets/blue_cube.glb");

	entity frostModEntity = s_entityManager.CreateEntity();
	s_entityManager.AddComponent<MagazineModificationComponent>(frostModEntity).type = MagazineModificationComponent::Type::Frost;
	s_entityManager.AddComponent<PickupComponent>(frostModEntity).itemName = "Frost modification";
	s_entityManager.AddComponent<ModelComponent>(frostModEntity, frostModID);
	s_entityManager.AddComponent<TransformComponent>(frostModEntity, position).SetScale({ 0.3f, 0.3f, 0.3f });
	s_entityManager.AddComponent<ShadowReceiverComponent>(frostModEntity);
	auto& ni = s_entityManager.AddComponent<NetworkId>(frostModEntity);
	ni.entityTypeId = EntityTypes::FrostMagazineModification;
	if (id == 0)
		ni.id = ++frostModNetworkID;
	else
	{
		ni.id = id;
		frostModNetworkID = id;
	}

	LuaMain::GetScriptManager()->AddScript(frostModEntity, "Pickupable.lua");

	auto& lerpAnimator = s_entityManager.AddComponent<PickupLerpAnimateComponent>(frostModEntity);
	lerpAnimator.baseOrigin = s_entityManager.GetComponent<TransformComponent>(frostModEntity).GetPosition().y;
	lerpAnimator.baseTarget = lerpAnimator.baseOrigin + 2.0f;
	lerpAnimator.currentOrigin = lerpAnimator.baseOrigin;
	return ni.id;
}

u32 ItemManager::CreateTurretPickup(Vector3 position, u32 id)
{

	static u32 turretNetworkID = 0u;
	u32 turretBaseModelID = AssetManager::Get().LoadModelAsset("Assets/Models/Temporary_Assets/turretBase.glb");
	u32 turretHeadModelID = AssetManager::Get().LoadModelAsset("Assets/Models/Temporary_Assets/turret2.glb");

	entity turretPickUpEntity = s_entityManager.CreateEntity();
	s_entityManager.AddComponent<ActiveItemComponent>(turretPickUpEntity).type = ActiveItemComponent::Type::Turret;
	s_entityManager.AddComponent<PickupComponent>(turretPickUpEntity).itemName = "Turret";
	s_entityManager.AddComponent<ModelComponent>(turretPickUpEntity, turretBaseModelID);
	s_entityManager.AddComponent<TransformComponent>(turretPickUpEntity, position).SetScale({ 0.4f, 0.4f, 0.4f });
	s_entityManager.AddComponent<ShadowReceiverComponent>(turretPickUpEntity);
	auto& ni = s_entityManager.AddComponent<NetworkId>(turretPickUpEntity);
	ni.entityTypeId = EntityTypes::Turret;
	if (id == 0)
		ni.id = ++turretNetworkID;
	else
	{
		ni.id = id;
		turretNetworkID = id;
	}

	LuaMain::GetScriptManager()->AddScript(turretPickUpEntity, "Pickupable.lua");

	auto& lerpAnimator = s_entityManager.AddComponent<PickupLerpAnimateComponent>(turretPickUpEntity);
	lerpAnimator.baseOrigin = s_entityManager.GetComponent<TransformComponent>(turretPickUpEntity).GetPosition().y;
	lerpAnimator.baseTarget = lerpAnimator.baseOrigin + 2.0f;
	lerpAnimator.currentOrigin = lerpAnimator.baseOrigin;

	entity turretHeadpEntity = s_entityManager.CreateEntity();
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
	static u32 speedBoostNetworkdID = 0u;

	u32 speedBoostID = AssetManager::Get().LoadModelAsset("Assets/Models/Temporary_Assets/speedPassive.glb");

	entity speedBoostEntity = s_entityManager.CreateEntity();
	s_entityManager.AddComponent<PassiveItemComponent>(speedBoostEntity).type = PassiveItemComponent::Type::SpeedBoost;
	s_entityManager.AddComponent<PickupComponent>(speedBoostEntity).itemName = "Speed Boost";
	s_entityManager.AddComponent<ModelComponent>(speedBoostEntity, speedBoostID);
	s_entityManager.AddComponent<TransformComponent>(speedBoostEntity, position).SetScale({ 0.5f, 0.5f, 0.5f });
	s_entityManager.AddComponent<ShadowReceiverComponent>(speedBoostEntity);
	auto& ni = s_entityManager.AddComponent<NetworkId>(speedBoostEntity);
	ni.entityTypeId = EntityTypes::IncreaseSpeed;
	if (id == 0)
		ni.id = ++speedBoostNetworkdID;
	else
	{
		ni.id = id;
		speedBoostNetworkdID = id;
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
	static u32 speedBoostNetworkdID2 = 0u;

	u32 speedBoostID = AssetManager::Get().LoadModelAsset("Assets/Models/Temporary_Assets/speedPassive2.glb");

	entity speedBoostEntity = s_entityManager.CreateEntity();
	s_entityManager.AddComponent<PassiveItemComponent>(speedBoostEntity).type = PassiveItemComponent::Type::SpeedBoost2;
	s_entityManager.AddComponent<PickupComponent>(speedBoostEntity).itemName = "Speed Boost X2";
	s_entityManager.AddComponent<ModelComponent>(speedBoostEntity, speedBoostID);
	s_entityManager.AddComponent<TransformComponent>(speedBoostEntity, position).SetScale({ 0.5f, 0.5f, 0.5f });
	s_entityManager.AddComponent<ShadowReceiverComponent>(speedBoostEntity);
	auto& ni = s_entityManager.AddComponent<NetworkId>(speedBoostEntity);
	ni.entityTypeId = EntityTypes::IncreaseSpeed2;
	if (id == 0)
		ni.id = ++speedBoostNetworkdID2;
	else
	{
		ni.id = id;
		speedBoostNetworkdID2 = id;
	}

	LuaMain::GetScriptManager()->AddScript(speedBoostEntity, "Pickupable.lua");

	auto& lerpAnimator = s_entityManager.AddComponent<PickupLerpAnimateComponent>(speedBoostEntity);
	lerpAnimator.baseOrigin = s_entityManager.GetComponent<TransformComponent>(speedBoostEntity).GetPosition().y;
	lerpAnimator.baseTarget = lerpAnimator.baseOrigin + 2.0f;
	lerpAnimator.currentOrigin = lerpAnimator.baseOrigin;
	return ni.id;
}


//Placeholder
//u32 ItemManager::CreateHealthPickup(DirectX::SimpleMath::Vector3 position, u32 id)
//{
//	static u32 healthNetworkdID = 0u;
//
//	u32 healthBoostID = AssetManager::Get().LoadModelAsset("Assets/Models/Temporary_Assets/Health.glb");
//
//	entity healthBoostEntity = s_entityManager.CreateEntity();
//	s_entityManager.AddComponent<PassiveItemComponent>(healthBoostEntity).type = PassiveItemComponent::Type::Template;
//	s_entityManager.AddComponent<PickupComponent>(healthBoostEntity).itemName = "Heal";
//	s_entityManager.AddComponent<ModelComponent>(healthBoostEntity, healthBoostID);
//	s_entityManager.AddComponent<TransformComponent>(healthBoostEntity, position).SetScale({ 0.3f, 0.3f, 0.3f });
//	s_entityManager.AddComponent<ShadowReceiverComponent>(healthBoostEntity);
//	auto& ni = s_entityManager.AddComponent<NetworkId>(healthBoostEntity);
//	ni.entityTypeId = EntityTypes::Health;
//	if (id == 0)
//		ni.id = ++healthNetworkdID;
//	else
//		ni.id = id;
//
//	LuaMain::GetScriptManager()->AddScript(healthBoostEntity, "Pickupable.lua");
//
//	auto& lerpAnimator = s_entityManager.AddComponent<PickupLerpAnimateComponent>(healthBoostEntity);
//	lerpAnimator.baseOrigin = s_entityManager.GetComponent<TransformComponent>(healthBoostEntity).GetPosition().y;
//	lerpAnimator.baseTarget = lerpAnimator.baseOrigin + 2.0f;
//	lerpAnimator.currentOrigin = lerpAnimator.baseOrigin;
//	return ni.id;
//}

u32 ItemManager::CreateJumpBoost(DirectX::SimpleMath::Vector3 position, u32 id)
{
	static u32 jumpBoostID = 0u;

	u32 jumpBoostId = AssetManager::Get().LoadModelAsset("Assets/Models/Temporary_Assets/TempJumBoostglb.glb");

	entity pEntity = s_entityManager.CreateEntity();
	s_entityManager.AddComponent<PassiveItemComponent>(pEntity).type = PassiveItemComponent::Type::JumpBoost;
	s_entityManager.AddComponent<PickupComponent>(pEntity).itemName = "JumpBoost";
	s_entityManager.AddComponent<ModelComponent>(pEntity, jumpBoostId);
	s_entityManager.AddComponent<TransformComponent>(pEntity, position).SetScale({ 0.3f, 0.3f, 0.3f });
	s_entityManager.AddComponent<ShadowReceiverComponent>(pEntity);
	auto& ni = s_entityManager.AddComponent<NetworkId>(pEntity);
	ni.entityTypeId = EntityTypes::JumpBoost;
	if (id == 0)
		ni.id = ++jumpBoostID;
	else
	{
		ni.id = id;
		jumpBoostID = id;
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
	static u32 fullAutoNetworkID = 0u;

	u32 modelID = AssetManager::Get().LoadModelAsset("Assets/Models/Temporary_Assets/green_cube.glb");

	entity fullAutoEntity = s_entityManager.CreateEntity();
	s_entityManager.AddComponent<MiscComponent>(fullAutoEntity).type = MiscComponent::Type::FullAuto;
	s_entityManager.AddComponent<PickupComponent>(fullAutoEntity).itemName = "Full Auto";
	s_entityManager.AddComponent<ModelComponent>(fullAutoEntity, modelID);
	s_entityManager.AddComponent<TransformComponent>(fullAutoEntity, position).SetScale({ 0.3f, 0.3f, 0.3f });
	s_entityManager.AddComponent<ShadowReceiverComponent>(fullAutoEntity);
	auto& ni = s_entityManager.AddComponent<NetworkId>(fullAutoEntity);
	ni.entityTypeId = EntityTypes::FullAutoMisc;
	if (id == 0)
		ni.id = ++fullAutoNetworkID;
	else
	{
		ni.id = id;
		fullAutoNetworkID = id;
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
	static u32 chargeShotNetworkID = 0u;

	u32 modelID = AssetManager::Get().LoadModelAsset("Assets/Models/Temporary_Assets/red_cube.glb");

	entity chargeShotEntity = s_entityManager.CreateEntity();
	s_entityManager.AddComponent<MiscComponent>(chargeShotEntity).type = MiscComponent::Type::ChargeShot;
	s_entityManager.AddComponent<PickupComponent>(chargeShotEntity).itemName = "Charge Shot";
	s_entityManager.AddComponent<ModelComponent>(chargeShotEntity, modelID);
	s_entityManager.AddComponent<TransformComponent>(chargeShotEntity, position).SetScale({ 0.3f, 0.3f, 0.3f });
	s_entityManager.AddComponent<ShadowReceiverComponent>(chargeShotEntity);
	auto& ni = s_entityManager.AddComponent<NetworkId>(chargeShotEntity);
	ni.entityTypeId = EntityTypes::ChargeShotMisc;
	if (id == 0)
		ni.id = ++chargeShotNetworkID;
	else
	{
		ni.id = id;
		chargeShotNetworkID = id;
	}

	LuaMain::GetScriptManager()->AddScript(chargeShotEntity, "Pickupable.lua");

	auto& lerpAnimator = s_entityManager.AddComponent<PickupLerpAnimateComponent>(chargeShotEntity);
	lerpAnimator.baseOrigin = s_entityManager.GetComponent<TransformComponent>(chargeShotEntity).GetPosition().y;
	lerpAnimator.baseTarget = lerpAnimator.baseOrigin + 2.0f;
	lerpAnimator.currentOrigin = lerpAnimator.baseOrigin;
	return ni.id;
}
