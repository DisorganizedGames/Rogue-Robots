#include "MainScene.h"
#include "GameComponent.h"
using namespace DOG;
using namespace DirectX;
using namespace DirectX::SimpleMath;


MainScene::MainScene() : Scene(SceneType::MainScene)
{

}

void MainScene::SetUpScene(std::vector<std::function<std::vector<DOG::entity>()>> entityCreators)
{
	for (auto& func : entityCreators)
	{
		auto entities = func();
		for (entity e : entities)
			AddComponent<SceneComponent>(e, m_sceneType);
	}
	CreateGrenadePickup({ 15, 81, 50 });
	CreateGrenadePickup({ 15, 81, 50 });
	CreateGrenadePickup({ 17, 81, 50 });
	CreateGrenadePickup({ 17, 81, 50 });
	CreateMissilePickup({ 15, 81, 52 });
	CreateMissilePickup({ 15, 81, 52 });
	CreateMissilePickup({ 17, 81, 52 });
	CreateMissilePickup({ 17, 81, 52 });

	CreateFrostModificationPickup({ 3, 76, 6 });
	CreateMissilePickup({ 1, 76, 6 });
	CreateMissilePickup({ 1, 76, 6 });

	CreateTrampolinePickup({ 27, 76, 15 });
	CreateGrenadePickup({ 27, 76, 17 });
	CreateGrenadePickup({ 27, 76, 17 });
	CreateMissilePickup({ 27, 76, 19 });
	CreateMissilePickup({ 27, 76, 19 });

	CreateGrenadePickup({ 33, 76, 0 });
	CreateGrenadePickup({ 33, 76, 0 });
	CreateMaxHealthBoostPickup({ 33, 76, 2 });
	CreateMaxHealthBoostPickup({ 35, 76, 2 });
	CreateMaxHealthBoostPickup({ 37, 76, 2 });
	CreateMaxHealthBoostPickup({ 39, 76, 2 });
	CreateLight({ 31, 78, 2 }, {1.0f, 0.1f, 0.1f}, 7.0f);
	CreateLight({ 35, 78, 2 }, { 0.1f, 1.0f, 0.1f }, 7.0f);
	CreateLight({ 38, 78, 2 }, { 0.1f, 0.1f, 1.0f }, 7.0f);
}


void MainScene::CreateTrampolinePickup(DirectX::SimpleMath::Vector3 position)
{
	u32 trampolineID = AssetManager::Get().LoadModelAsset("Assets/Models/Temporary_Assets/Trampoline.glb");

	entity trampolineEntity = CreateEntity();
	AddComponent<ActiveItemComponent>(trampolineEntity).type = ActiveItemComponent::Type::Trampoline;
	AddComponent<PickupComponent>(trampolineEntity);
	AddComponent<ModelComponent>(trampolineEntity, trampolineID);
	AddComponent<TransformComponent>(trampolineEntity, position).SetScale({ 0.3f, 0.3f, 0.3f });
	AddComponent<ShadowReceiverComponent>(trampolineEntity);
	LuaMain::GetScriptManager()->AddScript(trampolineEntity, "Pickupable.lua");

	auto& lerpAnimator = AddComponent<PickupLerpAnimateComponent>(trampolineEntity);
	lerpAnimator.baseOrigin = s_entityManager.GetComponent<TransformComponent>(trampolineEntity).GetPosition().y;
	lerpAnimator.baseTarget = lerpAnimator.baseOrigin + 2.0f;
	lerpAnimator.currentOrigin = lerpAnimator.baseOrigin;
}

void MainScene::CreateMissilePickup(DirectX::SimpleMath::Vector3 position)
{
	u32 missileID = AssetManager::Get().LoadModelAsset("Assets/Models/Ammunition/missile.glb");

	entity missileEntity = CreateEntity();
	auto& bc = AddComponent<BarrelComponent>(missileEntity);
	bc.type = BarrelComponent::Type::Missile;
	bc.maximumAmmoCapacityForType = 5;
	bc.ammoPerPickup = 1;
	AddComponent<PickupComponent>(missileEntity);
	AddComponent<ModelComponent>(missileEntity, missileID);
	AddComponent<TransformComponent>(missileEntity, position).SetScale({ 0.8f, 0.8f, 0.8f });
	AddComponent<ShadowReceiverComponent>(missileEntity);
	LuaMain::GetScriptManager()->AddScript(missileEntity, "Pickupable.lua");

	auto& lerpAnimator = AddComponent<PickupLerpAnimateComponent>(missileEntity);
	lerpAnimator.baseOrigin = s_entityManager.GetComponent<TransformComponent>(missileEntity).GetPosition().y;
	lerpAnimator.baseTarget = lerpAnimator.baseOrigin + 2.0f;
	lerpAnimator.currentOrigin = lerpAnimator.baseOrigin;
}

