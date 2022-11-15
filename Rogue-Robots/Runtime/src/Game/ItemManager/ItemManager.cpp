#include "ItemManager.h"
#include "Game/GameLayer.h"
#include "../LoadSplitModels.h"

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
	case EntityTypes::Medkit:
		break;
	case EntityTypes::IncreaseMaxHp:
		break;
	case EntityTypes::Trampoline:
		return CreateTrampolinePickup(position, id);
		break;
	case EntityTypes::BulletBarrel:
		break;
	case EntityTypes::GrenadeBarrel:
		return CreateGrenadePickup(position, id);
		break;
	case EntityTypes::MissileBarrel:
		return CreateMissilePickup(position, id);
		break;
	case EntityTypes::DefaultMagazineModification:
		break;
	case EntityTypes::FrostMagazineModification:
		return CreateFrostModificationPickup(position, id);
		break;
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
	EntityManager::Get().Collect<NetworkId>().Do([&](entity id, NetworkId& items)
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
		ni.id = id;

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
		ni.id = id;

	LuaMain::GetScriptManager()->AddScript(missileEntity, "Pickupable.lua");

	auto& lerpAnimator = s_entityManager.AddComponent<PickupLerpAnimateComponent>(missileEntity);
	lerpAnimator.baseOrigin = s_entityManager.GetComponent<TransformComponent>(missileEntity).GetPosition().y;
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
		ni.id = id;;

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
	s_entityManager.AddComponent<TransformComponent>(healthBoostEntity, position).SetScale({ 0.5f, 0.5f, 0.5f });
	s_entityManager.AddComponent<ShadowReceiverComponent>(healthBoostEntity);
	auto& ni = s_entityManager.AddComponent<NetworkId>(healthBoostEntity);
	ni.entityTypeId = EntityTypes::IncreaseMaxHp;
	if (id == 0)
		ni.id = ++healtBoostNetworkdID;
	else
		ni.id = id;

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
		ni.id = id;

	LuaMain::GetScriptManager()->AddScript(frostModEntity, "Pickupable.lua");

	auto& lerpAnimator = s_entityManager.AddComponent<PickupLerpAnimateComponent>(frostModEntity);
	lerpAnimator.baseOrigin = s_entityManager.GetComponent<TransformComponent>(frostModEntity).GetPosition().y;
	lerpAnimator.baseTarget = lerpAnimator.baseOrigin + 2.0f;
	lerpAnimator.currentOrigin = lerpAnimator.baseOrigin;
	return ni.id;
}