void MainScene::CreateGrenadePickup(DirectX::SimpleMath::Vector3 position)
{
	u32 grenadeID = AssetManager::Get().LoadModelAsset("Assets/Models/Ammunition/Grenade/Grenade.glb");

	entity grenadeEntity = CreateEntity();
	auto& bc = AddComponent<BarrelComponent>(grenadeEntity);
	bc.type = BarrelComponent::Type::Grenade;
	bc.maximumAmmoCapacityForType = 10;
	bc.ammoPerPickup = 2;
	AddComponent<PickupComponent>(grenadeEntity);
	AddComponent<ModelComponent>(grenadeEntity, grenadeID);
	AddComponent<TransformComponent>(grenadeEntity, position).SetScale({ 0.5f, 0.5f, 0.5f });
	AddComponent<ShadowReceiverComponent>(grenadeEntity);
	LuaMain::GetScriptManager()->AddScript(grenadeEntity, "Pickupable.lua");

	auto& lerpAnimator = AddComponent<PickupLerpAnimateComponent>(grenadeEntity);
	lerpAnimator.baseOrigin = s_entityManager.GetComponent<TransformComponent>(grenadeEntity).GetPosition().y;
	lerpAnimator.baseTarget = lerpAnimator.baseOrigin + 2.0f;
	lerpAnimator.currentOrigin = lerpAnimator.baseOrigin;
}

void MainScene::CreateMaxHealthBoostPickup(DirectX::SimpleMath::Vector3 position)
{
	u32 healthBoostID = AssetManager::Get().LoadModelAsset("Assets/Models/Temporary_Assets/Medkit.glb");

	entity healthBoostEntity = CreateEntity();
	AddComponent<PassiveItemComponent>(healthBoostEntity).type = PassiveItemComponent::Type::MaxHealthBoost;
	AddComponent<PickupComponent>(healthBoostEntity);
	AddComponent<ModelComponent>(healthBoostEntity, healthBoostID);
	AddComponent<TransformComponent>(healthBoostEntity, position).SetScale({ 0.5f, 0.5f, 0.5f });
	AddComponent<ShadowReceiverComponent>(healthBoostEntity);
	LuaMain::GetScriptManager()->AddScript(healthBoostEntity, "Pickupable.lua");

	auto& lerpAnimator = AddComponent<PickupLerpAnimateComponent>(healthBoostEntity);
	lerpAnimator.baseOrigin = s_entityManager.GetComponent<TransformComponent>(healthBoostEntity).GetPosition().y;
	lerpAnimator.baseTarget = lerpAnimator.baseOrigin + 2.0f;
	lerpAnimator.currentOrigin = lerpAnimator.baseOrigin;
}

void MainScene::CreateFrostModificationPickup(DirectX::SimpleMath::Vector3 position)
{
	u32 frostModID = AssetManager::Get().LoadModelAsset("Assets/Models/Temporary_Assets/blue_cube.glb");

	entity frostModEntity = CreateEntity();
	AddComponent<MagazineModificationComponent>(frostModEntity).type = MagazineModificationComponent::Type::Frost;
	AddComponent<PickupComponent>(frostModEntity);
	AddComponent<ModelComponent>(frostModEntity, frostModID);
	AddComponent<TransformComponent>(frostModEntity, position).SetScale({ 0.3f, 0.3f, 0.3f });
	AddComponent<ShadowReceiverComponent>(frostModEntity);
	LuaMain::GetScriptManager()->AddScript(frostModEntity, "Pickupable.lua");

	auto& lerpAnimator = AddComponent<PickupLerpAnimateComponent>(frostModEntity);
	lerpAnimator.baseOrigin = s_entityManager.GetComponent<TransformComponent>(frostModEntity).GetPosition().y;
	lerpAnimator.baseTarget = lerpAnimator.baseOrigin + 2.0f;
	lerpAnimator.currentOrigin = lerpAnimator.baseOrigin;
}



void MainScene::CreateLight(DirectX::SimpleMath::Vector3 position, DirectX::SimpleMath::Vector3 color, float strength)
{
	//LightHandle pointLight = LightManager::Get().AddPointLight(PointLightDesc(), LightUpdateFrequency::Never);
	entity movingPointLight = CreateEntity();
	AddComponent<TransformComponent>(movingPointLight, position, DirectX::SimpleMath::Vector3(0, 0, 0), DirectX::SimpleMath::Vector3(1.f));
	//AddComponent<PointLightComponent>(movingPointLight, pointLight, color, strength);
}